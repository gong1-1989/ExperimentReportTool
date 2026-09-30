#pragma once
#include "extension/DocumentTool.h"

/**
 * @brief 字数统计工具：字数/字符数/段落数/表格数
 */
class WordCountTool : public DocumentTool {
public:
    QString toolId() const override { return QStringLiteral("word_count"); }
    QString displayName() const override { return QStringLiteral("字数统计"); }
    QString description() const override { return QStringLiteral("统计全文字数、字符数、段落数与数据表数量"); }
    DocumentToolResult execute(const DocumentContext& ctx, QString* error) const override;
};
