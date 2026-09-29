/**
 * @file PrintManager.cpp
 * @brief 打印管理器实现文件
 */

#include "PrintManager.h"
#include "export/ObjectRenderer.h"
#include "core/utils/Logger.h"
#include "core/utils/AppDimensions.h"
#include "core/utils/AppTheme.h"
#include "chart/ChartRenderer.h"
#include "chart/ChartConfigDialog.h"
#include "service/DataTableService.h"
#include "core/models/DataTable.h"

#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPageSetupDialog>
#include <QTextDocument>
#include <QTextCursor>
#include <QPainter>
#include <QMessageBox>
#include "ui/UiHelper.h"
#include <QApplication>
#include <QSettings>
#include <QFile>
#include <QBuffer>
#include <QImage>
#include <QPixmap>
#include <QDir>
#include <QDateTime>
#include <QByteArray>
#include <QRegularExpression>
#include <QVector>
#include <QUrl>
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
        UiHelper::error(parent, tr("打印失败"), tr("报告为空"));
        return false;
    }

    LOG_DEBUG("打印预览开始");

    QPrinter printer(QPrinter::HighResolution);
    setupPrinter(printer, m_config);

    QPrintPreviewDialog preview(&printer, parent);
    preview.setWindowTitle(tr("打印预览 - %1").arg(report->title()));
    preview.resize(AppDimensions::Window::PrintPreviewWidth, AppDimensions::Window::PrintPreviewHeight);

    // 收紧预览工具栏图标间距（避免全局 QSS 的 QToolButton padding 撑宽图标间隔）
    preview.setStyleSheet(
        "QPrintPreviewDialog QToolBar { spacing: 2px; padding: 2px 4px; }"
        "QPrintPreviewDialog QToolButton { padding: 2px 5px; }"
        "QPrintPreviewDialog QToolBar::separator { margin: 0 2px; }");

    // 连接 paintRequested 信号
    connect(&preview, &QPrintPreviewDialog::paintRequested,
            this, [this, report](QPrinter* printer) {
                QTextDocument* doc = renderDocument(report, m_config);
                if (doc) {
                    doc->print(printer);
                    delete doc;
                }
            });

    const bool accepted = preview.exec() == QDialog::Accepted;
    cleanupTempChartFiles();
    return accepted;
}

// ===========================================================================
// 打印
// ===========================================================================

bool PrintManager::print(const Report::Ptr& report, QWidget* parent)
{
    if (!report) {
        UiHelper::error(parent, tr("打印失败"), tr("报告为空"));
        return false;
    }

    LOG_DEBUG("打印开始");

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
    cleanupTempChartFiles();

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
        cleanupTempChartFiles();
        return true;
    }
    cleanupTempChartFiles();
    return false;
}

// ===========================================================================
// 导出为 PDF（复用打印预览的渲染逻辑）
// ===========================================================================

bool PrintManager::exportToPdf(const Report::Ptr& report,
                                const QString& filePath,
                                QWidget* parent)
{
    Q_UNUSED(parent);

    if (!report) return false;
    if (filePath.isEmpty()) return false;

    // 确保输出目录存在
    QDir().mkpath(QFileInfo(filePath).absolutePath());

    // 使用与打印预览相同的配置
    PrintConfig config = m_config;
    config.includeTitle = true;
    config.includeMeta = true;

    // 创建 PDF 打印机
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    setupPrinter(printer, config);

    // 复用打印预览的渲染逻辑
    QTextDocument* doc = renderDocument(report, config);
    if (doc) {
        doc->print(&printer);
        delete doc;
        cleanupTempChartFiles();
        return QFile::exists(filePath);
    }
    cleanupTempChartFiles();
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
QString PrintManager::extractHtmlBody(const QString& html)
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
    Q_UNUSED(report);
    if (id > 0) {
        // 正 ID：从数据库获取数据表
        return DataTableService::getById(id);
    }
    return nullptr;
}

void PrintManager::cleanupTempChartFiles()
{
    for (const QString& file : m_tempChartFiles) {
        QFile::remove(file);
    }
    m_tempChartFiles.clear();
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
    html += "h1 { font-size: 22pt; color: #1a1a1a; margin-bottom: 20px; }";   /* 对齐由段落自身决定 */
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
    // 注意：不设置 img { max-width: 100%; }，让 QTextDocument 自动处理图片缩放
    html += "</style></head><body>";

    // 标题（报告主标题固定居中）
    if (config.includeTitle) {
        html += QString("<p align='center' style='text-align:center; font-size:20pt; font-weight:bold;'>%1</p>").arg(report->title().toHtmlEscaped());
    }

    // 元信息
    if (config.includeMeta) {
        html += "<div class='meta'>";
        // QTextDocument 不支持 span 的 margin-right，用显式空格分隔三个信息项
        html += QString("<span><strong>创建者:</strong> %1</span>&nbsp;&nbsp;&nbsp;").arg(report->author().toHtmlEscaped());
        html += QString("<span><strong>实验日期:</strong> %1</span>&nbsp;&nbsp;&nbsp;").arg(report->experimentDate().toString("yyyy-MM-dd"));
        html += QString("<span><strong>创建时间:</strong> %1</span>").arg(report->createdAt().toString("yyyy-MM-dd hh:mm"));
        html += "</div>";
    }

    // 内容：连续文档 + 对象锚点替换
    QString contentHtml = extractHtmlBody(report->document());
    if (contentHtml.isEmpty()) {
        contentHtml = "<p style='color:#888;'>（报告内容为空）</p>";
    }

    // 对象锚点 → 对象渲染 HTML（object://type/id）
    const QRegularExpression anchorRegex(
        "<img[^>]*src=\"object://([^/\"]+)/([^\"]+)\"[^>]*>");
    // 对象锚点段落 → 对齐方式映射（跟随编辑窗段落对齐设置）
    QHash<QString, QString> anchorAligns;
    {
        const QRegularExpression pRe("<p([^>]*)>(.*?)</p>",
                                     QRegularExpression::DotMatchesEverythingOption);
        const QRegularExpression objRe("object://[a-z_0-9]+/([0-9a-fA-F-]+)");
        const QRegularExpression alignRe("align=[\"'](left|center|right|justify)[\"']",
                                         QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator pit = pRe.globalMatch(contentHtml);
        while (pit.hasNext()) {
            const QRegularExpressionMatch pm = pit.next();
            const QRegularExpressionMatch om = objRe.match(pm.captured(2));
            if (!om.hasMatch()) continue;
            QString align = QStringLiteral("left"); // 无 align 属性 = 左对齐
            const QRegularExpressionMatch am = alignRe.match(pm.captured(1));
            if (am.hasMatch()) align = am.captured(1);
            anchorAligns.insert(om.captured(1), align);
        }
    }
    const auto renderObject = [&](const QRegularExpressionMatch& m) -> QString {
        const QString objectId = m.captured(2);
        const QString align = anchorAligns.value(objectId, QStringLiteral("left"));

        // 在报告对象中查找
        ContentBlock object;
        const QList<ContentBlock>& objects = report->objects();
        for (const ContentBlock& obj : objects) {
            if (obj.id == objectId) { object = obj; break; }
        }
        if (object.id.isEmpty()) {
            return QString("<p style='color:#888;'>[对象已不存在]</p>");
        }

        // 统一走共享渲染器（与导出同一套对象渲染逻辑，参数化打印差异）
        // 打印用本地文件/临时文件引用，且数据表查找带旧版内嵌 fallback（getDataTableForPrint）
        return ObjectRenderer::renderObject(
            object, align, ObjectRenderer::printOptions(), report->id(),
            [&](qint64 tid) { return getDataTableForPrint(tid, report); },
            &m_tempChartFiles);
    };
    QRegularExpressionMatchIterator anchorIt = anchorRegex.globalMatch(contentHtml);
    QVector<QPair<int, int> > anchorSpans;
    QVector<QString> anchorHtmls;
    while (anchorIt.hasNext()) {
        const QRegularExpressionMatch m = anchorIt.next();
        anchorSpans.append(qMakePair(m.capturedStart(0), m.capturedLength(0)));
        anchorHtmls.append(renderObject(m));
    }
    for (int ai = anchorSpans.size() - 1; ai >= 0; --ai) {
        contentHtml.replace(anchorSpans.at(ai).first, anchorSpans.at(ai).second,
                            anchorHtmls.at(ai));
    }

    html += contentHtml;
    html += "</body></html>";
    doc->setHtml(html);

    // 设置默认页面大小（A4），确保图片和表格能正确布局
    // 注意：实际打印时会使用 QPrinter 的页面设置
    doc->setPageSize(QSizeF(794, 1123));  // A4 尺寸（像素，96 DPI）

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
