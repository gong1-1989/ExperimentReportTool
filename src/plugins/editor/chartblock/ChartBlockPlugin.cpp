/**
 * @file ChartBlockPlugin.cpp
 * @brief 图表块编辑器插件实现文件
 */

#include "ChartBlockPlugin.h"
#include "core/plugin/CoreService.h"
#include "core/models/Report.h"
#include "editor/OtherBlockEditors.h"
#include "core/utils/Logger.h"

ChartBlockPlugin::ChartBlockPlugin(QObject* parent)
    : QObject(parent)
    , m_core(nullptr)
{
}

bool ChartBlockPlugin::initialize(CoreService* core)
{
    m_core = core;
    if (m_core) m_core->logger()->info("图表块编辑器插件已初始化");
    return true;
}

void ChartBlockPlugin::shutdown()
{
    if (m_core) m_core->logger()->info("图表块编辑器插件已关闭");
    m_core = nullptr;
}

ContentBlock ChartBlockPlugin::createDefaultBlock() const
{
    ContentBlock block(BlockType::Chart);
    block.data["chartType"] = "line";      // 默认折线图
    block.data["title"] = "";               // 图表标题
    block.data["dataTableId"] = 0;          // 关联的数据表 ID（0=未配置）
    block.data["xColumn"] = 0;              // X 轴列索引
    block.data["yColumns"] = QJsonArray({1, 2});  // Y 轴列索引列表
    return block;
}

BlockEditor* ChartBlockPlugin::createEditor(const ContentBlock& block, QWidget* parent)
{
    return new ChartBlockEditor(block, parent);
}

QString ChartBlockPlugin::renderToHtml(const ContentBlock& block) const
{
    // 图表在 HTML 中用占位符表示，实际渲染由导出器处理
    const QString title = block.data.value("title").toString();
    return QString("<div class='chart-placeholder' style='border:1px dashed #ccc;padding:20px;text-align:center;color:#999;'>"
                   "[图表: %1]</div>").arg(title.toHtmlEscaped());
}

QString ChartBlockPlugin::plainText(const ContentBlock& block) const
{
    return QString("[图表: %1]").arg(block.data.value("title").toString());
}
