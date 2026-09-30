#pragma once
// ===========================================================================
// 写作辅助工具契约（F 域）
// ---------------------------------------------------------------------------
// 主程序从编辑器构造文档上下文，工具只做纯本地文本分析/生成，
// 不接触数据库、不接触编辑器内部。与 ExportAdapter / DataAnalyzer /
// StatsProvider 同风格的扩展契约，支持内置注册与动态库插件。
// ===========================================================================

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <memory>

/// 文档上下文（主程序从编辑器构造，工具只读）
struct DocumentContext {
    QString text;              ///< 编辑器全文（纯文本）
    int cursorPos = -1;        ///< 光标位置（-1=未知）
    int selectionStart = -1;   ///< 选区起点（-1=无选区）
    int selectionEnd = -1;     ///< 选区终点
    QString reportTitle;       ///< 报告标题
    int tableCount = 0;        ///< 报告数据表数量（<=0 表示未提供）
};

/// 工具执行结果
struct DocumentToolResult {
    QStringList messages;      ///< 提示列表 / 片段名列表
    bool applyEdit = false;    ///< 是否需要写回编辑器
    int insertPos = -1;        ///< 插入位置（-1=不插入）
    QString insertText;        ///< 待插入文本（applyEdit 时有效）
    QVariantMap stats;         ///< 结构化统计（指标名→值，键按名称排序展示）
};

/**
 * @brief 写作辅助工具抽象契约
 *
 * 纯抽象类（非 QObject）。snippets()/snippetText() 为可选能力，
 * 仅“常用片段插入”类工具实现，其余工具使用默认空实现。
 */
class DocumentTool {
public:
    virtual ~DocumentTool() = default;

    virtual QString toolId() const = 0;          ///< 唯一 ID（注册键）
    virtual QString displayName() const = 0;     ///< UI 显示名
    virtual QString description() const = 0;     ///< 说明
    virtual DocumentToolResult execute(const DocumentContext& ctx, QString* error) const = 0;

    /// 可选：可用片段名列表（片段类工具实现）
    virtual QStringList snippets() const { return {}; }
    /// 可选：指定片段文本（片段类工具实现）
    virtual QString snippetText(const QString& name) const { return {}; }
};

using DocumentToolPtr = std::shared_ptr<DocumentTool>;
