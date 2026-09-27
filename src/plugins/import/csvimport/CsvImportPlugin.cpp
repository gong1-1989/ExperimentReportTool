/**
 * @file CsvImportPlugin.cpp
 * @brief CSV 数据导入插件实现文件
 */

#include "CsvImportPlugin.h"
#include "core/plugin/CoreService.h"
#include "utils/CsvParser.h"
#include "core/utils/Logger.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

CsvImportPlugin::CsvImportPlugin(QObject* parent)
    : QObject(parent)
    , m_core(nullptr)
{
}

bool CsvImportPlugin::initialize(CoreService* core)
{
    m_core = core;
    if (m_core) m_core->logger()->info("CSV 导入插件已初始化");
    return true;
}

void CsvImportPlugin::shutdown()
{
    if (m_core) m_core->logger()->info("CSV 导入插件已关闭");
    m_core = nullptr;
}

QString CsvImportPlugin::formatDisplayName(const QString& format) const
{
    if (format == "csv") return tr("CSV 文件");
    if (format == "txt") return tr("文本文件");
    return format;
}

DataTable::Ptr CsvImportPlugin::importFromFile(const QString& filePath, QWidget* parent)
{
    Q_UNUSED(parent);

    // 使用 CsvParser 直接解析文件（自动检测编码和分隔符）
    CsvParser parser;
    parser.setHasHeader(true);
    CsvParseResult result = parser.parseFile(filePath);

    if (!result.success) {
        if (m_core) {
            m_core->logger()->error(QString("CSV 解析失败: %1 (行 %2)")
                                        .arg(result.errorMessage)
                                        .arg(result.errorLine));
        }
        return nullptr;
    }

    if (result.rows.isEmpty()) {
        if (m_core) m_core->logger()->warning("CSV 文件为空");
        return nullptr;
    }

    DataTable::Ptr table = DataTable::create();
    table->setName(QFileInfo(filePath).baseName());

    // 第一行作为表头
    const QStringList headers = result.rows.first();
    for (const QString& h : headers) {
        ColumnDefinition col(h, ColumnType::Text);
        table->appendColumn(col);
    }

    // 其余行作为数据
    for (int i = 1; i < result.rows.size(); ++i) {
        QVariantList row;
        for (int c = 0; c < headers.size(); ++c) {
            row.append(c < result.rows[i].size() ? result.rows[i][c] : QString(""));
        }
        table->appendRow(row);
    }

    return table;
}

DataTable::Ptr CsvImportPlugin::preview(const QString& filePath, int maxRows)
{
    DataTable::Ptr table = importFromFile(filePath);
    if (table && table->rowCount() > maxRows) {
        // 截断到 maxRows 行
        while (table->rowCount() > maxRows) {
            table->removeRow(maxRows);
        }
    }
    return table;
}
