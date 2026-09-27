/**
 * @file TextBlockPlugin.cpp
 * @brief 文本块编辑器插件实现文件
 */

#include "TextBlockPlugin.h"
#include "core/plugin/CoreService.h"
#include "core/models/Report.h"
#include "editor/TextBlockEditor.h"
#include "core/utils/Logger.h"

#include <QJsonArray>

TextBlockPlugin::TextBlockPlugin(QObject* parent)
    : QObject(parent)
    , m_core(nullptr)
{
}

bool TextBlockPlugin::initialize(CoreService* core)
{
    m_core = core;
    if (m_core) m_core->logger()->info("文本块编辑器插件已初始化");
    return true;
}

void TextBlockPlugin::shutdown()
{
    if (m_core) m_core->logger()->info("文本块编辑器插件已关闭");
    m_core = nullptr;
}

ContentBlock TextBlockPlugin::createDefaultBlock() const
{
    ContentBlock block(BlockType::Paragraph);
    block.data["text"] = "";
    return block;
}

BlockEditor* TextBlockPlugin::createEditor(const ContentBlock& block, QWidget* parent)
{
    return new TextBlockEditor(block, parent);
}

/**
 * @brief 从块数据中提取 HTML 文本（保留富文本格式）
 *
 * 与 ExportManager::getBlockTextHtml 逻辑一致，
 * 优先使用 text 字段（HTML格式），为空时用 plain_text 兜底。
 */
static QString extractBlockTextHtml(const ContentBlock& block)
{
    const QString textHtml = block.data.value("text").toString();

    // 简单提取 body 内容（如果是完整 HTML 文档）
    QString result = textHtml;
    if (textHtml.contains("<body", Qt::CaseInsensitive)) {
        const int bodyStart = textHtml.indexOf("<body", 0, Qt::CaseInsensitive);
        const int bodyTagEnd = textHtml.indexOf('>', bodyStart);
        const int bodyEnd = textHtml.indexOf("</body>", bodyTagEnd, Qt::CaseInsensitive);
        if (bodyStart >= 0 && bodyTagEnd > 0 && bodyEnd > 0) {
            result = textHtml.mid(bodyTagEnd + 1, bodyEnd - bodyTagEnd - 1);
        }
    }

    // 如果 HTML 提取结果为空，尝试用 plain_text 兜底
    if (result.isEmpty() || result == "<p></p>" || result == "<p><br></p>") {
        const QString plainText = block.data.value("plain_text").toString();
        if (!plainText.isEmpty()) {
            result = QString("<p>%1</p>").arg(plainText.toHtmlEscaped());
        }
    }

    return result;
}

QString TextBlockPlugin::renderToHtml(const ContentBlock& block, const Report* report) const
{
    Q_UNUSED(report);

    switch (block.type) {
        case BlockType::Heading1:
            return QString("<h1>%1</h1>").arg(extractBlockTextHtml(block));
        case BlockType::Heading2:
            return QString("<h2>%1</h2>").arg(extractBlockTextHtml(block));
        case BlockType::Heading3:
            return QString("<h3>%1</h3>").arg(extractBlockTextHtml(block));
        case BlockType::Paragraph:
            return extractBlockTextHtml(block);
        case BlockType::BulletList: {
            QString html = "<ul>\n";
            if (block.data.value("items").isArray()) {
                for (const QJsonValue& item : block.data.value("items").toArray()) {
                    html += QString("<li>%1</li>\n").arg(item.toString().toHtmlEscaped());
                }
            }
            html += "</ul>";
            return html;
        }
        case BlockType::NumberedList: {
            QString html = "<ol>\n";
            if (block.data.value("items").isArray()) {
                for (const QJsonValue& item : block.data.value("items").toArray()) {
                    html += QString("<li>%1</li>\n").arg(item.toString().toHtmlEscaped());
                }
            }
            html += "</ol>";
            return html;
        }
        case BlockType::Quote:
            return QString("<blockquote>%1</blockquote>").arg(extractBlockTextHtml(block));
        default:
            return extractBlockTextHtml(block);
    }
}

QString TextBlockPlugin::plainText(const ContentBlock& block) const
{
    const QString plain = block.data.value("plain_text").toString();
    if (!plain.isEmpty()) return plain;
    return block.data.value("text").toString();
}
