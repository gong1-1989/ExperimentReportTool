/**
 * @file WordExportPlugin.h
 * @brief Word 导出插件头文件
 */

#ifndef WORD_EXPORT_PLUGIN_H
#define WORD_EXPORT_PLUGIN_H

#include <QObject>
#include "core/plugin/ExportPluginInterface.h"

class WordExportPlugin : public QObject, public ExportPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface ExportPluginInterface)
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.WordExport")

public:
    explicit WordExportPlugin(QObject* parent = nullptr);

    QString name() const override { return tr("Word 导出"); }
    QString iid() const override { return "com.examplereporttool.plugin.WordExport"; }
    QString version() const override { return "1.0.0"; }
    QString description() const override { return tr("将报告导出为 Word 文档"); }
    QString author() const override { return "ExperimentReportTool Team"; }
    QString category() const override { return "Export"; }

    bool initialize(CoreService* core) override;
    void shutdown() override;

    QString format() const override { return "docx"; }
    QString formatName() const override { return tr("Word 文档"); }
    QString fileExtension() const override { return "docx"; }
    QString formatDescription() const override { return tr("Microsoft Word 格式，可编辑"); }

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

#endif // WORD_EXPORT_PLUGIN_H
