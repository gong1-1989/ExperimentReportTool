/**
 * @file JsonExportAdapter.h
 * @brief JSON 导出适配器（内置注册，结构化数据交换）
 */

#ifndef JSON_EXPORT_ADAPTER_H
#define JSON_EXPORT_ADAPTER_H

#include "ExportAdapter.h"

class JsonExportAdapter : public ExportAdapter
{
public:
    QString formatName() const override { return QStringLiteral("JSON 数据"); }
    QString fileFilter() const override { return QStringLiteral("JSON 文件 (*.json)"); }
    QStringList extensions() const override { return {QStringLiteral("json")}; }
    bool exportReport(const ReportRenderContext& ctx, const QString& targetPath,
                      std::function<void(int)> progress = {}) override;
};

#endif // JSON_EXPORT_ADAPTER_H
