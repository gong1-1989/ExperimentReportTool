/**
 * @file CsvImporter.cpp
 * @brief CSV 文件导入工具实现
 */

#include "CsvImporter.h"

#include "import/CsvImportAdapter.h"
#include "core/utils/Logger.h"

#include <QCoreApplication>
#include <QFileInfo>

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
    // 统一走导入契约（CsvImportAdapter 包装 CsvParser，自动检测编码和分隔符）
    const CsvImportAdapter adapter;
    QString parseError;
    const ImportData data = adapter.parse(filePath, &parseError);

    if (data.isEmpty()) {
        const QString err = parseError.isEmpty()
            ? QCoreApplication::translate("CsvImporter", "CSV 文件为空")
            : parseError;
        LOG_ERROR(err);
        if (errorMessage) *errorMessage = err;
        return nullptr;
    }

    DataTable::Ptr table = DataTable::create();
    table->setName(QFileInfo(filePath).baseName());

    // 第一行作为表头
    const QStringList headers = data.rows.first();
    for (const QString& h : headers) {
        ColumnDefinition col(h, ColumnType::Text);
        table->appendColumn(col);
    }

    // 其余行作为数据
    for (int i = 1; i < data.rows.size(); ++i) {
        QVariantList row;
        for (int c = 0; c < headers.size(); ++c) {
            row.append(c < data.rows[i].size() ? data.rows[i][c] : QString(""));
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
