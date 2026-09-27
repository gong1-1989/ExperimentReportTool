/**
 * @file ExportManager.cpp
 * @brief 导出管理器实现文件
 */

#include "ExportManager.h"
#include "core/utils/Logger.h"
#include "core/utils/AppConfig.h"
#include "chart/ChartRenderer.h"
#include "chart/ChartConfigDialog.h"
#include "data/repositories/DataTableRepository.h"
#include "core/models/DataTable.h"
#include "core/plugin/PluginManager.h"
#include "core/plugin/EditorBlockPluginInterface.h"
#include "print/PrintManager.h"

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
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>

// ===========================================================================
// 静态成员初始化
// ===========================================================================

PluginManager* ExportManager::s_pluginManager = nullptr;

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
// 插件渲染支持
// ===========================================================================

void ExportManager::setPluginManager(PluginManager* manager)
{
    s_pluginManager = manager;
}

EditorBlockPluginInterface* ExportManager::findBlockPlugin(BlockType type)
{
    if (!s_pluginManager) return nullptr;

    // 将 BlockType 转换为字符串标识
    QString typeStr;
    switch (type) {
        case BlockType::Paragraph:    typeStr = "paragraph"; break;
        case BlockType::Heading1:     typeStr = "heading1"; break;
        case BlockType::Heading2:     typeStr = "heading2"; break;
        case BlockType::Heading3:     typeStr = "heading3"; break;
        case BlockType::BulletList:   typeStr = "bullet_list"; break;
        case BlockType::NumberedList: typeStr = "numbered_list"; break;
        case BlockType::Quote:        typeStr = "quote"; break;
        case BlockType::Table:        typeStr = "table"; break;
        case BlockType::Image:        typeStr = "image"; break;
        case BlockType::CodeBlock:    typeStr = "code"; break;
        case BlockType::Divider:      typeStr = "divider"; break;
        case BlockType::Chart:        typeStr = "chart"; break;
        case BlockType::Formula:      typeStr = "formula"; break;
        default:                      return nullptr;
    }

    // 遍历所有插件，查找匹配的编辑器块插件
    const QList<PluginInterface*> plugins = s_pluginManager->loadedPlugins();
    for (PluginInterface* p : plugins) {
        auto* editorPlugin = dynamic_cast<EditorBlockPluginInterface*>(p);
        if (!editorPlugin) continue;

        const QString pluginType = editorPlugin->blockType();

        // 精确匹配
        if (pluginType == typeStr) {
            return editorPlugin;
        }

        // 文本类型通配：TextBlockPlugin 的 blockType 为 "paragraph"，
        // 但它处理所有文本类型（标题、段落、列表、引用）
        if (pluginType == "paragraph" &&
            (type == BlockType::Paragraph ||
             type == BlockType::Heading1 ||
             type == BlockType::Heading2 ||
             type == BlockType::Heading3 ||
             type == BlockType::BulletList ||
             type == BlockType::NumberedList ||
             type == BlockType::Quote)) {
            return editorPlugin;
        }
    }
    return nullptr;
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
    // 直接复用打印预览的渲染逻辑，确保 PDF 与打印预览显示一致
    PrintManager printManager;
    PrintConfig printConfig = printManager.currentConfig();
    printConfig.includeTitle = config.includeTitle;
    printConfig.includeMeta = config.includeMeta;
    printConfig.includeTableOfContents = config.includeTableOfContents;
    printManager.setConfig(printConfig);

    return printManager.exportToPdf(report, config.filePath, parent);
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
    // Qt6 中 QTextStream 默认使用 UTF-8 编码，无需调用 setCodec
    stream << html;
    file.close();

    return true;
}

// ===========================================================================
// Word 导出（基于 HTML 的 .docx 简化实现）
// ===========================================================================

/**
 * @brief 将 HTML 中的 base64 图片保存到指定目录，并用相对路径替换
 *
 * 用于 Word 导出：Word 对 base64 data: URI 支持有限，
 * 需要将图片保存为本地文件，用相对路径引用。
 *
 * @param html 原始 HTML
 * @param outputDir 输出目录（图片保存到 outputDir/images/）
 * @return 转换后的 HTML
 */
static QString convertBase64ImagesToRelativeFiles(const QString& html, const QString& outputDir)
{
    QString result = html;
    const QString imagesDir = QDir(outputDir).filePath("images");
    QDir().mkpath(imagesDir);

    QRegularExpression regex("src=\"data:([^;]+);base64,([^\"]+)\"");
    QRegularExpressionMatchIterator it = regex.globalMatch(html);
    int imageIndex = 0;

    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QString mimeType = match.captured(1);
        const QString base64Data = match.captured(2);

        // 确定文件扩展名
        QString ext = "png";
        if (mimeType.contains("jpeg") || mimeType.contains("jpg")) ext = "jpg";
        else if (mimeType.contains("gif")) ext = "gif";
        else if (mimeType.contains("bmp")) ext = "bmp";

        // 保存图片文件
        const QString fileName = QString("image_%1.%2").arg(imageIndex++).arg(ext);
        const QString filePath = QDir(imagesDir).filePath(fileName);

        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QByteArray::fromBase64(base64Data.toLatin1()));
            file.close();
        }

        // 用相对路径替换（Word 支持相对路径）
        const QString relativePath = QString("images/%1").arg(fileName);
        result.replace(match.captured(0), QString("src=\"%1\"").arg(relativePath));
    }

    return result;
}

bool ExportManager::exportToWord(const Report::Ptr& report,
                                   const ExportConfig& config,
                                   QWidget* parent)
{
    Q_UNUSED(parent);

    // 简化实现：生成 Word 可以打开的 HTML 文件，扩展名用 .doc
    // 完整的 .docx 需要 OOXML 格式，后续可以用 libdocx 或 pandoc
    QString html = reportToHtml(report, config);

    // Word 对 base64 图片支持有限，将图片保存到同目录的 images/ 文件夹
    const QString outputDir = QFileInfo(config.filePath).absolutePath();
    html = convertBase64ImagesToRelativeFiles(html, outputDir);

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
    // Qt6 中 QTextStream 默认使用 UTF-8 编码，无需调用 setCodec
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
    // Qt6 中 QTextStream 默认使用 UTF-8 编码，无需调用 setCodec
    stream << text;
    file.close();

    return true;
}

// ===========================================================================
// 报告转 HTML
// ===========================================================================

/**
 * @brief 从完整 HTML 文档中提取 body 标签内的内容
 *
 * TextBlockEditor 存储的是 QTextEdit::toHtml() 的完整 HTML 文档，
 * 包含 <!DOCTYPE>、<html>、<head>、<body> 等标签。
 * 在导出时，需要提取 <body> 内的内容嵌入到主文档中，
 * 否则会显示为 HTML 源码。
 *
 * @param html 完整的 HTML 文档
 * @return body 标签内的 HTML 片段，如果不是完整文档则返回原文本
 */
static QString extractHtmlBody(const QString& html)
{
    if (html.isEmpty()) return QString();

    // 如果不包含 <body 标签，说明是 HTML 片段，直接返回
    if (!html.contains("<body", Qt::CaseInsensitive)) {
        return html;
    }

    // 查找 <body 开始位置
    const int bodyStart = html.indexOf("<body", 0, Qt::CaseInsensitive);
    if (bodyStart < 0) return html;

    // 查找 <body> 标签结束位置（> 字符）
    const int bodyTagEnd = html.indexOf('>', bodyStart);
    if (bodyTagEnd < 0) return html;

    // 查找 </body> 位置
    int bodyEnd = html.indexOf("</body>", bodyTagEnd, Qt::CaseInsensitive);
    if (bodyEnd < 0) {
        // 没有 </body>，取到文档末尾
        bodyEnd = html.size();
    }

    QString result = html.mid(bodyTagEnd + 1, bodyEnd - bodyTagEnd - 1).trimmed();

    // 如果提取结果为空，尝试用 QTextDocument 解析整个 HTML 并提取 body 内容
    if (result.isEmpty()) {
        QTextDocument doc;
        doc.setHtml(html);
        result = doc.toHtml();
        // 再次提取 body
        if (result.contains("<body", Qt::CaseInsensitive)) {
            const int s = result.indexOf("<body", 0, Qt::CaseInsensitive);
            const int e = result.indexOf('>', s);
            const int end = result.indexOf("</body>", e, Qt::CaseInsensitive);
            if (e >= 0 && end > e) {
                result = result.mid(e + 1, end - e - 1).trimmed();
            }
        }
    }

    return result;
}

/**
 * @brief 从 HTML 文档中提取纯文本（用于目录等需要纯文本的场景）
 *
 * 使用 QTextDocument 解析 HTML 并提取纯文本内容。
 *
 * @param html HTML 文档或片段
 * @return 纯文本内容
 */
static QString htmlToPlainText(const QString& html)
{
    QTextDocument doc;
    doc.setHtml(html);
    return doc.toPlainText().trimmed();
}

/**
 * @brief 安全获取文本块的 HTML 内容
 *
 * 优先使用 text 字段（完整 HTML），提取 body 内容。
 * 如果提取结果为空，降级使用 plain_text 字段。
 *
 * @param block 内容块
 * @return HTML 片段
 */
static QString getBlockTextHtml(const ContentBlock& block)
{
    const QString textHtml = block.data.value("text").toString();
    QString result = extractHtmlBody(textHtml);

    // 如果 HTML 提取结果为空，尝试用 plain_text 兜底
    if (result.isEmpty() || result == "<p></p>" || result == "<p><br></p>") {
        const QString plainText = block.data.value("plain_text").toString();
        if (!plainText.isEmpty()) {
            result = QString("<p>%1</p>").arg(plainText.toHtmlEscaped());
        }
    }

    return result;
}

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
                // 目录需要纯文本标题，从 HTML 中提取纯文本
                const QString text = htmlToPlainText(block.data.value("text").toString());
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
        html += blockToHtml(report->blockAt(i), headingCounter, report);
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

QString ExportManager::blockToHtml(const ContentBlock& block, int& headingCounter, const Report::Ptr& report)
{
    // ========================================================================
    // 第一步：优先通过插件渲染（消除重复逻辑）
    // 插件返回非空字符串表示渲染成功，空字符串表示由框架兜底
    // ========================================================================
    EditorBlockPluginInterface* plugin = findBlockPlugin(block.type);
    if (plugin) {
        const QString pluginHtml = plugin->renderToHtml(block, report.data());
        if (!pluginHtml.isEmpty()) {
            // 标题块需要更新目录计数器
            if (block.type == BlockType::Heading1 || block.type == BlockType::Heading2) {
                ++headingCounter;
            }
            return pluginHtml + "\n";
        }
    }

    // ========================================================================
    // 第二步：插件未提供渲染时，使用框架内置实现（兜底）
    // ========================================================================
    switch (block.type) {
    case BlockType::Heading1: {
        ++headingCounter;
        return QString("<h1 id=\"heading-%1\">%2</h1>\n")
            .arg(headingCounter)
            .arg(getBlockTextHtml(block));
    }
    case BlockType::Heading2: {
        ++headingCounter;
        return QString("<h2 id=\"heading-%1\">%2</h2>\n")
            .arg(headingCounter)
            .arg(getBlockTextHtml(block));
    }
    case BlockType::Heading3:
        return QString("<h3>%1</h3>\n")
            .arg(getBlockTextHtml(block));

    case BlockType::Paragraph:
        // 段落直接使用提取后的 HTML 内容（保留格式）
        return getBlockTextHtml(block) + "\n";

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
            .arg(getBlockTextHtml(block));

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

    case BlockType::Table: {
        // 从 JSON 数据渲染 HTML 表格
        // 注意：QTextDocument 对 HTML 表格支持有限，只用最基本的属性
        const int rows = block.data.value("rows").toInt(0);
        const int cols = block.data.value("cols").toInt(0);

        if (rows > 0 && cols > 0) {
            // 用最简单的表格标签，确保 QTextDocument 能正确渲染
            QString html = "<table border=\"1\" width=\"100%\" cellpadding=\"4\" cellspacing=\"0\">\n";

            // 表头
            if (block.data.value("headers").isArray()) {
                const QJsonArray headers = block.data.value("headers").toArray();
                html += "<tr>\n";
                for (int col = 0; col < cols; ++col) {
                    const QString headerText = col < headers.size()
                                                   ? headers[col].toString()
                                                   : QString("列%1").arg(col + 1);
                    html += QString("<th bgcolor=\"#f0f0f0\"><b>%1</b></th>\n")
                               .arg(headerText.toHtmlEscaped());
                }
                html += "</tr>\n";
            }

            // 表格数据
            if (block.data.value("cells").isArray()) {
                const QJsonArray cells = block.data.value("cells").toArray();
                for (int row = 0; row < rows && row < cells.size(); ++row) {
                    html += "<tr>\n";
                    const QJsonArray rowData = cells[row].toArray();
                    for (int col = 0; col < cols; ++col) {
                        const QString cellText = col < rowData.size()
                                                     ? rowData[col].toString()
                                                     : QString();
                        html += QString("<td>%1</td>\n").arg(cellText.toHtmlEscaped());
                    }
                    html += "</tr>\n";
                }
            }

            html += "</table>\n";
            html += "<br>\n";
            return html;
        }
        return "<p>[空表格]</p>\n";
    }

    case BlockType::Chart: {
        // 从 JSON 数据解析图表配置
        const ChartConfig config = ChartConfig::fromJson(block.data);

        if (config.dataTableId != 0) {
            DataTable::Ptr table;

            if (config.dataTableId > 0) {
                // 正 ID：从数据库获取数据表
                table = DataTableRepository::findById(config.dataTableId);
            } else if (report) {
                // 负 ID：从报告表格块获取
                const int targetIndex = -config.dataTableId - 1;
                int tableBlockIndex = 0;

                for (int i = 0; i < report->blockCount(); ++i) {
                    const ContentBlock& tableBlock = report->blockAt(i);
                    if (tableBlock.type == BlockType::Table) {
                        if (tableBlockIndex == targetIndex) {
                            // 将表格块转换为 DataTable
                            table = DataTable::create();
                            table->setId(config.dataTableId);
                            table->setName(QString("表格块 #%1").arg(tableBlockIndex + 1));

                            const int cols = tableBlock.data.value("cols").toInt(0);
                            QList<ColumnDefinition> columns;
                            if (tableBlock.data.value("headers").isArray()) {
                                const QJsonArray headers = tableBlock.data.value("headers").toArray();
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

                            if (tableBlock.data.value("cells").isArray()) {
                                const QJsonArray cells = tableBlock.data.value("cells").toArray();
                                for (int row = 0; row < cells.size(); ++row) {
                                    const QJsonArray rowData = cells[row].toArray();
                                    QVariantList variantRow;
                                    for (int col = 0; col < cols; ++col) {
                                        variantRow.append(col < rowData.size() ? rowData[col].toVariant() : QVariant());
                                    }
                                    table->appendRow(variantRow);
                                }
                            }
                            break;
                        }
                        ++tableBlockIndex;
                    }
                }
            }

            if (table) {
                // 使用 ChartRenderer 渲染图表
                ChartRenderer renderer;
                renderer.setConfig(config);
                renderer.setDataTable(table);

                if (renderer.render()) {
                    // 将图表渲染为图片
                    const QPixmap pixmap = renderer.toPixmap(config.width, config.height);
                    if (!pixmap.isNull()) {
                        // 转换为 base64
                        QByteArray byteArray;
                        QBuffer buffer(&byteArray);
                        buffer.open(QIODevice::WriteOnly);
                        pixmap.save(&buffer, "PNG");
                        const QString base64 = QString::fromLatin1(byteArray.toBase64());
                        QString html = "<div class=\"chart-block\">\n";
                        html += QString("<img src=\"data:image/png;base64,%1\" alt=\"%2\">\n")
                                    .arg(base64, config.title.toHtmlEscaped());
                        if (!config.title.isEmpty()) {
                            html += QString("<p class=\"chart-caption\">%1</p>\n")
                                        .arg(config.title.toHtmlEscaped());
                        }
                        html += "</div>\n";
                        return html;
                    }
                }
                return "<div class=\"chart-block\">[图表渲染失败]</div>\n";
            }
            return "<div class=\"chart-block\">[数据源不存在]</div>\n";
        }
        return "<div class=\"chart-block\">[未配置图表]</div>\n";
    }

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
        .table-block {
            margin: 16px 0;
        }
        .table-block table {
            border-collapse: collapse;
            width: 100%;
            font-size: 14px;
        }
        .table-block th {
            border: 1px solid #333;
            padding: 8px 12px;
            background-color: #f0f0f0;
            font-weight: bold;
            text-align: left;
        }
        .table-block td {
            border: 1px solid #333;
            padding: 8px 12px;
        }
        .chart-block {
            text-align: center;
            margin: 16px 0;
        }
        .chart-block img {
            max-width: 100%;
        }
        .chart-caption, .image-caption {
            color: #666;
            font-size: 12px;
            margin-top: 8px;
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

    // 使用配置中的默认导出路径
    const QString defaultPath = AppConfig::instance().defaultExportPath();
    const QString fullDefaultName = defaultPath.isEmpty()
        ? defaultName
        : QDir(defaultPath).filePath(defaultName);

    const QString filePath = QFileDialog::getSaveFileName(
        parent, QObject::tr("导出报告"), fullDefaultName, filter);

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
