/**
 * @file ChartBlockPlugin.h
 * @brief 图表块编辑器插件头文件
 */

#ifndef CHART_BLOCK_PLUGIN_H
#define CHART_BLOCK_PLUGIN_H

#include <QObject>
#include "core/plugin/EditorBlockPluginInterface.h"

class ChartBlockPlugin : public QObject, public EditorBlockPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface EditorBlockPluginInterface)
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.ChartBlock")

public:
    explicit ChartBlockPlugin(QObject* parent = nullptr);

    QString name() const override { return tr("图表块编辑器"); }
    QString iid() const override { return "com.examplereporttool.plugin.ChartBlock"; }
    QString version() const override { return "1.0.0"; }
    QString description() const override { return tr("提供数据图表的编辑和渲染功能"); }
    QString author() const override { return "ExperimentReportTool Team"; }
    QString category() const override { return "Editor"; }

    bool initialize(CoreService* core) override;
    void shutdown() override;

    QString blockType() const override { return "chart"; }
    QString blockDisplayName() const override { return tr("图表"); }
    QString blockIcon() const override { return "📈"; }
    QString blockDescription() const override { return tr("基于数据表的折线图、柱状图等"); }

    ContentBlock createDefaultBlock() const override;
    BlockEditor* createEditor(const ContentBlock& block, QWidget* parent) override;

    QString renderToHtml(const ContentBlock& block, const Report* report = nullptr) const override;
    QString plainText(const ContentBlock& block) const override;

private:
    CoreService* m_core;
};

#endif // CHART_BLOCK_PLUGIN_H
