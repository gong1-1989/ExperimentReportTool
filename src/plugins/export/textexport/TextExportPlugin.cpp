/**
 * @file TextExportPlugin.cpp
 * @brief 纯文本导出插件实现文件
 */

#include "TextExportPlugin.h"
#include "core/plugin/CoreService.h"
#include "export/ExportManager.h"
#include "core/utils/Logger.h"

#include <QDir>
#include <QRegularExpression>

TextExportPlugin::TextExportPlugin(QObject* parent)
    : QObject(parent)
    , m_core(nullptr)
{
}

bool TextExportPlugin::initialize(CoreService* core)
{
    m_core = core;
    if (m_core) m_core->logger()->info("纯文本导出插件已初始化");
    return true;
}

void TextExportPlugin::shutdown()
{
    if (m_core) m_core->logger()->info("纯文本导出插件已关闭");
    m_core = nullptr;
}

bool TextExportPlugin::exportReport(const Report::Ptr& report,
                                     const ExportConfig& config,
                                     QWidget* parent)
{
    ExportManager exporter;
    ExportConfig textConfig = config;
    textConfig.format = ExportFormat::Text;
    return exporter.exportReport(report, textConfig, parent);
}

int TextExportPlugin::exportReports(const QList<Report::Ptr>& reports,
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
        const QString filePath = QDir(outputDir).filePath(fileName + ".txt");

        ExportConfig textConfig = config;
        textConfig.format = ExportFormat::Text;
        textConfig.filePath = filePath;

        if (exporter.exportReport(report, textConfig)) successCount++;
    }
    return successCount;
}
