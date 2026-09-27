/**
 * @file TextBlockPlugin.cpp
 * @brief 文本块编辑器插件实现文件
 */

#include "TextBlockPlugin.h"
#include "core/plugin/CoreService.h"
#include "core/models/Report.h"
#include "editor/TextBlockEditor.h"
#include "core/utils/Logger.h"

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

QString TextBlockPlugin::renderToHtml(const ContentBlock& block) const
{
    const QString text = block.data.value("text").toString();
    const QString escaped = text.toHtmlEscaped();

    switch (block.type) {
        case BlockType::Heading1:
            return QString("<h1>%1</h1>").arg(escaped);
        case BlockType::Heading2:
            return QString("<h2>%1</h2>").arg(escaped);
        case BlockType::Heading3:
            return QString("<h3>%1</h3>").arg(escaped);
        case BlockType::Paragraph:
        default:
            return QString("<p>%1</p>").arg(escaped);
    }
}

QString TextBlockPlugin::plainText(const ContentBlock& block) const
{
    return block.data.value("text").toString();
}
