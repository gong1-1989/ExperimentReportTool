/**
 * @file ChartBlockPlugin.cpp
 * @brief 图表块编辑器插件实现文件
 */

#include "ChartBlockPlugin.h"
#include "core/plugin/CoreService.h"
#include "core/models/Report.h"
#include "editor/OtherBlockEditors.h"
#include "chart/ChartRenderer.h"
#include "chart/ChartConfigDialog.h"
#include "data/repositories/DataTableRepository.h"
#include "core/models/DataTable.h"
#include "core/utils/Logger.h"

#include <QBuffer>
#include <QPixmap>
#include <QJsonArray>

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
    block.data["chartType"] = "line";
    block.data["title"] = "";
    block.data["dataTableId"] = 0;
    block.data["xColumn"] = 0;
    block.data["yColumns"] = QJsonArray({1, 2});
    return block;
}

BlockEditor* ChartBlockPlugin::createEditor(const ContentBlock& block, QWidget* parent)
{
    return new ChartBlockEditor(block, parent);
}

/**
 * @brief 将报告中的表格块转换为临时 DataTable
 */
static DataTable::Ptr tableBlockToDataTable(const ContentBlock& block, int index, qint64 reportId)
{
    DataTable::Ptr table = DataTable::create();
    table->setId(-index - 1);
    table->setName(QString("表格块 #%1").arg(index + 1));
    table->setReportId(reportId);

    const int cols = block.data.value("cols").toInt(0);
    QList<ColumnDefinition> columns;
    if (block.data.value("headers").isArray()) {
        const QJsonArray headers = block.data.value("headers").toArray();
        for (int col = 0; col < cols; ++col) {
            ColumnDefinition colDef;
            colDef.name = col < headers.size() ? headers[col].toString() : QString("列%1").arg(col + 1);
            colDef.type = ColumnType::Text;
            columns.append(colDef);
        }
    } else {
        for (int col = 0; col < cols; ++col) {
            ColumnDefinition colDef;
            colDef.name = QString("列%1").arg(col + 1);
            colDef.type = ColumnType::Text;
            columns.append(colDef);
        }
    }
    table->setColumns(columns);

    if (block.data.value("cells").isArray()) {
        const QJsonArray cells = block.data.value("cells").toArray();
        for (int row = 0; row < cells.size(); ++row) {
            const QJsonArray rowData = cells[row].toArray();
            QVariantList variantRow;
            for (int col = 0; col < cols; ++col) {
                variantRow.append(col < rowData.size() ? rowData[col].toVariant() : QVariant());
            }
            table->appendRow(variantRow);
        }
    }

    return table;
}

/**
 * @brief 根据 ID 获取数据表（正 ID 从数据库，负 ID 从报告表格块）
 */
static DataTable::Ptr getDataTableById(qint64 id, const Report* report)
{
    if (id > 0) {
        return DataTableRepository::findById(id);
    } else if (id < 0 && report) {
        const int targetIndex = -id - 1;
        int tableBlockIndex = 0;

        for (int i = 0; i < report->blockCount(); ++i) {
            const ContentBlock& tableBlock = report->blockAt(i);
            if (tableBlock.type == BlockType::Table) {
                if (tableBlockIndex == targetIndex) {
                    return tableBlockToDataTable(tableBlock, targetIndex, report->id());
                }
                ++tableBlockIndex;
            }
        }
    }
    return nullptr;
}

QString ChartBlockPlugin::renderToHtml(const ContentBlock& block, const Report* report) const
{
    const ChartConfig config = ChartConfig::fromJson(block.data);

    if (config.dataTableId == 0) {
        return "<div class=\"chart-block\">[未配置图表]</div>";
    }

    DataTable::Ptr table = getDataTableById(config.dataTableId, report);
    if (!table) {
        if (config.dataTableId > 0) {
            return "<div class=\"chart-block\">[数据源不存在]</div>";
        }
        return "<div class=\"chart-block\">[表格块不存在]</div>";
    }

    // 使用 ChartRenderer 渲染图表为图片
    ChartRenderer renderer;
    renderer.setConfig(config);
    renderer.setDataTable(table);

    if (renderer.render()) {
        const QPixmap pixmap = renderer.toPixmap(config.width, config.height);
        if (!pixmap.isNull()) {
            QByteArray byteArray;
            QBuffer buffer(&byteArray);
            buffer.open(QIODevice::WriteOnly);
            pixmap.save(&buffer, "PNG");
            const QString base64 = QString::fromLatin1(byteArray.toBase64());

            QString html = "<div class=\"chart-block\" style=\"text-align:center;\">\n";
            html += QString("<img src=\"data:image/png;base64,%1\" alt=\"%2\">\n")
                       .arg(base64, config.title.toHtmlEscaped());
            if (!config.title.isEmpty()) {
                html += QString("<p class=\"chart-caption\" style=\"color:#666;font-size:small;\">%1</p>\n")
                           .arg(config.title.toHtmlEscaped());
            }
            html += "</div>";
            return html;
        }
    }

    return "<div class=\"chart-block\">[图表渲染失败]</div>";
}

QString ChartBlockPlugin::plainText(const ContentBlock& block) const
{
    return QString("[图表: %1]").arg(block.data.value("title").toString());
}
