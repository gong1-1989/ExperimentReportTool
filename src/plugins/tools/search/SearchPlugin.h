/**
 * @file SearchPlugin.h
 * @brief 搜索功能插件头文件
 */

#ifndef SEARCH_PLUGIN_H
#define SEARCH_PLUGIN_H

#include <QObject>
#include "core/plugin/ToolPluginInterface.h"

class SearchPlugin : public QObject, public ToolPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface ToolPluginInterface)
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.Search")

public:
    explicit SearchPlugin(QObject* parent = nullptr);

    QString name() const override { return tr("全文搜索"); }
    QString iid() const override { return "com.examplereporttool.plugin.Search"; }
    QString version() const override { return "1.0.0"; }
    QString description() const override { return tr("在所有报告中进行全文搜索"); }
    QString author() const override { return "ExperimentReportTool Team"; }
    QString category() const override { return "Tool"; }

    bool initialize(CoreService* core) override;
    void shutdown() override;

    QString toolName() const override { return tr("搜索..."); }
    QString toolCategory() const override { return tr("编辑"); }
    QString toolShortcut() const override { return "Ctrl+F"; }

    bool execute(QWidget* parent, const QVariantMap& context = QVariantMap()) override;

private:
    CoreService* m_core;
};

#endif // SEARCH_PLUGIN_H
