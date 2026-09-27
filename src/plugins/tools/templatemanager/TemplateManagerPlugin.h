/**
 * @file TemplateManagerPlugin.h
 * @brief 模板管理插件头文件
 */

#ifndef TEMPLATE_MANAGER_PLUGIN_H
#define TEMPLATE_MANAGER_PLUGIN_H

#include <QObject>
#include "core/plugin/ToolPluginInterface.h"

class TemplateManagerPlugin : public QObject, public ToolPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface ToolPluginInterface)
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.TemplateManager")

public:
    explicit TemplateManagerPlugin(QObject* parent = nullptr);

    QString name() const override { return tr("模板管理"); }
    QString iid() const override { return "com.examplereporttool.plugin.TemplateManager"; }
    QString version() const override { return "1.0.0"; }
    QString description() const override { return tr("管理报告模板，创建和编辑模板"); }
    QString author() const override { return "ExperimentReportTool Team"; }
    QString category() const override { return "Tool"; }

    bool initialize(CoreService* core) override;
    void shutdown() override;

    QString toolName() const override { return tr("模板管理..."); }
    QString toolCategory() const override { return tr("工具"); }

    bool execute(QWidget* parent, const QVariantMap& context = QVariantMap()) override;

private:
    CoreService* m_core;
};

#endif // TEMPLATE_MANAGER_PLUGIN_H
