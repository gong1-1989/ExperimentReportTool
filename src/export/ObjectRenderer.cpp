#include "export/ObjectRenderer.h"

#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPixmap>
#include <QBuffer>
#include <QUrl>
#include <QDir>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>

#include "chart/ChartRenderer.h"
#include "extension/ObjectRegistry.h"
#include "chart/ChartConfigDialog.h"
#include "core/utils/Logger.h"

// ===========================================================================
// 选项工厂
// ===========================================================================

ObjectRenderer::Options ObjectRenderer::htmlExportOptions()
{
    Options opt;
    opt.imageMode = ImageMode::Base64Embed;
    opt.tableStyle = TableStyle::CssClass;
    opt.chartMode = ChartMode::Base64Embed;
    opt.maxTableRows = 100;
    opt.imageMaxWidth = 100000;   // 导出不限制页面适配
    opt.imageMaxHeight = 100000;
    opt.chartMaxWidth = 100000;
    opt.chartMaxHeight = 100000;
    opt.cssClass = QStringLiteral("report-image");
    opt.captionClass = QStringLiteral("image-caption");
    opt.formulaClass = QStringLiteral("formula");
    opt.emptyParaTemplate = QStringLiteral("<p class=\"empty-content\">%1</p>\n");
    opt.imageMissingText = QStringLiteral("[图片文件不存在]");
    opt.imageReadFailText = QStringLiteral("[图片读取失败]");
    opt.tableMissingText = QStringLiteral("[数据表不存在]");
    opt.chartFailText = QStringLiteral("[图表渲染失败]");
    opt.sourceMissingText = QStringLiteral("[数据源不存在]");
    opt.chartNotConfiguredText = QStringLiteral("[未配置图表]");
    return opt;
}

ObjectRenderer::Options ObjectRenderer::printOptions()
{
    Options opt;
    opt.imageMode = ImageMode::LocalFileUrl;
    opt.tableStyle = TableStyle::InlineCentered;
    opt.chartMode = ChartMode::TempFile;
    opt.maxTableRows = 50;
    opt.imageMaxWidth = 720;    // A4 可用内容宽
    opt.imageMaxHeight = 300;
    opt.chartMaxWidth = 720;
    opt.chartMaxHeight = 300;
    opt.cssClass = QString();
    opt.captionClass = QString();
    opt.formulaClass = QString();
    opt.emptyParaTemplate = QStringLiteral("<p align='center' style='color:#888;'>%1</p>");
    opt.imageMissingText = QStringLiteral("[图片文件不存在]");
    opt.imageReadFailText = QStringLiteral("[图片读取失败]");
    opt.tableMissingText = QStringLiteral("[数据表不存在]");
    opt.chartFailText = QStringLiteral("[图表渲染失败]");
    opt.sourceMissingText = QStringLiteral("[数据源不存在]");
    opt.chartNotConfiguredText = QStringLiteral("[未配置图表，请先选择数据源]");
    return opt;
}

// ===========================================================================
// 渲染
// ===========================================================================

QString ObjectRenderer::renderObject(const ContentBlock& object, const QString& align,
                                     const Options& opt, qint64 ownerReportId,
                                     const std::function<DataTable::Ptr(qint64)>& tableResolver,
                                     QList<QString>* outTempFiles)
{
    QString objHtml;

    auto emptyPara = [&](const QString& text) {
        return opt.emptyParaTemplate.arg(text);
    };

    switch (object.type) {
    case BlockType::Image: {
        const QString imagePath = object.data.value("path").toString();
        const QString caption = object.data.value("caption").toString();
        if (!imagePath.isEmpty() && QFile::exists(imagePath)) {
            if (opt.imageMode == ImageMode::Base64Embed) {
                QFile imgFile(imagePath);
                if (imgFile.open(QIODevice::ReadOnly)) {
                    const QByteArray bytes = imgFile.readAll();
                    const QString b64 = QString::fromLatin1(bytes.toBase64());
                    const QString ext = QFileInfo(imagePath).suffix().toLower();
                    objHtml += QString("<p class=\"%1\" align=\"%2\"><img src=\"data:image/%3;base64,%4\"/></p>\n")
                                   .arg(opt.cssClass).arg(align).arg(ext).arg(b64);
                    if (!caption.isEmpty()) {
                        objHtml += QString("<p class=\"%1\" align=\"%2\">%3</p>\n")
                                       .arg(opt.captionClass).arg(align).arg(caption.toHtmlEscaped());
                    }
                } else {
                    objHtml += emptyPara(opt.imageReadFailText);
                }
            } else {
                // 本地文件 URL（打印）：按实际大小渲染 + 页面适配
                const QString fileUrl = QUrl::fromLocalFile(imagePath).toString();
                const QImage imgInfo(imagePath);
                const int originWidth = imgInfo.width() > 0 ? imgInfo.width() : 560;
                int imgWidth = object.data.value("width").toInt();
                if (imgWidth <= 0) imgWidth = originWidth;
                int imgHeight = imgInfo.height() > 0
                    ? qRound(static_cast<double>(imgWidth) * imgInfo.height() / originWidth)
                    : 0;
                double scale = 1.0;
                if (imgWidth > opt.imageMaxWidth) scale = static_cast<double>(opt.imageMaxWidth) / imgWidth;
                if (imgHeight > 0 && imgHeight * scale > opt.imageMaxHeight)
                    scale = qMin(scale, static_cast<double>(opt.imageMaxHeight) / imgHeight);
                if (scale < 1.0 && imgHeight > 0) {
                    imgWidth = qRound(imgWidth * scale);
                    imgHeight = qRound(imgHeight * scale);
                }
                // line-height:1 避免图片行高被全局 line-height 放大，导致与后文间隔过大
                objHtml += QString("<p align='%1' style='line-height:1;'><img src='%2' width='%3' height='%4'/></p>")
                               .arg(align).arg(fileUrl).arg(imgWidth).arg(imgHeight);
                if (!caption.isEmpty()) {
                    objHtml += QString("<p align='%1' style='color:#666; font-size:10pt;'>%2</p>")
                                   .arg(align).arg(caption.toHtmlEscaped());
                }
            }
        } else {
            objHtml += emptyPara(opt.imageMissingText);
        }
        break;
    }

    case BlockType::Table:
    case BlockType::DataReference: {
        const qint64 tableId = static_cast<qint64>(object.data.value("tableId").toDouble());
        if (tableId > 0 && tableResolver) {
            const DataTable::Ptr table = tableResolver(tableId);
            if (table) {
                const QList<ColumnDefinition>& columnDefs = table->columns();
                const QList<QVariantList>& dataRows = table->rows();
                const int cols = qMax(columnDefs.size(), 1);
                const int maxRows = qMin(dataRows.size(), opt.maxTableRows);

                if (opt.tableStyle == TableStyle::CssClass) {
                    objHtml += QString("<table class=\"report-table\" align=\"%1\" border='1' cellspacing='0' cellpadding='4'>\n").arg(align);
                    objHtml += "<thead><tr>";
                    for (int c = 0; c < cols; ++c) {
                        const QString headerText = c < columnDefs.size()
                                                       ? columnDefs.at(c).name
                                                       : QString("列%1").arg(c + 1);
                        objHtml += QString("<th>%1</th>").arg(headerText.toHtmlEscaped());
                    }
                    objHtml += "</tr></thead>\n<tbody>\n";
                    for (int r = 0; r < maxRows; ++r) {
                        objHtml += "<tr>";
                        const QVariantList row = dataRows.at(r);
                        for (int c = 0; c < cols; ++c) {
                            const QString cell = c < row.size() ? row.at(c).toString() : QString();
                            objHtml += QString("<td>%1</td>").arg(cell.toHtmlEscaped());
                        }
                        objHtml += "</tr>\n";
                    }
                    objHtml += "</tbody></table>\n";
                } else {
                    // 内联居中样式（打印）
                    objHtml += QString("<p align='%1'><table border='1' cellspacing='0' cellpadding='4' width='100%' style='text-align:center;'>").arg(align);
                    objHtml += "<tr bgcolor='#e8e8e8'>";
                    for (int c = 0; c < cols; ++c) {
                        const QString headerText = c < columnDefs.size()
                                                       ? columnDefs.at(c).name
                                                       : QString("列%1").arg(c + 1);
                        objHtml += QString("<td align='center'><b>%1</b></td>").arg(headerText.toHtmlEscaped());
                    }
                    objHtml += "</tr>";
                    for (int r = 0; r < maxRows; ++r) {
                        objHtml += "<tr>";
                        const QVariantList row = dataRows.at(r);
                        for (int c = 0; c < cols; ++c) {
                            const QString cell = c < row.size() ? row.at(c).toString() : QString();
                            objHtml += QString("<td align='center'>%1</td>").arg(cell.toHtmlEscaped());
                        }
                        objHtml += "</tr>";
                    }
                    objHtml += "</table></p>";
                }

                const QString caption = object.data.value("caption").toString();
                if (!caption.isEmpty()) {
                    if (!opt.captionClass.isEmpty()) {
                        objHtml += QString("<p class=\"%1\" align=\"%2\">%3</p>\n")
                                       .arg(opt.captionClass).arg(align).arg(caption.toHtmlEscaped());
                    } else {
                        objHtml += QString("<p align='%1' style='color:#666; font-size:10pt;'>%2</p>")
                                       .arg(align).arg(caption.toHtmlEscaped());
                    }
                }
            } else {
                objHtml += emptyPara(opt.tableMissingText);
            }
        } else if (tableId <= 0) {
            // 旧版迁移表格：数据内嵌在 data（headers/cells）
            const QJsonArray headers = object.data.value("headers").toArray();
            const QJsonArray cells = object.data.value("cells").toArray();
            const int cols = qMax(headers.size(), 1);
            const int maxRows = qMin(cells.size(), opt.maxTableRows);

            if (opt.tableStyle == TableStyle::CssClass) {
                objHtml += QString("<table class=\"report-table\" align=\"%1\" border='1' cellspacing='0' cellpadding='4'>\n").arg(align);
                objHtml += "<thead><tr>";
                for (int c = 0; c < cols; ++c) {
                    const QString headerText = c < headers.size()
                                                   ? headers.at(c).toString()
                                                   : QString("列%1").arg(c + 1);
                    objHtml += QString("<th>%1</th>").arg(headerText.toHtmlEscaped());
                }
                objHtml += "</tr></thead>\n<tbody>\n";
                for (int r = 0; r < maxRows; ++r) {
                    objHtml += "<tr>";
                    const QJsonArray row = cells.at(r).toArray();
                    for (int c = 0; c < cols; ++c) {
                        const QString cell = c < row.size() ? row.at(c).toString() : QString();
                        objHtml += QString("<td>%1</td>").arg(cell.toHtmlEscaped());
                    }
                    objHtml += "</tr>\n";
                }
                objHtml += "</tbody></table>\n";
            } else {
                objHtml += QString("<p align='%1'><table border='1' cellspacing='0' cellpadding='4' width='100%' style='text-align:center;'>").arg(align);
                objHtml += "<tr bgcolor='#e8e8e8'>";
                for (int c = 0; c < cols; ++c) {
                    const QString headerText = c < headers.size()
                                                   ? headers.at(c).toString()
                                                   : QString("列%1").arg(c + 1);
                    objHtml += QString("<td align='center'><b>%1</b></td>").arg(headerText.toHtmlEscaped());
                }
                objHtml += "</tr>";
                for (int r = 0; r < maxRows; ++r) {
                    objHtml += "<tr>";
                    const QJsonArray row = cells.at(r).toArray();
                    for (int c = 0; c < cols; ++c) {
                        const QString cell = c < row.size() ? row.at(c).toString() : QString();
                        objHtml += QString("<td align='center'>%1</td>").arg(cell.toHtmlEscaped());
                    }
                    objHtml += "</tr>";
                }
                objHtml += "</table></p>";
            }
        } else {
            objHtml += emptyPara(opt.tableMissingText);
        }
        break;
    }

    case BlockType::Chart: {
        ChartConfig config = ChartConfig::fromJson(object.data.value("config").toObject());
        qint64 tableId = static_cast<qint64>(object.data.value("tableId").toDouble());
        if (tableId <= 0) tableId = config.dataTableId;
        config.dataTableId = tableId;
        if (tableId > 0 && tableResolver) {
            const DataTable::Ptr table = tableResolver(tableId);
            if (table) {
                if (opt.chartMode == ChartMode::Base64Embed) {
                    // 渲染走共享缓存：键含数据表更新时间与配置，数据/配置变化自动失效
                    const QPixmap pixmap = renderChartCached(config, table,
                        config.width > 0 ? config.width : 600,
                        config.height > 0 ? config.height : 400);
                    if (!pixmap.isNull()) {
                        QByteArray bytes;
                        QBuffer buffer(&bytes);
                        buffer.open(QIODevice::WriteOnly);
                        pixmap.save(&buffer, "PNG");
                        buffer.close();
                        const QString b64 = QString::fromLatin1(bytes.toBase64());
                        objHtml += QString("<p class=\"%1\" align=\"%2\"><img src=\"data:image/png;base64,%3\"/></p>\n")
                                       .arg(opt.cssClass).arg(align).arg(b64);
                        if (!config.title.isEmpty()) {
                            objHtml += QString("<p class=\"%1\" align=\"%2\">%3</p>\n")
                                           .arg(opt.captionClass).arg(align).arg(config.title.toHtmlEscaped());
                        }
                    } else {
                        objHtml += emptyPara(opt.chartFailText);
                    }
                } else {
                    // 临时文件（打印）：按配置大小 + 页面适配
                    const int chartConfigWidth = config.width > 0 ? config.width : 600;
                    const int chartConfigHeight = config.height > 0 ? config.height : 400;
                    int chartWidth = chartConfigWidth;
                    int chartHeight = chartConfigHeight;
                    double scale = 1.0;
                    if (chartWidth > opt.chartMaxWidth) scale = static_cast<double>(opt.chartMaxWidth) / chartWidth;
                    if (chartHeight * scale > opt.chartMaxHeight)
                        scale = qMin(scale, static_cast<double>(opt.chartMaxHeight) / chartHeight);
                    if (scale < 1.0) {
                        chartWidth = qRound(chartWidth * scale);
                        chartHeight = qRound(chartHeight * scale);
                    }
                    const QPixmap pixmap = renderChartCached(config, table, chartWidth, chartHeight);
                    if (!pixmap.isNull()) {
                        const QString tempFile = QString("%1/chart_%2_%3.png")
                            .arg(QDir::tempPath())
                            .arg(ownerReportId)
                            .arg(QDateTime::currentMSecsSinceEpoch());
                        pixmap.save(tempFile, "PNG");
                        if (outTempFiles) outTempFiles->append(tempFile);
                        const QString fileUrl = QUrl::fromLocalFile(tempFile).toString();
                        objHtml += QString("<p align='%1' style='line-height:1;'><img src='%2' width='%3' height='%4'/></p>")
                                       .arg(align).arg(fileUrl).arg(chartWidth).arg(chartHeight);
                        if (!config.title.isEmpty()) {
                            objHtml += QString("<p align='%1' style='color:#666; font-size:10pt;'>%2</p>")
                                           .arg(align).arg(config.title.toHtmlEscaped());
                        }
                    } else {
                        objHtml += emptyPara(opt.chartFailText);
                    }
                }
            } else {
                objHtml += emptyPara(opt.sourceMissingText);
            }
        } else {
            objHtml += emptyPara(opt.chartNotConfiguredText);
        }
        break;
    }

    case BlockType::Formula: {
        const QString latex = object.data.value("latex").toString();
        if (!latex.isEmpty()) {
            if (!opt.formulaClass.isEmpty()) {
                objHtml += QString("<p class=\"%1\" align=\"%2\">%3</p>\n")
                               .arg(opt.formulaClass).arg(align).arg(latex.toHtmlEscaped());
            } else {
                objHtml += QString("<p align='%1' style='font-style:italic; font-size:12pt;'>%2</p>")
                               .arg(align).arg(latex.toHtmlEscaped());
            }
        }
        break;
    }

    case BlockType::AttachmentCard:
    case BlockType::MediaRef: {
        // D 域：可插拔对象渲染（附件卡片 / 音视频引用）
        const QString typeId = (object.type == BlockType::AttachmentCard)
                                   ? QStringLiteral("attachment_card")
                                   : QStringLiteral("media_ref");
        DocumentObjectProviderPtr provider = ObjectRegistry::instance().providerById(typeId);
        if (provider) {
            const QString html = provider->renderHtml(object.data.toVariantMap(), align);
            if (!html.isEmpty()) objHtml += html;
        }
        break;
    }

    default:
        break;
    }

    return objHtml;
}

// ===========================================================================
// 图表渲染缓存
// ===========================================================================

namespace {
struct ChartCacheKey {
    qint64 tableId = 0;
    qint64 updatedMsecs = 0;   // 数据表更新时间：数据内容变化时自动失效
    QByteArray configJson;     // 图表配置序列化：配置变化时自动失效
    int width = 0;
    int height = 0;

    bool operator==(const ChartCacheKey& o) const {
        return tableId == o.tableId && updatedMsecs == o.updatedMsecs
            && configJson == o.configJson && width == o.width && height == o.height;
    }
};

uint qHash(const ChartCacheKey& k, uint seed)
{
    // 匿名命名空间内定义的 qHash(ChartCacheKey, uint) 会遮蔽全局 Qt 的 qHash 重载集
    // （非 ADL 的普通查找在匿名命名空间即停止），因此对基本字段必须显式 ::qHash 全局限定
    return ::qHash(k.tableId, seed) ^ ::qHash(k.updatedMsecs, seed)
        ^ ::qHash(k.configJson, seed) ^ ::qHash(k.width, seed) ^ ::qHash(k.height, seed);
}

// LRU 缓存：最多保留 64 张渲染结果，超出自动淘汰最久未用
QCache<ChartCacheKey, QPixmap> s_chartCache(64);
} // namespace

QPixmap ObjectRenderer::renderChartCached(const ChartConfig& config,
                                          const DataTable::Ptr& table,
                                          int width, int height)
{
    if (!table) return QPixmap();

    ChartCacheKey key;
    key.tableId = table->id();
    key.updatedMsecs = table->updatedAt().toMSecsSinceEpoch();
    key.configJson = QJsonDocument(config.toJson()).toJson(QJsonDocument::Compact);
    key.width = width;
    key.height = height;

    if (const QPixmap* hit = s_chartCache.object(key)) {
        return *hit;
    }

    ChartRenderer renderer;
    renderer.setConfig(config);
    renderer.setDataTable(table);
    if (!renderer.render()) {
        return QPixmap();
    }
    QPixmap pixmap = renderer.toPixmap(width, height);
    if (!pixmap.isNull()) {
        s_chartCache.insert(key, new QPixmap(pixmap));
    }
    return pixmap;
}
