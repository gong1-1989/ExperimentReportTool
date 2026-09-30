#pragma once
// ===========================================================================
// 写作辅助工具注册表（F 域）
// ===========================================================================

#include "extension/DocumentTool.h"

class ToolRegistry {
public:
    static ToolRegistry& instance();

    void registerTool(const DocumentToolPtr& tool);
    QList<DocumentToolPtr> tools() const;              ///< 按注册顺序
    DocumentToolPtr toolById(const QString& id) const;

    /// 惰性注册全部内置写作工具（首次调用时执行一次）
    void ensureBuiltinTools();

private:
    ToolRegistry() = default;
    QList<DocumentToolPtr> m_tools;
};
