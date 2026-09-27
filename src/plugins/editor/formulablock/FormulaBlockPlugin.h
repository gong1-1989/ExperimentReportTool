/**
 * @file FormulaBlockPlugin.h
 * @brief 公式块编辑器插件头文件
 */

#ifndef FORMULA_BLOCK_PLUGIN_H
#define FORMULA_BLOCK_PLUGIN_H

#include <QObject>
#include "core/plugin/EditorBlockPluginInterface.h"

class FormulaBlockPlugin : public QObject, public EditorBlockPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface EditorBlockPluginInterface)
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.FormulaBlock")

public:
    explicit FormulaBlockPlugin(QObject* parent = nullptr);

    QString name() const override { return tr("公式块编辑器"); }
    QString iid() const override { return "com.examplereporttool.plugin.FormulaBlock"; }
    QString version() const override { return "1.0.0"; }
    QString description() const override { return tr("提供数学公式的编辑和渲染功能"); }
    QString author() const override { return "ExperimentReportTool Team"; }
    QString category() const override { return "Editor"; }

    bool initialize(CoreService* core) override;
    void shutdown() override;

    QString blockType() const override { return "formula"; }
    QString blockDisplayName() const override { return tr("公式"); }
    QString blockIcon() const override { return "∑"; }
    QString blockDescription() const override { return tr("LaTeX 数学公式"); }

    ContentBlock createDefaultBlock() const override;
    BlockEditor* createEditor(const ContentBlock& block, QWidget* parent) override;

    QString renderToHtml(const ContentBlock& block) const override;
    QString plainText(const ContentBlock& block) const override;

private:
    CoreService* m_core;
};

#endif // FORMULA_BLOCK_PLUGIN_H
