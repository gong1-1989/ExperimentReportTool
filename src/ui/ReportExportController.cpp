/**
 * @file ReportExportController.cpp
 * @brief 报告导出/打印/打印预览/页面设置控制器实现
 *
 * 自 ReportEditorWindow 迁移（onExport/onPrint/onPrintPreview/onPageSetup 本体），
 * "可编辑时先保存"前置与只读判断保留在窗口槽，控制器保持纯依赖注入。
 */

#include "ReportExportController.h"

#include "export/ExportManager.h"
#include "print/PrintManager.h"
#include "ui/UiHelper.h"

#include <QApplication>
#include <QCoreApplication>

namespace {
// 非 QObject 类内使用 tr 语义：统一翻译上下文
inline QString trText(const char* source)
{
    return QCoreApplication::translate("ReportExportController", source);
}
}

ReportExportController::ReportExportController(QWidget* parent, PrintManager* printer,
                                               StatusFn statusFn)
    : m_parent(parent)
    , m_printManager(printer)
    , m_statusFn(std::move(statusFn))
{
}

void ReportExportController::exportReport(const Report::Ptr& report)
{
    if (!report) return;

    // 显示导出文件对话框
    const QString defaultName = report->title().isEmpty()
        ? trText("未命名报告") : report->title();
    const auto result = ExportManager::getSaveFilePath(m_parent, defaultName);

    if (result.first.isEmpty()) {
        return;  // 用户取消
    }

    // 执行导出
    ExportManager exporter;
    ExportConfig config;
    config.format = result.second;
    config.filePath = result.first;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool success = exporter.exportReport(report, config, m_parent);
    QApplication::restoreOverrideCursor();

    if (success) {
        UiHelper::info(m_parent, trText("导出成功"),
                       trText("报告已导出到:\n%1").arg(result.first));
        if (m_statusFn) m_statusFn(trText("导出成功: %1").arg(result.first));
    } else {
        UiHelper::error(m_parent, trText("导出失败"),
                        trText("导出报告时发生错误，请查看日志"));
    }
}

void ReportExportController::print(const Report::Ptr& report)
{
    if (!report || !m_printManager) return;

    m_printManager->print(report, m_parent);
}

void ReportExportController::printPreview(const Report::Ptr& report)
{
    if (!report || !m_printManager) return;

    m_printManager->printPreview(report, m_parent);
}

void ReportExportController::pageSetup()
{
    if (!m_printManager) return;

    PrintConfig config = m_printManager->currentConfig();
    if (m_printManager->pageSetup(config, m_parent)) {
        if (m_statusFn) m_statusFn(trText("页面设置已更新"));
    }
}
