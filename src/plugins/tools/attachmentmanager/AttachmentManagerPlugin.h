/**
 * @file AttachmentManagerPlugin.h
 * @brief 附件管理插件头文件
 */

#ifndef ATTACHMENT_MANAGER_PLUGIN_H
#define ATTACHMENT_MANAGER_PLUGIN_H

#include <QObject>
#include "core/plugin/ToolPluginInterface.h"

class AttachmentManagerPlugin : public QObject, public ToolPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface ToolPluginInterface)
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.AttachmentManager")

public:
    explicit AttachmentManagerPlugin(QObject* parent = nullptr);

    QString name() const override { return tr("附件管理"); }
    QString iid() const override { return "com.examplereporttool.plugin.AttachmentManager"; }
    QString version() const override { return "1.0.0"; }
    QString description() const override { return tr("管理报告附件文件"); }
    QString author() const override { return "ExperimentReportTool Team"; }
    QString category() const override { return "Tool"; }

    bool initialize(CoreService* core) override;
    void shutdown() override;

    QString toolName() const override { return tr("附件管理..."); }
    QString toolCategory() const override { return tr("工具"); }

    bool execute(QWidget* parent, const QVariantMap& context = QVariantMap()) override;
    bool isAvailable(const QVariantMap& context = QVariantMap()) const override;

private:
    CoreService* m_core;
};

#endif // ATTACHMENT_MANAGER_PLUGIN_H
