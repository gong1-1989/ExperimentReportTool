/**
 * @file ExcelExportAdapter.h
 * @brief Excel (.xlsx) 导出适配器（内置注册）
 *
 * 零第三方依赖：手写最小 xlsx（stored zip + inlineStr 单元格），
 * Excel / WPS 均可打开。报告信息一个 sheet + 每个数据表一个 sheet。
 */

#ifndef EXCEL_EXPORT_ADAPTER_H
#define EXCEL_EXPORT_ADAPTER_H

#include "ExportAdapter.h"

class ExcelExportAdapter : public ExportAdapter
{
public:
    QString formatName() const override { return QStringLiteral("Excel 工作簿"); }
    QString fileFilter() const override { return QStringLiteral("Excel 工作簿 (*.xlsx)"); }
    QStringList extensions() const override { return {QStringLiteral("xlsx")}; }
    bool exportReport(const ReportRenderContext& ctx, const QString& targetPath,
                      std::function<void(int)> progress = {}) override;
};

#endif // EXCEL_EXPORT_ADAPTER_H
