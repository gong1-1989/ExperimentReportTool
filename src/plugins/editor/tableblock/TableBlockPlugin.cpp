/**
 * @file TableBlockPlugin.cpp
 * @brief 表格块编辑器插件实现文件
 */

#include "TableBlockPlugin.h"
#include "core/plugin/CoreService.h"
#include "core/models/Report.h"
#include "editor/OtherBlockEditors.h"
#include "core/utils/Logger.h"

#include <QJsonArray>

TableBlockPlugin::TableBlockPlugin(QObject* parent)
    : QObject(parent)
    , m_core(nullptr)
{
}

bool TableBlockPlugin::initialize(CoreService* core)
{
    m_core = core;
    if (m_core) m_core->logger()->info("表格块编辑器插件已初始化");
    return true;
}

void TableBlockPlugin::shutdown()
{
    if (m_core) m_core->logger()->info("表格块编辑器插件已关闭");
    m_core = nullptr;
}

ContentBlock TableBlockPlugin::createDefaultBlock() const
{
    ContentBlock block(BlockType::Table);
    // 使用与框架一致的字段格式：rows/cols/headers/cells
    block.data["rows"] = 3;
    block.data["cols"] = 3;
    block.data["headers"] = QJsonArray({"列1", "列2", "列3"});

    QJsonArray cells;
    for (int r = 0; r < 3; ++r) {
        QJsonArray row;
        for (int c = 0; c < 3; ++c) {
            row.append("");
        }
        cells.append(row);
    }
    block.data["cells"] = cells;
    return block;
}

BlockEditor* TableBlockPlugin::createEditor(const ContentBlock& block, QWidget* parent)
{
    return new TableBlockEditor(block, parent);
}

QString TableBlockPlugin::renderToHtml(const ContentBlock& block, const Report* report) const
{
    Q_UNUSED(report);

    // 使用与框架一致的字段格式：rows/cols/headers/cells
    const int rows = block.data.value("rows").toInt(0);
    const int cols = block.data.value("cols").toInt(0);

    if (rows <= 0 || cols <= 0) {
        return "<p>[空表格]</p>";
    }

    // 用最简单的表格标签，确保 QTextDocument 能正确渲染
    QString html = "<table border=\"1\" width=\"100%\" cellpadding=\"4\" cellspacing=\"0\">\n";

    // 表头
    if (block.data.value("headers").isArray()) {
        const QJsonArray headers = block.data.value("headers").toArray();
        html += "<tr>\n";
        for (int col = 0; col < cols; ++col) {
            const QString headerText = col < headers.size()
                                           ? headers[col].toString()
                                           : QString("列%1").arg(col + 1);
            html += QString("<th bgcolor=\"#f0f0f0\"><b>%1</b></th>\n")
                       .arg(headerText.toHtmlEscaped());
        }
        html += "</tr>\n";
    }

    // 表格数据
    if (block.data.value("cells").isArray()) {
        const QJsonArray cells = block.data.value("cells").toArray();
        for (int row = 0; row < rows && row < cells.size(); ++row) {
            html += "<tr>\n";
            const QJsonArray rowData = cells[row].toArray();
            for (int col = 0; col < cols; ++col) {
                const QString cellText = col < rowData.size()
                                             ? rowData[col].toString()
                                             : QString();
                html += QString("<td>%1</td>\n").arg(cellText.toHtmlEscaped());
            }
            html += "</tr>\n";
        }
    }

    html += "</table>\n<br>\n";
    return html;
}

QString TableBlockPlugin::plainText(const ContentBlock& block) const
{
    QString text;
    const int rows = block.data.value("rows").toInt(0);
    const int cols = block.data.value("cols").toInt(0);

    if (block.data.value("cells").isArray()) {
        const QJsonArray cells = block.data.value("cells").toArray();
        for (int row = 0; row < rows && row < cells.size(); ++row) {
            const QJsonArray rowData = cells[row].toArray();
            QStringList rowTexts;
            for (int col = 0; col < cols; ++col) {
                rowTexts.append(col < rowData.size() ? rowData[col].toString() : "");
            }
            text += rowTexts.join(" | ") + "\n";
        }
    }
    return text;
}
