/**
 * @file CodeBlockPlugin.cpp
 * @brief 代码块编辑器插件实现文件
 */

#include "CodeBlockPlugin.h"
#include "core/plugin/CoreService.h"
#include "core/models/Report.h"
#include "editor/OtherBlockEditors.h"
#include "core/utils/Logger.h"

CodeBlockPlugin::CodeBlockPlugin(QObject* parent)
    : QObject(parent)
    , m_core(nullptr)
{
}

bool CodeBlockPlugin::initialize(CoreService* core)
{
    m_core = core;
    if (m_core) m_core->logger()->info("代码块编辑器插件已初始化");
    return true;
}

void CodeBlockPlugin::shutdown()
{
    if (m_core) m_core->logger()->info("代码块编辑器插件已关闭");
    m_core = nullptr;
}

ContentBlock CodeBlockPlugin::createDefaultBlock() const
{
    ContentBlock block(BlockType::CodeBlock);
    block.data["code"] = "";
    block.data["language"] = "cpp";
    block.data["showLineNumbers"] = true;
    return block;
}

BlockEditor* CodeBlockPlugin::createEditor(const ContentBlock& block, QWidget* parent)
{
    return new CodeBlockEditor(block, parent);
}

QString CodeBlockPlugin::renderToHtml(const ContentBlock& block, const Report* report) const
{
    Q_UNUSED(report);

    const QString code = block.data.value("code").toString();
    const QString language = block.data.value("language").toString("text");
    const bool showLineNumbers = block.data.value("showLineNumbers").toBool(true);

    QString html = QString("<pre class='code-block' data-language='%1' "
                           "style='background:#f5f5f5;padding:12px;border-radius:4px;"
                           "overflow-x:auto;font-family:Consolas,Monaco,monospace;font-size:13px;'>")
                       .arg(language.toHtmlEscaped());

    if (showLineNumbers) {
        const QStringList lines = code.split('\n');
        for (int i = 0; i < lines.size(); ++i) {
            html += QString("<span style='color:#999;display:inline-block;width:3em;"
                            "text-align:right;margin-right:1em;user-select:none;'>%1</span>")
                        .arg(i + 1);
            html += lines[i].toHtmlEscaped() + "\n";
        }
    } else {
        html += code.toHtmlEscaped();
    }

    html += "</pre>";
    return html;
}

QString CodeBlockPlugin::plainText(const ContentBlock& block) const
{
    return block.data.value("code").toString();
}
