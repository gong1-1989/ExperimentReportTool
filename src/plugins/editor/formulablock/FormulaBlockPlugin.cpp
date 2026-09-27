/**
 * @file FormulaBlockPlugin.cpp
 * @brief 公式块编辑器插件实现文件
 */

#include "FormulaBlockPlugin.h"
#include "core/plugin/CoreService.h"
#include "core/models/Report.h"
#include "editor/FormulaBlockEditor.h"
#include "core/utils/Logger.h"

FormulaBlockPlugin::FormulaBlockPlugin(QObject* parent)
    : QObject(parent)
    , m_core(nullptr)
{
}

bool FormulaBlockPlugin::initialize(CoreService* core)
{
    m_core = core;
    if (m_core) m_core->logger()->info("公式块编辑器插件已初始化");
    return true;
}

void FormulaBlockPlugin::shutdown()
{
    if (m_core) m_core->logger()->info("公式块编辑器插件已关闭");
    m_core = nullptr;
}

ContentBlock FormulaBlockPlugin::createDefaultBlock() const
{
    ContentBlock block(BlockType::Formula);
    block.data["formula"] = "E = mc^2";
    return block;
}

BlockEditor* FormulaBlockPlugin::createEditor(const ContentBlock& block, QWidget* parent)
{
    return new FormulaBlockEditor(block, parent);
}

QString FormulaBlockPlugin::renderToHtml(const ContentBlock& block) const
{
    const QString formula = block.data.value("formula").toString();
    return QString("<div class='formula' style='text-align:center;padding:10px;font-style:italic;'>"
                   "%1</div>").arg(formula.toHtmlEscaped());
}

QString FormulaBlockPlugin::plainText(const ContentBlock& block) const
{
    return block.data.value("formula").toString();
}
