/**
 * @file ExportManager.cpp
 * @brief 导出管理器实现文件
 */

#include "ExportManager.h"
#include "core/utils/Logger.h"

#include <QTextDocument>
#include <QTextCursor>
#include <QPrinter>
#include <QPrintDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QTextList>
#include <QTextTable>
#include <QBuffer>
#include <QImage>
#include <QPixmap>
#include <QRegularExpression>
#include <QXmlStreamWriter>
#include <QMimeDatabase>
#include <QMimeType>

// ===========================================================================
// 构造与析构
// ===========================================================================

ExportManager::ExportManager()
{
}

ExportManager::~ExportManager()
{
}

// ===========================================================================
// 导出入口
// ===========================================================================

bool ExportManager::exportReport(const Report::Ptr& report,
                                  const ExportConfig& config,
                                  QWidget* parent)
{
    if (!report) {
        QMessageBox::critical(parent, QObject::tr("导出失败"), QObject::tr("报告为空"));
        return false;
    }

    if (config.filePath.isEmpty()) {
        QMessageBox::critical(parent, QObject::tr("导出失败"), QObject::tr("输出路径为空"));
        return false;
    }

    // 确保输出目录存在
    QDir().mkpath(QFileInfo(config.filePath).absolutePath());

    bool success = false;
    switch (config.format) {
    case ExportFormat::Pdf:
        success = exportToPdf(report, config, parent);
        break;
    case ExportFormat::Html:
        success = exportToHtml(report, config, parent);
        break;
    case ExportFormat::Word:
        success = exportToWord(report, config, parent);
        break;
    case ExportFormat::Text:
        success = exportToText(report, config, parent);
        break;
    }

    if (success) {
        LOG_INFO(QString("报告已导出: %1").arg(config.filePath));
    } else {
        LOG_ERROR(QString("报告导出失败: %1").arg(config.filePath));
    }

    return success;
}

bool ExportManager::exportReport(const Report::Ptr& report,
                                  ExportFormat format,
                                  const QString& filePath,
                                  QWidget* parent)
{
    ExportConfig config;
    config.format = format;
    config.filePath = filePath;
    return exportReport(report, config, parent);
}

// ===========================================================================
// PDF 导出
// ===========================================================================

bool ExportManager::exportToPdf(const Report::Ptr& report,
                                  const ExportConfig& config,
                                  QWidget* parent)
{
    Q_UNUSED(parent);

    // 生成 HTML
    const QString html = reportToHtml(report, config);

    // 使用 QTextDocument 渲染 HTML 并打印到 PDF
    QTextDocument doc;
    doc.setHtml(html);
    doc.setDefaultFont(QFont(config.fontFamily, config.fontSize));

    // 设置页面大小
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(config.filePath);

    if (config.pageSize == "A4") {
        printer.setPageSize(QPageSize(QPageSize::A4));
    } else if (config.pageSize == "Letter") {
        printer.setPageSize(QPageSize(QPageSize::Letter));
    } else {
        printer.setPageSize(QPageSize(QPageSize::A4));
    }

    printer.setPageMargins(QMarginsF(20, 20, 20, 20), QPageLayout::Millimeter);

    // 打印文档
    doc.print(&printer);

    return QFile::exists(config.filePath);
}

// ===========================================================================
// HTML 导出
// ===========================================================================

bool ExportManager::exportToHtml(const Report::Ptr& report,
                                   const ExportConfig& config,
                                   QWidget* parent)
{
    Q_UNUSED(parent);

    const QString html = reportToHtml(report, config);

    QFile file(config.filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream << html;
    file.close();

    return true;
}

// ===========================================================================
// Word 导出（基于 HTML 的 .docx 简化实现）
// ===========================================================================

bool ExportManager::exportToWord(const Report::Ptr& report,
                                   const ExportConfig& config,
                                   QWidget* parent)
{
    Q_UNUSED(parent);

    // 简化实现：生成 Word 可以打开的 HTML 文件，扩展名用 .doc
    // 完整的 .docx 需要 OOXML 格式，后续可以用 libdocx 或 pandoc
    const QString html = reportToHtml(report, config);

    // 包装为 Word 兼容的 HTML（添加 MSO 命名空间）
    const QString wordHtml = QString(
        "<html xmlns:o=\"urn:schemas-microsoft-com:office:office\" "
        "xmlns:w=\"urn:schemas-microsoft-com:office:word\" "
        "xmlns=\"http://www.w3.org/TR/REC-html40\">"
        "<head><meta charset=\"utf-8\">"
        "<!--[if gte mso 9]><xml><w:WordDocument>"
        "<w:View>Print</w:View><w:Zoom>100</w:Zoom>"
        "</w:WordDocument></xml><![endif]-->"
        "</head><body>%1</body></html>"
    ).arg(html.section("<body>", 1).section("</body>", 0, 0));

    QFile file(config.filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream << wordHtml;
    file.close();

    return true;
}

// ===========================================================================
// 纯文本导出
// ===========================================================================

bool ExportManager::exportToText(const Report::Ptr& report,
                                   const ExportConfig& config,
                                   QWidget* parent)
{
    Q_UNUSED(parent);

    QString text;

    if (config.includeTitle) {
        text += report->title() + "\n";
        text += QString(report->title().length(), '=') + "\n\n";
    }

    if (config.includeMeta) {
        text += QString("作者: %1\n").arg(report->author());
        text += QString("实验日期: %1\n").arg(report->experimentDate().toString("yyyy-MM-dd"));
        text += QString("创建时间: %1\n\n").arg(report->createdAt().toString("yyyy-MM-dd hh:mm"));
    }

    // 遍历内容块
    for (int i = 0; i < report->blockCount(); ++i) {
        const ContentBlock& block = report->blockAt(i);
        switch (block.type) {
        case BlockType::Heading1:
            text += "\n# " + block.data.value("text").toString() + "\n\n";
            break;
        case BlockType::Heading2:
            text += "\n## " + block.data.value("text").toString() + "\n\n";
            break;
        case BlockType::Heading3:
            text += "\n### " + block.data.value("text").toString() + "\n\n";
            break;
        case BlockType::Paragraph:
            text += block.data.value("text").toString() + "\n\n";
            break;
        case BlockType::BulletList:
            if (block.data.value("items").isArray()) {
                for (const QJsonValue& item : block.data.value("items").toArray()) {
                    text += "- " + item.toString() + "\n";
                }
                text += "\n";
            }
            break;
        case BlockType::NumberedList:
            if (block.data.value("items").isArray()) {
                int num = 1;
                for (const QJsonValue& item : block.data.value("items").toArray()) {
                    text += QString("%1. %2\n").arg(num++).arg(item.toString());
                }
                text += "\n";
            }
            break;
        case BlockType::Quote:
            text += "> " + block.data.value("text").toString() + "\n\n";
            break;
        case BlockType::CodeBlock:
            text += "```\n" + block.data.value("code").toString() + "\n```\n\n";
            break;
        case BlockType::Divider:
            text += "\n" + QString(50, '-') + "\n\n";
            break;
        case BlockType::Image:
            text += QString("[图片: %1]\n\n").arg(block.data.value("caption").toString());
            break;
        case BlockType::Table:
            text += "[表格]\n\n";
            break;
        case BlockType::Chart:
            text += "[图表]\n\n";
            break;
        case BlockType::Formula:
        case BlockType::DataReference:
            break;
        }
    }

    QFile file(config.filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream << text;
    file.close();

    return true;
}

// ===========================================================================
// 报告转 HTML
// ===========================================================================

QString ExportManager::reportToHtml(const Report::Ptr& report, const ExportConfig& config)
{
    QString html;
    html += "<!DOCTYPE html>\n<html>\n<head>\n";
    html += "<meta charset=\"utf-8\">\n";
    html += QString("<title>%1</title>\n").arg(report->title().toHtmlEscaped());
    html += "<style>\n" + generateCss(config) + "\n</style>\n";
    html += "</head>\n<body>\n";

    // 标题
    if (config.includeTitle) {
        html += QString("<h1 class=\"report-title\">%1</h1>\n")
                    .arg(report->title().toHtmlEscaped());
    }

    // 元信息
    if (config.includeMeta) {
        html += "<div class=\"report-meta\">\n";
        html += QString("<span class=\"meta-item\"><strong>作者:</strong> %1</span>\n")
                    .arg(report->author().toHtmlEscaped());
        html += QString("<span class=\"meta-item\"><strong>实验日期:</strong> %1</span>\n")
                    .arg(report->experimentDate().toString("yyyy-MM-dd"));
        html += QString("<span class=\"meta-item\"><strong>创建时间:</strong> %1</span>\n")
                    .arg(report->createdAt().toString("yyyy-MM-dd hh:mm"));
        html += "</div>\n";
    }

    // 目录
    if (config.includeTableOfContents) {
        html += "<div class=\"table-of-contents\">\n";
        html += "<h2>目录</h2>\n<ul>\n";
        int tocIndex = 1;
        for (int i = 0; i < report->blockCount(); ++i) {
            const ContentBlock& block = report->blockAt(i);
            if (block.type == BlockType::Heading1 || block.type == BlockType::Heading2) {
                const QString text = block.data.value("text").toString();
                const QString indent = block.type == BlockType::Heading2 ? "  " : "";
                html += QString("%1<li><a href=\"#heading-%2\">%3</a></li>\n")
                            .arg(indent).arg(tocIndex).arg(text.toHtmlEscaped());
                ++tocIndex;
            }
        }
        html += "</ul>\n</div>\n";
    }

    // 正文内容
    html += "<div class=\"report-content\">\n";
    int headingCounter = 0;
    for (int i = 0; i < report->blockCount(); ++i) {
        html += blockToHtml(report->blockAt(i), headingCounter);
    }
    html += "</div>\n";

    // 页脚
    html += QString("<div class=\"report-footer\">由 %1 生成于 %2</div>\n")
                .arg("实验报告记录工具")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

    html += "</body>\n</html>";
    return html;
}

// ===========================================================================
// 内容块转 HTML
// ===========================================================================

QString ExportManager::blockToHtml(const ContentBlock& block, int& headingCounter)
{
    switch (block.type) {
    case BlockType::Heading1: {
        ++headingCounter;
        return QString("<h1 id=\"heading-%1\">%2</h1>\n")
            .arg(headingCounter)
            .arg(block.data.value("text").toString().toHtmlEscaped());
    }
    case BlockType::Heading2: {
        ++headingCounter;
        return QString("<h2 id=\"heading-%1\">%2</h2>\n")
            .arg(headingCounter)
            .arg(block.data.value("text").toString().toHtmlEscaped());
    }
    case BlockType::Heading3:
        return QString("<h3>%1</h3>\n")
            .arg(block.data.value("text").toString().toHtmlEscaped());

    case BlockType::Paragraph:
        return QString("<p>%1</p>\n")
            .arg(block.data.value("text").toString().toHtmlEscaped());

    case BlockType::BulletList: {
        QString html = "<ul>\n";
        if (block.data.value("items").isArray()) {
            for (const QJsonValue& item : block.data.value("items").toArray()) {
                html += QString("<li>%1</li>\n").arg(item.toString().toHtmlEscaped());
            }
        }
        html += "</ul>\n";
        return html;
    }

    case BlockType::NumberedList: {
        QString html = "<ol>\n";
        if (block.data.value("items").isArray()) {
            for (const QJsonValue& item : block.data.value("items").toArray()) {
                html += QString("<li>%1</li>\n").arg(item.toString().toHtmlEscaped());
            }
        }
        html += "</ol>\n";
        return html;
    }

    case BlockType::Quote:
        return QString("<blockquote>%1</blockquote>\n")
            .arg(block.data.value("text").toString().toHtmlEscaped());

    case BlockType::CodeBlock: {
        const QString code = block.data.value("code").toString().toHtmlEscaped();
        const QString lang = block.data.value("language").toString();
        return QString("<pre><code class=\"language-%1\">%2</code></pre>\n")
            .arg(lang).arg(code);
    }

    case BlockType::Divider:
        return "<hr>\n";

    case BlockType::Image: {
        const QString path = block.data.value("path").toString();
        const QString caption = block.data.value("caption").toString();
        QString html = "<div class=\"image-block\">\n";
        if (!path.isEmpty() && QFile::exists(path)) {
            // 将图片转为 base64 嵌入 HTML
            QFile imgFile(path);
            if (imgFile.open(QIODevice::ReadOnly)) {
                const QByteArray data = imgFile.readAll();
                const QString base64 = QString::fromLatin1(data.toBase64());
                const QString mime = QMimeDatabase().mimeTypeForFile(path).name();
                html += QString("<img src=\"data:%1;base64,%2\" alt=\"%3\">\n")
                           .arg(mime).arg(base64).arg(caption.toHtmlEscaped());
                imgFile.close();
            }
        } else {
            html += QString("<div class=\"image-placeholder\">[图片: %1]</div>\n")
                       .arg(caption.toHtmlEscaped());
        }
        if (!caption.isEmpty()) {
            html += QString("<p class=\"image-caption\">%1</p>\n").arg(caption.toHtmlEscaped());
        }
        html += "</div>\n";
        return html;
    }

    case BlockType::Table:
        // 表格块引用 data_tables 表，导出时需要查询数据库
        // 简化处理：显示占位
        return "<div class=\"table-block\">[表格数据]</div>\n";

    case BlockType::Chart:
        return "<div class=\"chart-block\">[图表]</div>\n";

    case BlockType::Formula:
        return QString("<div class=\"formula\">%1</div>\n")
            .arg(block.data.value("latex").toString().toHtmlEscaped());

    case BlockType::DataReference:
        return "<div class=\"data-reference\">[数据引用]</div>\n";
    }

    return QString();
}

// ===========================================================================
// CSS 样式生成
// ===========================================================================

QString ExportManager::generateCss(const ExportConfig& config)
{
    return QString(R"(
        body {
            font-family: "%1", sans-serif;
            font-size: %2px;
            line-height: 1.8;
            color: #333;
            max-width: 800px;
            margin: 0 auto;
            padding: 40px;
        }
        .report-title {
            text-align: center;
            font-size: 28px;
            color: #1a1a1a;
            border-bottom: 2px solid #4A90D9;
            padding-bottom: 16px;
            margin-bottom: 24px;
        }
        .report-meta {
            background: #f8f9fa;
            padding: 12px 16px;
            border-radius: 6px;
            margin-bottom: 24px;
            font-size: 13px;
            color: #666;
        }
        .report-meta .meta-item {
            display: inline-block;
            margin-right: 20px;
        }
        .table-of-contents {
            background: #f8f9fa;
            padding: 16px 24px;
            border-radius: 6px;
            margin-bottom: 24px;
        }
        .table-of-contents h2 {
            font-size: 18px;
            margin-top: 0;
        }
        .table-of-contents ul {
            list-style: none;
            padding-left: 0;
        }
        .table-of-contents li {
            padding: 4px 0;
        }
        .table-of-contents a {
            color: #4A90D9;
            text-decoration: none;
        }
        h1 { font-size: 24px; color: #1a1a1a; margin-top: 32px; border-bottom: 1px solid #eee; padding-bottom: 8px; }
        h2 { font-size: 20px; color: #2a2a2a; margin-top: 24px; }
        h3 { font-size: 17px; color: #333; margin-top: 20px; }
        p { margin: 12px 0; text-align: justify; }
        ul, ol { margin: 12px 0; padding-left: 28px; }
        li { margin: 6px 0; }
        blockquote {
            border-left: 4px solid #4A90D9;
            background: #f0f7ff;
            margin: 16px 0;
            padding: 12px 20px;
            color: #555;
            font-style: italic;
        }
        pre {
            background: #1e1e1e;
            color: #d4d4d4;
            padding: 16px;
            border-radius: 6px;
            overflow-x: auto;
            font-family: Consolas, Monaco, monospace;
            font-size: 13px;
            line-height: 1.5;
        }
        code {
            background: #f0f0f0;
            padding: 2px 6px;
            border-radius: 3px;
            font-family: Consolas, Monaco, monospace;
            font-size: 0.9em;
        }
        pre code {
            background: none;
            padding: 0;
        }
        hr {
            border: none;
            border-top: 1px solid #ddd;
            margin: 32px 0;
        }
        .image-block {
            text-align: center;
            margin: 20px 0;
        }
        .image-block img {
            max-width: 100%;
            border-radius: 4px;
            box-shadow: 0 2px 8px rgba(0,0,0,0.1);
        }
        .image-caption {
            font-size: 13px;
            color: #888;
            margin-top: 8px;
        }
        .table-block, .chart-block {
            background: #f8f9fa;
            border: 1px dashed #ddd;
            padding: 24px;
            text-align: center;
            color: #999;
            border-radius: 6px;
            margin: 16px 0;
        }
        .report-footer {
            margin-top: 48px;
            padding-top: 16px;
            border-top: 1px solid #eee;
            text-align: center;
            font-size: 12px;
            color: #aaa;
        }
        @media print {
            body { max-width: none; padding: 0; }
            .report-footer { display: none; }
        }
    )").arg(config.fontFamily).arg(config.fontSize);
}

// ===========================================================================
// 格式工具方法
// ===========================================================================

QString ExportManager::formatFilter(ExportFormat format)
{
    switch (format) {
    case ExportFormat::Pdf:  return QObject::tr("PDF 文件 (*.pdf)");
    case ExportFormat::Html: return QObject::tr("HTML 文件 (*.html *.htm)");
    case ExportFormat::Word: return QObject::tr("Word 文档 (*.doc)");
    case ExportFormat::Text: return QObject::tr("纯文本文件 (*.txt)");
    }
    return QObject::tr("所有文件 (*)");
}

QString ExportManager::formatExtension(ExportFormat format)
{
    switch (format) {
    case ExportFormat::Pdf:  return ".pdf";
    case ExportFormat::Html: return ".html";
    case ExportFormat::Word: return ".doc";
    case ExportFormat::Text: return ".txt";
    }
    return ".txt";
}

QList<ExportFormat> ExportManager::supportedFormats()
{
    return { ExportFormat::Pdf, ExportFormat::Html, ExportFormat::Word, ExportFormat::Text };
}

QString ExportManager::formatDisplayName(ExportFormat format)
{
    switch (format) {
    case ExportFormat::Pdf:  return QObject::tr("PDF");
    case ExportFormat::Html: return QObject::tr("HTML");
    case ExportFormat::Word: return QObject::tr("Word");
    case ExportFormat::Text: return QObject::tr("纯文本");
    }
    return QString();
}

QPair<QString, ExportFormat> ExportManager::getSaveFilePath(QWidget* parent,
                                                                const QString& defaultName)
{
    // 构建过滤器
    QStringList filters;
    for (ExportFormat fmt : supportedFormats()) {
        filters.append(formatFilter(fmt));
    }
    const QString filter = filters.join(";;");

    const QString filePath = QFileDialog::getSaveFileName(
        parent, QObject::tr("导出报告"), defaultName, filter);

    if (filePath.isEmpty()) {
        return qMakePair(QString(), ExportFormat::Pdf);
    }

    // 根据扩展名判断格式
    const QString ext = QFileInfo(filePath).suffix().toLower();
    ExportFormat format = ExportFormat::Pdf;
    if (ext == "html" || ext == "htm") format = ExportFormat::Html;
    else if (ext == "doc" || ext == "docx") format = ExportFormat::Word;
    else if (ext == "txt") format = ExportFormat::Text;

    return qMakePair(filePath, format);
}
