/**
 * @file TagManagerPlugin.h
 * @brief 标签管理插件头文件
 */

#ifndef TAG_MANAGER_PLUGIN_H
#define TAG_MANAGER_PLUGIN_H

#include <QObject>
#include "core/plugin/ToolPluginInterface.h"

class TagManagerPlugin : public QObject, public ToolPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface ToolPluginInterface)
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.TagManager")

public:
    explicit TagManagerPlugin(QObject* parent = nullptr);

    QString name() const override { return tr("标签管理"); }
    QString iid() const override { return "com.examplereporttool.plugin.TagManager"; }
    QString version() const override { return "1.0.0"; }
    QString description() const override { return tr("管理报告标签，支持分类和筛选"); }
    QString author() const override { return "ExperimentReportTool Team"; }
    QString category() const override { return "Tool"; }

    bool initialize(CoreService* core) override;
    void shutdown() override;

    QString toolName() const override { return tr("标签管理..."); }
    QString toolCategory() const override { return tr("工具"); }

    bool execute(QWidget* parent, const QVariantMap& context = QVariantMap()) override;

private:
    CoreService* m_core;
};

#endif // TAG_MANAGER_PLUGIN_H
