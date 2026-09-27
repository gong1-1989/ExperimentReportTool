/**
 * @file ImageBlockPlugin.h
 * @brief 图片块编辑器插件头文件
 */

#ifndef IMAGE_BLOCK_PLUGIN_H
#define IMAGE_BLOCK_PLUGIN_H

#include <QObject>
#include "core/plugin/EditorBlockPluginInterface.h"

class ImageBlockPlugin : public QObject, public EditorBlockPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface EditorBlockPluginInterface)
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.ImageBlock")

public:
    explicit ImageBlockPlugin(QObject* parent = nullptr);

    QString name() const override { return tr("图片块编辑器"); }
    QString iid() const override { return "com.examplereporttool.plugin.ImageBlock"; }
    QString version() const override { return "1.0.0"; }
    QString description() const override { return tr("提供图片的插入和编辑功能"); }
    QString author() const override { return "ExperimentReportTool Team"; }
    QString category() const override { return "Editor"; }

    bool initialize(CoreService* core) override;
    void shutdown() override;

    QString blockType() const override { return "image"; }
    QString blockDisplayName() const override { return tr("图片"); }
    QString blockIcon() const override { return "🖼️"; }
    QString blockDescription() const override { return tr("插入图片，支持缩放和说明"); }

    ContentBlock createDefaultBlock() const override;
    BlockEditor* createEditor(const ContentBlock& block, QWidget* parent) override;

    QString renderToHtml(const ContentBlock& block) const override;
    QString plainText(const ContentBlock& block) const override;

private:
    CoreService* m_core;
};

#endif // IMAGE_BLOCK_PLUGIN_H
