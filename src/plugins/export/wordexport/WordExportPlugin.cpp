/**
 * @file WordExportPlugin.cpp
 * @brief Word 导出插件实现文件
 */

#include "WordExportPlugin.h"
#include "core/plugin/CoreService.h"
#include "export/ExportManager.h"
#include "core/utils/Logger.h"

#include <QDir>
#include <QRegularExpression>

WordExportPlugin::WordExportPlugin(QObject* parent)
    : QObject(parent)
    , m_core(nullptr)
{
}

bool WordExportPlugin::initialize(CoreService* core)
{
    m_core = core;
    if (m_core) m_core->logger()->info("Word 导出插件已初始化");
    return true;
}

void WordExportPlugin::shutdown()
{
    if (m_core) m_core->logger()->info("Word 导出插件已关闭");
    m_core = nullptr;
}

bool WordExportPlugin::exportReport(const Report::Ptr& report,
                                     const ExportConfig& config,
                                     QWidget* parent)
{
    ExportManager exporter;
    ExportConfig wordConfig = config;
    wordConfig.format = ExportFormat::Word;
    return exporter.exportReport(report, wordConfig, parent);
}

int WordExportPlugin::exportReports(const QList<Report::Ptr>& reports,
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
        const QString filePath = QDir(outputDir).filePath(fileName + ".docx");

        ExportConfig wordConfig = config;
        wordConfig.format = ExportFormat::Word;
        wordConfig.filePath = filePath;

        if (exporter.exportReport(report, wordConfig)) successCount++;
    }
    return successCount;
}
