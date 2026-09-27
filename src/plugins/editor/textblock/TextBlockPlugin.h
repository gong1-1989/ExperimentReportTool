/**
 * @file TextBlockPlugin.h
 * @brief 文本块编辑器插件头文件
 *
 * 提供文本块（段落、标题）的编辑功能。
 */

#ifndef TEXT_BLOCK_PLUGIN_H
#define TEXT_BLOCK_PLUGIN_H

#include <QObject>
#include "core/plugin/EditorBlockPluginInterface.h"

/**
 * @brief 文本块编辑器插件
 *
 * 支持的块类型：
 * - paragraph: 普通段落
 * - heading1: 一级标题
 * - heading2: 二级标题
 * - heading3: 三级标题
 */
class TextBlockPlugin : public QObject, public EditorBlockPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface EditorBlockPluginInterface)
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.TextBlock")

public:
    explicit TextBlockPlugin(QObject* parent = nullptr);

    // ========================================================================
    // PluginInterface 实现
    // ========================================================================
    QString name() const override { return tr("文本块编辑器"); }
    QString iid() const override { return "com.examplereporttool.plugin.TextBlock"; }
    QString version() const override { return "1.0.0"; }
    QString description() const override { return tr("提供段落和标题的文本编辑功能"); }
    QString author() const override { return "ExperimentReportTool Team"; }
    QString category() const override { return "Editor"; }

    bool initialize(CoreService* core) override;
    void shutdown() override;

    // ========================================================================
    // EditorBlockPluginInterface 实现
    // ========================================================================
    QString blockType() const override { return "paragraph"; }
    QString blockDisplayName() const override { return tr("文本"); }
    QString blockIcon() const override { return "📝"; }
    QString blockDescription() const override { return tr("普通段落或标题文本"); }

    ContentBlock createDefaultBlock() const override;
    BlockEditor* createEditor(const ContentBlock& block, QWidget* parent) override;

    QString renderToHtml(const ContentBlock& block, const Report* report = nullptr) const override;
    QString plainText(const ContentBlock& block) const override;

private:
    CoreService* m_core;
};

#endif // TEXT_BLOCK_PLUGIN_H
