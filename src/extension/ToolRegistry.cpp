#include "ToolRegistry.h"

#include "tools/WordCountTool.h"
#include "tools/SnippetTool.h"
#include "tools/QualityCheckTool.h"

ToolRegistry& ToolRegistry::instance()
{
    static ToolRegistry s_registry;
    return s_registry;
}

void ToolRegistry::registerTool(const DocumentToolPtr& tool)
{
    if (!tool) return;
    for (const DocumentToolPtr& t : m_tools) {
        if (t->toolId() == tool->toolId()) return;  // 已注册则跳过（内置优先）
    }
    m_tools.append(tool);
}

QList<DocumentToolPtr> ToolRegistry::tools() const
{
    return m_tools;
}

DocumentToolPtr ToolRegistry::toolById(const QString& id) const
{
    for (const DocumentToolPtr& t : m_tools) {
        if (t->toolId() == id) return t;
    }
    return nullptr;
}

void ToolRegistry::ensureBuiltinTools()
{
    static const bool s_ready = []() {
        ToolRegistry& r = ToolRegistry::instance();
        r.registerTool(std::make_shared<WordCountTool>());
        r.registerTool(std::make_shared<SnippetTool>());
        r.registerTool(std::make_shared<QualityCheckTool>());
        return true;
    }();
    Q_UNUSED(s_ready);
}
