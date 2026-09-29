/**
 * @file ObjectPreviewRenderer.h
 * @brief 内容块预览图渲染器（表/图表/图片/公式 → QPixmap）
 *
 * 从 ReportEditor 提取：纯静态渲染，入参仅 ContentBlock（+ 数据库查询），
 * 不依赖编辑器实例，供编辑器对象预览等场景复用。
 */

#ifndef OBJECT_PREVIEW_RENDERER_H
#define OBJECT_PREVIEW_RENDERER_H

#include <QPixmap>

#include "core/models/Report.h"   // ContentBlock

/**
 * @brief 内容块预览图渲染器（全部静态方法）
 */
class ObjectPreviewRenderer
{
public:
    /**
     * @brief 按对象类型渲染预览图
     * @param object 内容块（表格/图表/图片/公式）
     * @return 渲染后的预览图；其他类型返回空 QPixmap
     */
    static QPixmap render(const ContentBlock& object);

private:
    static QPixmap renderTable(const ContentBlock& object);
    static QPixmap renderChart(const ContentBlock& object);
    static QPixmap renderImage(const ContentBlock& object);
    static QPixmap renderFormula(const ContentBlock& object);
};

#endif // OBJECT_PREVIEW_RENDERER_H
