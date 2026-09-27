/**
 * @file ImageBlockPlugin.cpp
 * @brief 图片块编辑器插件实现文件
 */

#include "ImageBlockPlugin.h"
#include "core/plugin/CoreService.h"
#include "core/models/Report.h"
#include "editor/OtherBlockEditors.h"
#include "core/utils/Logger.h"

ImageBlockPlugin::ImageBlockPlugin(QObject* parent)
    : QObject(parent)
    , m_core(nullptr)
{
}

bool ImageBlockPlugin::initialize(CoreService* core)
{
    m_core = core;
    if (m_core) m_core->logger()->info("图片块编辑器插件已初始化");
    return true;
}

void ImageBlockPlugin::shutdown()
{
    if (m_core) m_core->logger()->info("图片块编辑器插件已关闭");
    m_core = nullptr;
}

ContentBlock ImageBlockPlugin::createDefaultBlock() const
{
    ContentBlock block(BlockType::Image);
    block.data["path"] = "";        // 图片路径
    block.data["caption"] = "";     // 图片说明
    block.data["width"] = 0;        // 宽度（0=自适应）
    block.data["height"] = 0;       // 高度（0=自适应）
    return block;
}

BlockEditor* ImageBlockPlugin::createEditor(const ContentBlock& block, QWidget* parent)
{
    return new ImageBlockEditor(block, parent);
}

QString ImageBlockPlugin::renderToHtml(const ContentBlock& block) const
{
    const QString path = block.data.value("path").toString();
    const QString caption = block.data.value("caption").toString();
    const int width = block.data.value("width").toInt(0);

    QString widthAttr = (width > 0) ? QString("width='%1'").arg(width) : "";
    QString html = QString("<div style='text-align:center;'>"
                           "<img src='%1' %2 />").arg(path.toHtmlEscaped(), widthAttr);
    if (!caption.isEmpty()) {
        html += QString("<p style='color:#666;font-size:small;'>%1</p>").arg(caption.toHtmlEscaped());
    }
    html += "</div>";
    return html;
}

QString ImageBlockPlugin::plainText(const ContentBlock& block) const
{
    const QString caption = block.data.value("caption").toString();
    return caption.isEmpty() ? "[图片]" : QString("[图片: %1]").arg(caption);
}
