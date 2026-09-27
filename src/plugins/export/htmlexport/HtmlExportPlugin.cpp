/**
 * @file HtmlExportPlugin.cpp
 * @brief HTML 导出插件实现文件
 */

#include "HtmlExportPlugin.h"
#include "core/plugin/CoreService.h"
#include "export/ExportManager.h"
#include "core/utils/Logger.h"

#include <QDir>
#include <QRegularExpression>

HtmlExportPlugin::HtmlExportPlugin(QObject* parent)
    : QObject(parent)
    , m_core(nullptr)
{
}

bool HtmlExportPlugin::initialize(CoreService* core)
{
    m_core = core;
    if (m_core) m_core->logger()->info("HTML 导出插件已初始化");
    return true;
}

void HtmlExportPlugin::shutdown()
{
    if (m_core) m_core->logger()->info("HTML 导出插件已关闭");
    m_core = nullptr;
}

bool HtmlExportPlugin::exportReport(const Report::Ptr& report,
                                     const ExportConfig& config,
                                     QWidget* parent)
{
    ExportManager exporter;
    ExportConfig htmlConfig = config;
    htmlConfig.format = ExportFormat::Html;
    return exporter.exportReport(report, htmlConfig, parent);
}

int HtmlExportPlugin::exportReports(const QList<Report::Ptr>& reports,
                                     const ExportConfig& config,
                                     const QString& outputDir,
                                     QWidget* parent)
{
    Q_UNUSED(parent);
    int successCount = 0;
    ExportManager exporter;

    for (const Report::Ptr& report : reports) {
        QString fileName = report->title();
        if (fileName.isEmpty()) fileName = tr("未命名报告_%1").arg(report->id());
        fileName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
        const QString filePath = QDir(outputDir).filePath(fileName + ".html");

        ExportConfig htmlConfig = config;
        htmlConfig.format = ExportFormat::Html;
        htmlConfig.filePath = filePath;

        if (exporter.exportReport(report, htmlConfig)) successCount++;
    }
    return successCount;
}
