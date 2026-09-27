/**
 * @file PluginManagerPlugin.h
 * @brief 插件管理工具插件头文件
 */

#ifndef PLUGIN_MANAGER_PLUGIN_H
#define PLUGIN_MANAGER_PLUGIN_H

#include <QObject>
#include "core/plugin/ToolPluginInterface.h"

class PluginManagerPlugin : public QObject, public ToolPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface ToolPluginInterface)
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.PluginManager")

public:
    explicit PluginManagerPlugin(QObject* parent = nullptr);

    QString name() const override { return tr("插件管理"); }
    QString iid() const override { return "com.examplereporttool.plugin.PluginManager"; }
    QString version() const override { return "1.0.0"; }
    QString description() const override { return tr("查看和管理已加载的插件"); }
    QString author() const override { return "ExperimentReportTool Team"; }
    QString category() const override { return "Tool"; }

    bool initialize(CoreService* core) override;
    void shutdown() override;

    QString toolName() const override { return tr("插件管理..."); }
    QString toolCategory() const override { return tr("帮助"); }

    bool execute(QWidget* parent, const QVariantMap& context = QVariantMap()) override;

private:
    CoreService* m_core;
};

#endif // PLUGIN_MANAGER_PLUGIN_H
