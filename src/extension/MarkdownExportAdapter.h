/**
 * @file MarkdownExportAdapter.h
 * @brief Markdown 导出适配器（内置注册）
 */

#ifndef MARKDOWN_EXPORT_ADAPTER_H
#define MARKDOWN_EXPORT_ADAPTER_H

#include "ExportAdapter.h"

class MarkdownExportAdapter : public ExportAdapter
{
public:
    QString formatName() const override { return QStringLiteral("Markdown 文档"); }
    QString fileFilter() const override { return QStringLiteral("Markdown 文档 (*.md)"); }
    QStringList extensions() const override { return {QStringLiteral("md")}; }
    bool exportReport(const ReportRenderContext& ctx, const QString& targetPath,
                      std::function<void(int)> progress = {}) override;
};

#endif // MARKDOWN_EXPORT_ADAPTER_H
