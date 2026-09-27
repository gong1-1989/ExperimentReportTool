/**
 * @file TextExportPlugin.h
 * @brief 纯文本导出插件头文件
 */

#ifndef TEXT_EXPORT_PLUGIN_H
#define TEXT_EXPORT_PLUGIN_H

#include <QObject>
#include "core/plugin/ExportPluginInterface.h"

class TextExportPlugin : public QObject, public ExportPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface ExportPluginInterface)
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.TextExport")

public:
    explicit TextExportPlugin(QObject* parent = nullptr);

    QString name() const override { return tr("纯文本导出"); }
    QString iid() const override { return "com.examplereporttool.plugin.TextExport"; }
    QString version() const override { return "1.0.0"; }
    QString description() const override { return tr("将报告导出为纯文本"); }
    QString author() const override { return "ExperimentReportTool Team"; }
    QString category() const override { return "Export"; }

    bool initialize(CoreService* core) override;
    void shutdown() override;

    QString format() const override { return "txt"; }
    QString formatName() const override { return tr("纯文本"); }
    QString fileExtension() const override { return "txt"; }
    QString formatDescription() const override { return tr("纯文本格式，体积小，兼容性好"); }

    bool exportReport(const Report::Ptr& report,
                      const ExportConfig& config,
                      QWidget* parent = nullptr) override;

    int exportReports(const QList<Report::Ptr>& reports,
                      const ExportConfig& config,
                      const QString& outputDir,
                      QWidget* parent = nullptr) override;

    bool supportsBatchExport() const override { return true; }

private:
    CoreService* m_core;
};

#endif // TEXT_EXPORT_PLUGIN_H
