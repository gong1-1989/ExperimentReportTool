/**
 * @file ReportExportController.h
 * @brief 报告导出/打印/打印预览/页面设置控制器
 *
 * 自 ReportEditorWindow 提取：负责导出与打印类操作本体。
 * 调用方（窗口）负责"可编辑时先保存"前置与只读判断，
 * 控制器仅依赖报告对象、打印管理器与父窗口（对话框/提示）。
 */

#ifndef REPORT_EXPORT_CONTROLLER_H
#define REPORT_EXPORT_CONTROLLER_H

#include <QString>
#include <functional>

#include "core/models/Report.h"

class QWidget;
class PrintManager;

/**
 * @brief 报告导出/打印/打印预览/页面设置控制器
 */
class ReportExportController
{
public:
    /// 状态栏消息回调（窗口注入，用于导出成功等提示）
    using StatusFn = std::function<void(const QString&)>;

    explicit ReportExportController(QWidget* parent, PrintManager* printer,
                                    StatusFn statusFn);

    /// 导出报告（前置：调用方需先确保已保存）
    void exportReport(const Report::Ptr& report);
    /// 打印报告（前置：调用方需先确保已保存）
    void print(const Report::Ptr& report);
    /// 打印预览（前置：调用方需先确保已保存）
    void printPreview(const Report::Ptr& report);
    /// 页面设置
    void pageSetup();

private:
    QWidget* m_parent;        ///< 对话框父窗口
    PrintManager* m_printManager;  ///< 打印管理器
    StatusFn m_statusFn;      ///< 状态栏消息回调
};

#endif // REPORT_EXPORT_CONTROLLER_H
