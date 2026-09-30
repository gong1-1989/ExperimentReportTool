/**
 * @file ReportRenderContextBuilder.h
 * @brief 报告渲染上下文构建器
 *
 * 必须在 UI（主）线程调用：内部含数据库查询（数据表、创建者用户名）。
 * 构建后的上下文供导出插件在后台线程消费，插件不触碰数据库。
 */

#ifndef REPORT_RENDER_CONTEXT_BUILDER_H
#define REPORT_RENDER_CONTEXT_BUILDER_H

#include "ExportAdapter.h"
#include "core/models/Report.h"

/// 构建报告渲染上下文（主线程调用）
ReportRenderContext buildReportRenderContext(const Report::Ptr& report);

#endif // REPORT_RENDER_CONTEXT_BUILDER_H
