/**
 * @file HtmlExportPlugin.h
 * @brief HTML 导出插件头文件
 */

#ifndef HTML_EXPORT_PLUGIN_H
#define HTML_EXPORT_PLUGIN_H

#include <QObject>
#include "core/plugin/ExportPluginInterface.h"

class HtmlExportPlugin : public QObject, public ExportPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface ExportPluginInterface)
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.HtmlExport")

public:
    explicit HtmlExportPlugin(QObject* parent = nullptr);

    QString name() const override { return tr("HTML 导出"); }
    QString iid() const override { return "com.examplereporttool.plugin.HtmlExport"; }
    QString version() const override { return "1.0.0"; }
    QString description() const override { return tr("将报告导出为 HTML 网页"); }
    QString author() const override { return "ExperimentReportTool Team"; }
    QString category() const override { return "Export"; }

    bool initialize(CoreService* core) override;
    void shutdown() override;

    QString format() const override { return "html"; }
    QString formatName() const override { return tr("HTML 网页"); }
    QString fileExtension() const override { return "html"; }
    QString formatDescription() const override { return tr("网页格式，可在浏览器中查看"); }

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

#endif // HTML_EXPORT_PLUGIN_H
