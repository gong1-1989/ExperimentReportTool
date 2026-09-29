/**
 * @file CsvImporter.cpp
 * @brief CSV 文件导入工具实现
 */

#include "CsvImporter.h"

#include "utils/CsvParser.h"
#include "core/utils/Logger.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QTextStream>

QStringList CsvImporter::supportedFormats()
{
    return {"csv", "txt"};
}

QString CsvImporter::fileFilter()
{
    return QStringLiteral("CSV 文件 (*.csv *.txt)");
}

QString CsvImporter::formatDisplayName(const QString& format)
{
    if (format == "csv") return QCoreApplication::translate("CsvImporter", "CSV 文件");
    if (format == "txt") return QCoreApplication::translate("CsvImporter", "文本文件");
    return format;
}

DataTable::Ptr CsvImporter::importFile(const QString& filePath, QString* errorMessage)
{
    // 使用 CsvParser 直接解析文件（自动检测编码和分隔符）
    CsvParser parser;
    parser.setHasHeader(true);
    CsvParseResult result = parser.parseFile(filePath);

    if (!result.success) {
        const QString err = QCoreApplication::translate("CsvImporter", "CSV 解析失败: %1 (行 %2)")
                                .arg(result.errorMessage)
                                .arg(result.errorLine);
        LOG_ERROR(err);
        if (errorMessage) *errorMessage = err;
        return nullptr;
    }

    if (result.rows.isEmpty()) {
        const QString err = QCoreApplication::translate("CsvImporter", "CSV 文件为空");
        LOG_WARNING(err);
        if (errorMessage) *errorMessage = err;
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

DataTable::Ptr CsvImporter::preview(const QString& filePath, int maxRows)
{
    DataTable::Ptr table = importFile(filePath);
    if (table && table->rowCount() > maxRows) {
        // 截断到 maxRows 行
        while (table->rowCount() > maxRows) {
            table->removeRow(maxRows);
        }
    }
    return table;
}
