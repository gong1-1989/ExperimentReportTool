/**
 * @file CsvImportPlugin.h
 * @brief CSV 数据导入插件头文件
 */

#ifndef CSV_IMPORT_PLUGIN_H
#define CSV_IMPORT_PLUGIN_H

#include <QObject>
#include "core/plugin/ImportPluginInterface.h"

class CsvImportPlugin : public QObject, public ImportPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface ImportPluginInterface)
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.CsvImport")

public:
    explicit CsvImportPlugin(QObject* parent = nullptr);

    QString name() const override { return tr("CSV 导入"); }
    QString iid() const override { return "com.examplereporttool.plugin.CsvImport"; }
    QString version() const override { return "1.0.0"; }
    QString description() const override { return tr("从 CSV 文件导入数据表格"); }
    QString author() const override { return "ExperimentReportTool Team"; }
    QString category() const override { return "Import"; }

    bool initialize(CoreService* core) override;
    void shutdown() override;

    QStringList supportedFormats() const override { return {"csv", "txt"}; }
    QString formatDisplayName(const QString& format) const override;
    QString fileFilter() const override { return tr("CSV 文件 (*.csv *.txt)"); }

    DataTable::Ptr importFromFile(const QString& filePath,
                                   QWidget* parent = nullptr) override;

    bool supportsPreview() const override { return true; }
    DataTable::Ptr preview(const QString& filePath, int maxRows = 100) override;

private:
    CoreService* m_core;
};

#endif // CSV_IMPORT_PLUGIN_H
