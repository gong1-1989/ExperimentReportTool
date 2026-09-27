/**
 * @file ImageBlockPlugin.cpp
 * @brief 图片块编辑器插件实现文件
 */

#include "ImageBlockPlugin.h"
#include "core/plugin/CoreService.h"
#include "core/models/Report.h"
#include "editor/OtherBlockEditors.h"
#include "core/utils/Logger.h"

#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>

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
    block.data["path"] = "";
    block.data["caption"] = "";
    block.data["width"] = 0;
    block.data["height"] = 0;
    return block;
}

BlockEditor* ImageBlockPlugin::createEditor(const ContentBlock& block, QWidget* parent)
{
    return new ImageBlockEditor(block, parent);
}

QString ImageBlockPlugin::renderToHtml(const ContentBlock& block, const Report* report) const
{
    Q_UNUSED(report);

    const QString path = block.data.value("path").toString();
    const QString caption = block.data.value("caption").toString();
    const int width = block.data.value("width").toInt(0);

    QString html = "<div class=\"image-block\" style=\"text-align:center;\">\n";

    if (!path.isEmpty() && QFile::exists(path)) {
        // 将图片转为 base64 嵌入 HTML（确保导出后图片不丢失）
        QFile imgFile(path);
        if (imgFile.open(QIODevice::ReadOnly)) {
            const QByteArray data = imgFile.readAll();
            const QString base64 = QString::fromLatin1(data.toBase64());
            const QString mime = QMimeDatabase().mimeTypeForFile(path).name();
            const QString widthAttr = (width > 0) ? QString(" width=\"%1\"").arg(width) : "";
            html += QString("<img src=\"data:%1;base64,%2\" alt=\"%3\"%4>\n")
                       .arg(mime).arg(base64).arg(caption.toHtmlEscaped()).arg(widthAttr);
            imgFile.close();
        } else {
            html += QString("<div class=\"image-placeholder\">[图片加载失败: %1]</div>\n")
                       .arg(path.toHtmlEscaped());
        }
    } else {
        html += QString("<div class=\"image-placeholder\">[图片: %1]</div>\n")
                   .arg(caption.toHtmlEscaped());
    }

    if (!caption.isEmpty()) {
        html += QString("<p class=\"image-caption\" style=\"color:#666;font-size:small;\">%1</p>\n")
                   .arg(caption.toHtmlEscaped());
    }

    html += "</div>";
    return html;
}

QString ImageBlockPlugin::plainText(const ContentBlock& block) const
{
    const QString caption = block.data.value("caption").toString();
    return caption.isEmpty() ? "[图片]" : QString("[图片: %1]").arg(caption);
}
