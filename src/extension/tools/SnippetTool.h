#pragma once
#include "extension/DocumentTool.h"

/**
 * @brief 常用片段插入工具：内置实验报告常用语句模板，插入到光标处
 */
class SnippetTool : public DocumentTool {
public:
    QString toolId() const override { return QStringLiteral("snippet"); }
    QString displayName() const override { return QStringLiteral("常用片段"); }
    QString description() const override { return QStringLiteral("插入实验报告常用语句模板（实验目的/原理/步骤/结论等）"); }

    DocumentToolResult execute(const DocumentContext& ctx, QString* error) const override;
    QStringList snippets() const override;
    QString snippetText(const QString& name) const override;
};
