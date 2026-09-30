#include "SnippetTool.h"

#include <QHash>

namespace {

const QHash<QString, QString>& snippetMap()
{
    static const QHash<QString, QString> kSnippets = {
        { QStringLiteral("实验目的"),
          QStringLiteral("实验目的：\n    1. 通过本次实验，验证……\n    2. 掌握……的操作方法。\n    3. 加深对……原理的理解。\n") },
        { QStringLiteral("实验原理"),
          QStringLiteral("实验原理：\n    本实验基于……原理，利用……，通过测量……分析……。\n") },
        { QStringLiteral("实验步骤"),
          QStringLiteral("实验步骤：\n    1. 准备实验器材，检查设备状态。\n    2. 按规范连接/配制……。\n    3. 记录数据，重复测量 3 次取平均值。\n    4. 整理数据并计算……。\n") },
        { QStringLiteral("实验结果与分析"),
          QStringLiteral("实验结果与分析：\n    本次实验测得……，与理论值……相比，相对误差为……。\n    分析误差来源：……。\n") },
        { QStringLiteral("实验结论"),
          QStringLiteral("实验结论：\n    通过本次实验，验证了……，掌握了……的操作方法，实验结果符合/基本符合预期。\n") },
        { QStringLiteral("数据记录说明"),
          QStringLiteral("数据记录见下表，其中……列表示……。\n") },
    };
    return kSnippets;
}

}  // namespace

DocumentToolResult SnippetTool::execute(const DocumentContext& ctx, QString* error) const
{
    Q_UNUSED(ctx);
    Q_UNUSED(error);
    DocumentToolResult r;
    r.messages = snippetMap().keys();  // 片段名列表（供 UI 选择后插入）
    r.applyEdit = false;             // 插入由 UI 在用户选择后触发（snippetText）
    return r;
}

QStringList SnippetTool::snippets() const
{
    return snippetMap().keys();
}

QString SnippetTool::snippetText(const QString& name) const
{
    return snippetMap().value(name);
}
