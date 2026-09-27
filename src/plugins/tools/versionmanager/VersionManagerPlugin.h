/**
 * @file VersionManagerPlugin.h
 * @brief 版本管理插件头文件
 */

#ifndef VERSION_MANAGER_PLUGIN_H
#define VERSION_MANAGER_PLUGIN_H

#include <QObject>
#include "core/plugin/ToolPluginInterface.h"

class VersionManagerPlugin : public QObject, public ToolPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface ToolPluginInterface)
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.VersionManager")

public:
    explicit VersionManagerPlugin(QObject* parent = nullptr);

    QString name() const override { return tr("版本历史"); }
    QString iid() const override { return "com.examplereporttool.plugin.VersionManager"; }
    QString version() const override { return "1.0.0"; }
    QString description() const override { return tr("查看和管理报告版本历史"); }
    QString author() const override { return "ExperimentReportTool Team"; }
    QString category() const override { return "Tool"; }

    bool initialize(CoreService* core) override;
    void shutdown() override;

    QString toolName() const override { return tr("版本历史..."); }
    QString toolCategory() const override { return tr("工具"); }

    bool execute(QWidget* parent, const QVariantMap& context = QVariantMap()) override;
    bool isAvailable(const QVariantMap& context = QVariantMap()) const override;

private:
    CoreService* m_core;
};

#endif // VERSION_MANAGER_PLUGIN_H
