/**
 * @file PdfExportPlugin.cpp
 * @brief PDF 导出插件实现文件
 */

#include "PdfExportPlugin.h"
#include "core/plugin/CoreService.h"
#include "export/ExportManager.h"
#include "core/utils/Logger.h"

#include <QDir>
#include <QRegularExpression>

// ============================================================================
// 构造
// ============================================================================

PdfExportPlugin::PdfExportPlugin(QObject* parent)
    : QObject(parent)
    , m_core(nullptr)
{
}

// ============================================================================
// PluginInterface 实现
// ============================================================================

bool PdfExportPlugin::initialize(CoreService* core)
{
    m_core = core;
    if (m_core) {
        m_core->logger()->info("PDF 导出插件已初始化");
    }
    return true;
}

void PdfExportPlugin::shutdown()
{
    if (m_core) {
        m_core->logger()->info("PDF 导出插件已关闭");
    }
    m_core = nullptr;
}

// ============================================================================
// ExportPluginInterface 实现
// ============================================================================

bool PdfExportPlugin::exportReport(const Report::Ptr& report,
                                    const ExportConfig& config,
                                    QWidget* parent)
{
    ExportManager exporter;
    ExportConfig pdfConfig = config;
    pdfConfig.format = ExportFormat::Pdf;
    return exporter.exportReport(report, pdfConfig, parent);
}

int PdfExportPlugin::exportReports(const QList<Report::Ptr>& reports,
                                    const ExportConfig& config,
                                    const QString& outputDir,
                                    QWidget* parent)
{
    Q_UNUSED(parent);

    int successCount = 0;
    ExportManager exporter;

    for (const Report::Ptr& report : reports) {
        // 构造文件名
        QString fileName = report->title();
        if (fileName.isEmpty()) {
            fileName = tr("未命名报告_%1").arg(report->id());
        }
        fileName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
        const QString filePath = QDir(outputDir).filePath(fileName + ".pdf");

        ExportConfig pdfConfig = config;
        pdfConfig.format = ExportFormat::Pdf;
        pdfConfig.filePath = filePath;

        if (exporter.exportReport(report, pdfConfig)) {
            successCount++;
        }
    }

    return successCount;
}
