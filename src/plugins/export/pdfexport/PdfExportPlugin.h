/**
 * @file PdfExportPlugin.h
 * @brief PDF 导出插件头文件
 *
 * 将报告导出为 PDF 格式的插件。
 */

#ifndef PDF_EXPORT_PLUGIN_H
#define PDF_EXPORT_PLUGIN_H

#include <QObject>
#include "core/plugin/ExportPluginInterface.h"

/**
 * @brief PDF 导出插件
 */
class PdfExportPlugin : public QObject, public ExportPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface ExportPluginInterface)
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.PdfExport")

public:
    /// 构造函数
    explicit PdfExportPlugin(QObject* parent = nullptr);

    // ========================================================================
    // PluginInterface 实现
    // ========================================================================
    QString name() const override { return tr("PDF 导出"); }
    QString iid() const override { return "com.examplereporttool.plugin.PdfExport"; }
    QString version() const override { return "1.0.0"; }
    QString description() const override { return tr("将报告导出为 PDF 文档"); }
    QString author() const override { return "ExperimentReportTool Team"; }
    QString category() const override { return "Export"; }

    bool initialize(CoreService* core) override;
    void shutdown() override;

    // ========================================================================
    // ExportPluginInterface 实现
    // ========================================================================
    QString format() const override { return "pdf"; }
    QString formatName() const override { return tr("PDF 文档"); }
    QString fileExtension() const override { return "pdf"; }
    QString formatDescription() const override { return tr("便携式文档格式，适合打印和分享"); }

    bool exportReport(const Report::Ptr& report,
                      const ExportConfig& config,
                      QWidget* parent = nullptr) override;

    int exportReports(const QList<Report::Ptr>& reports,
                      const ExportConfig& config,
                      const QString& outputDir,
                      QWidget* parent = nullptr) override;

    bool supportsBatchExport() const override { return true; }

private:
    CoreService* m_core;  ///< 核心服务
};

#endif // PDF_EXPORT_PLUGIN_H
