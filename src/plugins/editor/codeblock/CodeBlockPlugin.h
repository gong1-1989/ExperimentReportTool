/**
 * @file CodeBlockPlugin.h
 * @brief 代码块编辑器插件头文件
 */

#ifndef CODE_BLOCK_PLUGIN_H
#define CODE_BLOCK_PLUGIN_H

#include <QObject>
#include "core/plugin/EditorBlockPluginInterface.h"

class CodeBlockPlugin : public QObject, public EditorBlockPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface EditorBlockPluginInterface)
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.CodeBlock")

public:
    explicit CodeBlockPlugin(QObject* parent = nullptr);

    QString name() const override { return tr("代码块编辑器"); }
    QString iid() const override { return "com.examplereporttool.plugin.CodeBlock"; }
    QString version() const override { return "1.0.0"; }
    QString description() const override { return tr("提供代码块的编辑和语法高亮功能"); }
    QString author() const override { return "ExperimentReportTool Team"; }
    QString category() const override { return "Editor"; }

    bool initialize(CoreService* core) override;
    void shutdown() override;

    QString blockType() const override { return "code"; }
    QString blockDisplayName() const override { return tr("代码"); }
    QString blockIcon() const override { return "💻"; }
    QString blockDescription() const override { return tr("代码块，支持语法高亮"); }

    ContentBlock createDefaultBlock() const override;
    BlockEditor* createEditor(const ContentBlock& block, QWidget* parent) override;

    QString renderToHtml(const ContentBlock& block, const Report* report = nullptr) const override;
    QString plainText(const ContentBlock& block) const override;

private:
    CoreService* m_core;
};

#endif // CODE_BLOCK_PLUGIN_H
