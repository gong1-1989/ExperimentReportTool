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
    // 创建一个 3x3 的默认表格
    QJsonArray rows;
    for (int r = 0; r < 3; ++r) {
        QJsonArray cols;
        for (int c = 0; c < 3; ++c) {
            cols.append("");
        }
        rows.append(cols);
    }
    block.data["rows"] = rows;
    block.data["headers"] = QJsonArray({"列1", "列2", "列3"});
    return block;
}

BlockEditor* TableBlockPlugin::createEditor(const ContentBlock& block, QWidget* parent)
{
    return new TableBlockEditor(block, parent);
}

QString TableBlockPlugin::renderToHtml(const ContentBlock& block) const
{
    // 简单的 HTML 表格渲染
    QString html = "<table border='1' cellpadding='4' style='border-collapse:collapse;'>";

    // 表头
    const QJsonArray headers = block.data.value("headers").toArray();
    if (!headers.isEmpty()) {
        html += "<thead><tr>";
        for (const QJsonValue& h : headers) {
            html += QString("<th style='background:#f0f0f0;'>%1</th>").arg(h.toString().toHtmlEscaped());
        }
        html += "</tr></thead>";
    }

    // 表体
    const QJsonArray rows = block.data.value("rows").toArray();
    html += "<tbody>";
    for (const QJsonValue& rowVal : rows) {
        html += "<tr>";
        const QJsonArray cols = rowVal.toArray();
        for (const QJsonValue& col : cols) {
            html += QString("<td>%1</td>").arg(col.toString().toHtmlEscaped());
        }
        html += "</tr>";
    }
    html += "</tbody></table>";

    return html;
}

QString TableBlockPlugin::plainText(const ContentBlock& block) const
{
    QString text;
    const QJsonArray rows = block.data.value("rows").toArray();
    for (const QJsonValue& rowVal : rows) {
        const QJsonArray cols = rowVal.toArray();
        QStringList rowTexts;
        for (const QJsonValue& col : cols) {
            rowTexts.append(col.toString());
        }
        text += rowTexts.join("\t") + "\n";
    }
    return text;
}
