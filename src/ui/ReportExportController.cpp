/**
 * @file ReportExportController.cpp
 * @brief 报告导出/打印/打印预览/页面设置控制器实现
 *
 * 自 ReportEditorWindow 迁移（onExport/onPrint/onPrintPreview/onPageSetup 本体），
 * "可编辑时先保存"前置与只读判断保留在窗口槽，控制器保持纯依赖注入。
 */

#include "ReportExportController.h"

#include "export/ExportManager.h"
#include "extension/ExportAdapter.h"
#include "extension/ExportRegistry.h"
#include "extension/ReportRenderContextBuilder.h"
#include "print/PrintManager.h"
#include "ui/UiHelper.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFileDialog>
#include <QObject>
#include <QFutureWatcher>
#include <QProgressDialog>
#include <QtConcurrent/QtConcurrentRun>

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

    // 确保内置导出插件已注册
    ExportRegistry::instance().ensureBuiltinAdapters();

    // 文件对话框：原生 4 格式 + 插件格式（注册表自动聚合）
    QStringList filters;
    filters << trText("PDF 文件 (*.pdf)")
            << trText("HTML 文件 (*.html)")
            << trText("Word 文档 (*.doc)")
            << trText("文本文件 (*.txt)");
    for (ExportAdapter* a : ExportRegistry::instance().allAdapters())
        filters << a->fileFilter();

    const QString defaultName = report->title().isEmpty()
        ? trText("未命名报告") : report->title();
    QFileDialog dlg(m_parent, trText("导出报告"), QString(), filters.join(QStringLiteral(";;")));
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.selectFile(defaultName);   // 初始文件名（第三个参数是目录，不能传文件名）
    if (dlg.exec() != QDialog::Accepted) return;
    const QString path = dlg.selectedFiles().value(0);
    if (path.isEmpty()) return;
    const QString selectedFilter = dlg.selectedNameFilter();

    // 判定所选格式：原生 或 插件
    ExportFormat format = ExportFormat::Html;
    ExportAdapter* adapter = nullptr;
    if (selectedFilter.contains(QStringLiteral("(*.pdf)"))) format = ExportFormat::Pdf;
    else if (selectedFilter.contains(QStringLiteral("(*.html)"))) format = ExportFormat::Html;
    else if (selectedFilter.contains(QStringLiteral("(*.doc)"))) format = ExportFormat::Word;
    else if (selectedFilter.contains(QStringLiteral("(*.txt)"))) format = ExportFormat::Text;
    else {
        for (ExportAdapter* a : ExportRegistry::instance().allAdapters()) {
            if (selectedFilter == a->fileFilter()) { adapter = a; break; }
        }
        if (!adapter) {
            UiHelper::error(m_parent, trText("导出失败"), trText("未知的导出格式"));
            return;
        }
    }

    // 进度对话框（原生与插件共用）
    auto* watcher = new QFutureWatcher<bool>(m_parent);
    auto* progress = new QProgressDialog(
        trText("正在导出，请稍候..."), QString(), 0, 0, m_parent);
    progress->setWindowTitle(trText("导出"));
    progress->setWindowModality(Qt::WindowModal);
    progress->setCancelButton(nullptr);
    progress->setMinimumDuration(300);

    if (!adapter) {
        // ---- 原生格式：后台线程 ExportManager（不共享成员） ----
        ExportConfig config;
        config.format = format;
        config.filePath = path;

        QObject::connect(watcher, &QFutureWatcher<bool>::finished, m_parent,
                         [this, progress, config, watcher]() {
            const bool success = watcher->result();
            progress->close();
            progress->deleteLater();
            if (success) {
                UiHelper::info(m_parent, trText("导出成功"),
                               trText("报告已导出到:\n%1").arg(config.filePath));
                if (m_statusFn) m_statusFn(trText("导出成功: %1").arg(config.filePath));
            } else {
                UiHelper::error(m_parent, trText("导出失败"), trText("导出报告时发生错误，请查看日志"));
            }
        });

        watcher->setFuture(QtConcurrent::run([report, config]() {
            ExportManager exporter;   // 线程内独立构造，避免跨线程共享成员
            return exporter.exportReport(report, config, nullptr);
        }));
    } else {
        // ---- 插件格式：主线程先构建渲染上下文（含数据库查询），后台线程只消费 ----
        const ReportRenderContext ctx = buildReportRenderContext(report);

        QObject::connect(watcher, &QFutureWatcher<bool>::finished, m_parent,
                         [this, progress, path, watcher]() {
            const bool success = watcher->result();
            progress->close();
            progress->deleteLater();
            if (success) {
                UiHelper::info(m_parent, trText("导出成功"),
                               trText("报告已导出到:\n%1").arg(path));
                if (m_statusFn) m_statusFn(trText("导出成功: %1").arg(path));
            } else {
                UiHelper::error(m_parent, trText("导出失败"),
                                trText("扩展格式导出失败，请查看日志或检查文件是否被占用"));
            }
        });

        watcher->setFuture(QtConcurrent::run([ctx, adapter, path]() {
            return adapter->exportReport(ctx, path, nullptr);
        }));
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
