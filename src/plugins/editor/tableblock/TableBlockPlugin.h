/**
 * @file TableBlockPlugin.h
 * @brief 表格块编辑器插件头文件
 */

#ifndef TABLE_BLOCK_PLUGIN_H
#define TABLE_BLOCK_PLUGIN_H

#include <QObject>
#include "core/plugin/EditorBlockPluginInterface.h"

class TableBlockPlugin : public QObject, public EditorBlockPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface EditorBlockPluginInterface)
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.TableBlock")

public:
    explicit TableBlockPlugin(QObject* parent = nullptr);

    QString name() const override { return tr("表格块编辑器"); }
    QString iid() const override { return "com.examplereporttool.plugin.TableBlock"; }
    QString version() const override { return "1.0.0"; }
    QString description() const override { return tr("提供数据表格的编辑功能"); }
    QString author() const override { return "ExperimentReportTool Team"; }
    QString category() const override { return "Editor"; }

    bool initialize(CoreService* core) override;
    void shutdown() override;

    QString blockType() const override { return "table"; }
    QString blockDisplayName() const override { return tr("表格"); }
    QString blockIcon() const override { return "📊"; }
    QString blockDescription() const override { return tr("可编辑的数据表格"); }

    ContentBlock createDefaultBlock() const override;
    BlockEditor* createEditor(const ContentBlock& block, QWidget* parent) override;

    QString renderToHtml(const ContentBlock& block, const Report* report = nullptr) const override;
    QString plainText(const ContentBlock& block) const override;

private:
    CoreService* m_core;
};

#endif // TABLE_BLOCK_PLUGIN_H
