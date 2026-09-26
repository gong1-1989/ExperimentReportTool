/**
 * @file PrintManager.cpp
 * @brief 打印管理器实现文件
 */

#include "PrintManager.h"
#include "core/utils/Logger.h"
#include "chart/ChartRenderer.h"
#include "chart/ChartConfigDialog.h"
#include "data/repositories/DataTableRepository.h"
#include "core/models/DataTable.h"

#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPageSetupDialog>
#include <QTextDocument>
#include <QTextCursor>
#include <QPainter>
#include <QMessageBox>
#include <QApplication>
#include <QSettings>
#include <QFile>
#include <QBuffer>
#include <QImage>
#include <QPixmap>
#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>

// ===========================================================================
// 构造与析构
// ===========================================================================

PrintManager::PrintManager(QObject* parent)
    : QObject(parent)
{
    // 从设置中加载打印配置
    QSettings settings;
    m_config.pageSize = static_cast<QPageSize::PageSizeId>(
        settings.value("print/pageSize", QPageSize::A4).toInt());
    m_config.orientation = static_cast<QPageLayout::Orientation>(
        settings.value("print/orientation", QPageLayout::Portrait).toInt());
    m_config.printHeader = settings.value("print/printHeader", true).toBool();
    m_config.printFooter = settings.value("print/printFooter", true).toBool();
    m_config.printPageNumbers = settings.value("print/printPageNumbers", true).toBool();
}

PrintManager::~PrintManager()
{
    // 保存打印配置
    QSettings settings;
    settings.setValue("print/pageSize", static_cast<int>(m_config.pageSize));
    settings.setValue("print/orientation", static_cast<int>(m_config.orientation));
    settings.setValue("print/printHeader", m_config.printHeader);
    settings.setValue("print/printFooter", m_config.printFooter);
    settings.setValue("print/printPageNumbers", m_config.printPageNumbers);
}

// ===========================================================================
// 打印预览
// ===========================================================================

bool PrintManager::printPreview(const Report::Ptr& report, QWidget* parent)
{
    if (!report) {
        QMessageBox::critical(parent, tr("打印失败"), tr("报告为空"));
        return false;
    }

    QPrinter printer(QPrinter::HighResolution);
    setupPrinter(printer, m_config);

    QPrintPreviewDialog preview(&printer, parent);
    preview.setWindowTitle(tr("打印预览 - %1").arg(report->title()));
    preview.resize(1000, 700);

    // 连接 paintRequested 信号
    connect(&preview, &QPrintPreviewDialog::paintRequested,
            this, [this, report](QPrinter* printer) {
                QTextDocument* doc = renderDocument(report, m_config);
                if (doc) {
                    doc->print(printer);
                    delete doc;
                }
            });

    return preview.exec() == QDialog::Accepted;
}

// ===========================================================================
// 打印
// ===========================================================================

bool PrintManager::print(const Report::Ptr& report, QWidget* parent)
{
    if (!report) {
        QMessageBox::critical(parent, tr("打印失败"), tr("报告为空"));
        return false;
    }

    QPrinter printer(QPrinter::HighResolution);
    setupPrinter(printer, m_config);

    QPrintDialog dialog(&printer, parent);
    dialog.setWindowTitle(tr("打印 - %1").arg(report->title()));

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    // 从对话框更新配置
    m_config.copies = printer.copyCount();
    m_config.collate = printer.collateCopies();

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QTextDocument* doc = renderDocument(report, m_config);
    if (doc) {
        doc->print(&printer);
        delete doc;
    }
    QApplication::restoreOverrideCursor();

    LOG_INFO(QString("报告已打印: %1").arg(report->title()));
    return true;
}

// ===========================================================================
// 使用指定配置打印
// ===========================================================================

bool PrintManager::printWithConfig(const Report::Ptr& report,
                                     const PrintConfig& config,
                                     QWidget* parent)
{
    if (!report) return false;

    QPrinter printer(QPrinter::HighResolution);
    setupPrinter(printer, config);

    QTextDocument* doc = renderDocument(report, config);
    if (doc) {
        doc->print(&printer);
        delete doc;
        return true;
    }
    return false;
}

// ===========================================================================
// 页面设置
// ===========================================================================

bool PrintManager::pageSetup(PrintConfig& config, QWidget* parent)
{
    QPrinter printer(QPrinter::HighResolution);
    setupPrinter(printer, config);

    QPageSetupDialog dialog(&printer, parent);
    dialog.setWindowTitle(tr("页面设置"));

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    // 从打印机更新配置
    config.pageSize = printer.pageLayout().pageSize().id();
    config.orientation = printer.pageLayout().orientation();
    config.margins = printer.pageLayout().margins(QPageLayout::Millimeter);

    m_config = config;
    return true;
}

// ===========================================================================
// 配置 QPrinter
// ===========================================================================

void PrintManager::setupPrinter(QPrinter& printer, const PrintConfig& config)
{
    printer.setPageSize(QPageSize(config.pageSize));
    printer.setPageOrientation(config.orientation);
    printer.setPageMargins(config.margins, QPageLayout::Millimeter);
    printer.setCopyCount(config.copies);
    printer.setCollateCopies(config.collate);

    if (config.printRange == QPrinter::PageRange) {
        printer.setPrintRange(QPrinter::PageRange);
        printer.setFromTo(config.fromPage, config.toPage);
    } else {
        printer.setPrintRange(QPrinter::AllPages);
    }
}

// ===========================================================================
// 渲染文档
// ===========================================================================

/**
 * @brief 从完整 HTML 文档中提取 body 标签内的内容
 *
 * TextBlockEditor 存储的是 QTextEdit::toHtml() 的完整 HTML 文档，
 * 包含 <!DOCTYPE>、<html>、<head>、<body> 等标签。
 * 在打印/导出时，需要提取 <body> 内的内容嵌入到主文档中，
 * 否则会显示为 HTML 源码。
 *
 * @param html 完整的 HTML 文档
 * @return body 标签内的 HTML 片段，如果不是完整文档则返回原文本
 */
static QString extractHtmlBody(const QString& html)
{
    // 如果不是完整的 HTML 文档（没有 <body 标签），直接返回原文本
    if (!html.contains("<body", Qt::CaseInsensitive)) {
        return html;
    }

    // 查找 <body ...> 开始标签
    const int bodyStart = html.indexOf("<body", 0, Qt::CaseInsensitive);
    if (bodyStart < 0) return html;

    // 查找 body 开始标签的结束位置（> 符号）
    const int bodyTagEnd = html.indexOf('>', bodyStart);
    if (bodyTagEnd < 0) return html;

    // 查找 </body> 结束标签
    const int bodyEnd = html.indexOf("</body>", bodyTagEnd, Qt::CaseInsensitive);
    if (bodyEnd < 0) return html;

    // 提取 body 标签内的内容
    return html.mid(bodyTagEnd + 1, bodyEnd - bodyTagEnd - 1);
}

/**
 * @brief 根据 ID 获取数据表（支持数据库数据表和报告表格块）
 * @param id 数据表 ID（正数=数据库数据表，负数=报告表格块）
 * @param report 报告对象（用于查找表格块）
 * @return 数据表对象，失败返回 nullptr
 */
static DataTable::Ptr getDataTableForPrint(qint64 id, const Report::Ptr& report)
{
    if (id > 0) {
        // 正 ID：从数据库获取数据表
        return DataTableRepository::findById(id);
    } else if (id < 0 && report) {
        // 负 ID：从报告表格块获取
        const int targetIndex = -id - 1;
        int tableBlockIndex = 0;

        for (int i = 0; i < report->blockCount(); ++i) {
            const ContentBlock& block = report->blockAt(i);
            if (block.type == BlockType::Table) {
                if (tableBlockIndex == targetIndex) {
                    // 将表格块转换为 DataTable
                    DataTable::Ptr table = DataTable::create();
                    table->setId(id);
                    table->setName(QObject::tr("表格块 #%1").arg(tableBlockIndex + 1));

                    const int cols = block.data.value("cols").toInt(0);
                    QList<ColumnDefinition> columns;
                    if (block.data.value("headers").isArray()) {
                        const QJsonArray headers = block.data.value("headers").toArray();
                        for (int col = 0; col < cols; ++col) {
                            ColumnDefinition colDef;
                            colDef.name = col < headers.size() ? headers[col].toString() : QString("列%1").arg(col + 1);
                            colDef.type = ColumnType::Text;
                            columns.append(colDef);
                        }
                    } else {
                        for (int col = 0; col < cols; ++col) {
                            ColumnDefinition colDef;
                            colDef.name = QString("列%1").arg(col + 1);
                            colDef.type = ColumnType::Text;
                            columns.append(colDef);
                        }
                    }
                    table->setColumns(columns);

                    if (block.data.value("cells").isArray()) {
                        const QJsonArray cells = block.data.value("cells").toArray();
                        for (int row = 0; row < cells.size(); ++row) {
                            const QJsonArray rowData = cells[row].toArray();
                            QVariantList variantRow;
                            for (int col = 0; col < cols; ++col) {
                                variantRow.append(col < rowData.size() ? rowData[col].toVariant() : QVariant());
                            }
                            table->appendRow(variantRow);
                        }
                    }
                    return table;
                }
                ++tableBlockIndex;
            }
        }
    }
    return nullptr;
}

QTextDocument* PrintManager::renderDocument(const Report::Ptr& report,
                                              const PrintConfig& config)
{
    if (!report) return nullptr;

    // 使用 ExportManager 生成 HTML
    ExportConfig exportConfig;
    exportConfig.includeTitle = config.includeTitle;
    exportConfig.includeMeta = config.includeMeta;
    exportConfig.includeTableOfContents = config.includeTableOfContents;
    exportConfig.fontFamily = "Microsoft YaHei";
    exportConfig.fontSize = 12;

    ExportManager exporter;
    // 调用 reportToHtml（需要通过一个公开方法或直接使用）
    // 由于 reportToHtml 是私有方法，我们这里直接生成 HTML
    // 实际上可以让 ExportManager 暴露一个 toHtml 方法
    // 简化处理：直接构造 QTextDocument

    QTextDocument* doc = new QTextDocument();
    doc->setDefaultFont(QFont("Microsoft YaHei", 11));

    // 构建 HTML 内容
    QString html;
    html += "<html><head><meta charset='utf-8'><style>";
    html += "body { font-family: 'Microsoft YaHei', sans-serif; font-size: 11pt; line-height: 1.8; color: #333; }";
    html += "h1 { font-size: 22pt; text-align: center; color: #1a1a1a; margin-bottom: 20px; }";
    html += "h2 { font-size: 16pt; color: #2a2a2a; margin-top: 24px; border-bottom: 1px solid #ddd; padding-bottom: 4px; }";
    html += "h3 { font-size: 13pt; color: #333; margin-top: 18px; }";
    html += "p { margin: 10px 0; text-align: justify; }";
    html += "ul, ol { margin: 10px 0; padding-left: 28px; }";
    html += "li { margin: 6px 0; }";
    html += "blockquote { border-left: 3px solid #4A90D9; background: #f0f7ff; margin: 14px 0; padding: 10px 16px; color: #555; font-style: italic; }";
    html += "pre { background: #f5f5f5; border: 1px solid #ddd; padding: 12px; border-radius: 4px; font-family: Consolas, monospace; font-size: 9pt; overflow-x: auto; }";
    html += "code { background: #f0f0f0; padding: 1px 4px; border-radius: 2px; font-family: Consolas, monospace; font-size: 0.9em; }";
    html += "hr { border: none; border-top: 1px solid #ddd; margin: 24px 0; }";
    html += ".meta { background: #f8f9fa; padding: 10px 14px; border-radius: 4px; margin-bottom: 20px; font-size: 10pt; color: #666; }";
    html += ".meta span { margin-right: 16px; }";
    html += "img { max-width: 100%; }";
    html += "</style></head><body>";

    // 标题
    if (config.includeTitle) {
        html += QString("<h1>%1</h1>").arg(report->title().toHtmlEscaped());
    }

    // 元信息
    if (config.includeMeta) {
        html += "<div class='meta'>";
        html += QString("<span><strong>作者:</strong> %1</span>").arg(report->author().toHtmlEscaped());
        html += QString("<span><strong>实验日期:</strong> %1</span>").arg(report->experimentDate().toString("yyyy-MM-dd"));
        html += QString("<span><strong>创建时间:</strong> %1</span>").arg(report->createdAt().toString("yyyy-MM-dd hh:mm"));
        html += "</div>";
    }

    // 内容块
    for (int i = 0; i < report->blockCount(); ++i) {
        const ContentBlock& block = report->blockAt(i);
        switch (block.type) {
        case BlockType::Heading1:
            // 文本块存储的是完整 HTML 文档，需要提取 body 内容
            html += QString("<h2>%1</h2>").arg(extractHtmlBody(block.data.value("text").toString()));
            break;
        case BlockType::Heading2:
            html += QString("<h3>%1</h3>").arg(extractHtmlBody(block.data.value("text").toString()));
            break;
        case BlockType::Heading3:
            html += QString("<h3 style='font-size:12pt;'>%1</h3>").arg(extractHtmlBody(block.data.value("text").toString()));
            break;
        case BlockType::Paragraph:
            // 段落直接使用提取后的 HTML 内容（保留格式）
            html += extractHtmlBody(block.data.value("text").toString());
            break;
        case BlockType::BulletList:
            html += "<ul>";
            if (block.data.value("items").isArray()) {
                for (const QJsonValue& item : block.data.value("items").toArray()) {
                    html += QString("<li>%1</li>").arg(item.toString().toHtmlEscaped());
                }
            }
            html += "</ul>";
            break;
        case BlockType::NumberedList:
            html += "<ol>";
            if (block.data.value("items").isArray()) {
                for (const QJsonValue& item : block.data.value("items").toArray()) {
                    html += QString("<li>%1</li>").arg(item.toString().toHtmlEscaped());
                }
            }
            html += "</ol>";
            break;
        case BlockType::Quote:
            html += QString("<blockquote>%1</blockquote>").arg(extractHtmlBody(block.data.value("text").toString()));
            break;
        case BlockType::CodeBlock:
            // 代码块使用纯文本，需要转义
            html += QString("<pre><code>%1</code></pre>").arg(block.data.value("code").toString().toHtmlEscaped());
            break;
        case BlockType::Divider:
            html += "<hr>";
            break;
        case BlockType::Image: {
            const QString imagePath = block.data.value("path").toString();
            const QString caption = block.data.value("caption").toString();
            const int displayWidth = block.data.value("width").toInt(600);

            // 尝试读取图片文件并转换为 base64 嵌入 HTML
            if (!imagePath.isEmpty() && QFile::exists(imagePath)) {
                QImage image(imagePath);
                if (!image.isNull()) {
                    // 按显示宽度缩放
                    if (image.width() > displayWidth) {
                        image = image.scaledToWidth(displayWidth, Qt::SmoothTransformation);
                    }
                    // 转换为 base64
                    QByteArray byteArray;
                    QBuffer buffer(&byteArray);
                    buffer.open(QIODevice::WriteOnly);
                    image.save(&buffer, "PNG");
                    const QString base64 = QString::fromLatin1(byteArray.toBase64());
                    // 嵌入图片
                    html += QString("<div style='text-align:center; margin:10px 0;'>"
                                    "<img src='data:image/png;base64,%1' style='max-width:100%;'/>"
                                    "</div>").arg(base64);
                    // 图片说明
                    if (!caption.isEmpty()) {
                        html += QString("<p style='text-align:center; color:#666; font-size:10pt; margin-top:5px;'>%1</p>")
                                    .arg(caption.toHtmlEscaped());
                    }
                } else {
                    html += QString("<p style='text-align:center; color:#888;'>[图片加载失败: %1]</p>")
                                .arg(imagePath.toHtmlEscaped());
                }
            } else {
                html += "<p style='text-align:center; color:#888;'>[未选择图片]</p>";
            }
            break;
        }
        case BlockType::Table: {
            // 从 JSON 数据渲染 HTML 表格
            const int rows = block.data.value("rows").toInt(0);
            const int cols = block.data.value("cols").toInt(0);

            if (rows > 0 && cols > 0) {
                // 使用最简单的 HTML 表格，确保 QTextDocument 兼容
                html += "<p><table border='1' cellspacing='0' cellpadding='4' width='100%'>";

                // 表头
                if (block.data.value("headers").isArray()) {
                    const QJsonArray headers = block.data.value("headers").toArray();
                    html += "<tr bgcolor='#e8e8e8'>";
                    for (int col = 0; col < cols; ++col) {
                        const QString headerText = col < headers.size()
                                                       ? headers[col].toString()
                                                       : QString("列%1").arg(col + 1);
                        html += QString("<td><b>%1</b></td>").arg(headerText.toHtmlEscaped());
                    }
                    html += "</tr>";
                }

                // 表格数据
                if (block.data.value("cells").isArray()) {
                    const QJsonArray cells = block.data.value("cells").toArray();
                    for (int row = 0; row < rows && row < cells.size(); ++row) {
                        html += "<tr>";
                        const QJsonArray rowData = cells[row].toArray();
                        for (int col = 0; col < cols; ++col) {
                            const QString cellText = col < rowData.size()
                                                         ? rowData[col].toString()
                                                         : QString();
                            html += QString("<td>%1</td>").arg(cellText.toHtmlEscaped());
                        }
                        html += "</tr>";
                    }
                }

                html += "</table></p>";
            } else {
                html += "<p style='text-align:center; color:#888;'>[空表格]</p>";
            }
            break;
        }
        case BlockType::Chart: {
            // 从 JSON 数据解析图表配置
            const ChartConfig config = ChartConfig::fromJson(block.data);

            if (config.dataTableId != 0) {
                // 根据 ID 获取数据表（正 ID=数据库，负 ID=表格块）
                DataTable::Ptr table = getDataTableForPrint(config.dataTableId, report);

                if (table) {
                    // 使用 ChartRenderer 渲染图表
                    ChartRenderer renderer;
                    renderer.setConfig(config);
                    renderer.setDataTable(table);

                    if (renderer.render()) {
                        // 将图表渲染为图片
                        const int chartWidth = config.width > 0 ? config.width : 600;
                        const int chartHeight = config.height > 0 ? config.height : 400;
                        const QPixmap pixmap = renderer.toPixmap(chartWidth, chartHeight);
                        if (!pixmap.isNull()) {
                            // 转换为 base64
                            QByteArray byteArray;
                            QBuffer buffer(&byteArray);
                            buffer.open(QIODevice::WriteOnly);
                            pixmap.save(&buffer, "PNG");
                            const QString base64 = QString::fromLatin1(byteArray.toBase64());
                            // 嵌入图表图片（只设置宽度，高度自适应，避免拉伸）
                            // 注意：QTextDocument 对图片尺寸支持有限，使用宽度属性确保显示正确
                            html += QString("<p align='center'>"
                                            "<img src='data:image/png;base64,%1' width='%2'/>"
                                            "</p>").arg(base64).arg(chartWidth);
                            // 图表标题
                            if (!config.title.isEmpty()) {
                                html += QString("<p align='center' style='color:#666; font-size:10pt;'>%1</p>")
                                            .arg(config.title.toHtmlEscaped());
                            }
                        } else {
                            html += "<p align='center' style='color:#888;'>[图表渲染失败]</p>";
                        }
                    } else {
                        html += "<p align='center' style='color:#888;'>[图表渲染失败]</p>";
                    }
                } else {
                    html += "<p style='text-align:center; color:#888;'>[数据源不存在]</p>";
                }
            } else {
                html += "<p style='text-align:center; color:#888;'>[未配置图表，请先选择数据源]</p>";
            }
            break;
        }
        default:
            break;
        }
    }

    html += "</body></html>";
    doc->setHtml(html);

    return doc;
}

// ===========================================================================
// 页眉页脚（预留，当前通过 QTextDocument 实现）
// ===========================================================================

void PrintManager::printHeaderFooter(QPrinter* printer, QPainter* painter,
                                       int page, int totalPages)
{
    Q_UNUSED(printer);
    Q_UNUSED(painter);
    Q_UNUSED(page);
    Q_UNUSED(totalPages);
    // 页眉页脚功能可以通过 QTextDocument 的页眉页脚框架实现
    // 当前版本简化处理
}
