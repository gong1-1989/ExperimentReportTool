/**
 * @file CsvImportAdapter.cpp
 * @brief CSV 导入适配器实现
 */

#include "import/CsvImportAdapter.h"

#include "utils/CsvParser.h"
#include "core/utils/Logger.h"

#include <QCoreApplication>

QString CsvImportAdapter::formatName() const
{
    return QCoreApplication::translate("CsvImportAdapter", "CSV 文件");
}

QStringList CsvImportAdapter::extensions() const
{
    return {QStringLiteral("csv"), QStringLiteral("txt")};
}

QString CsvImportAdapter::fileFilter() const
{
    return QStringLiteral("CSV 文件 (*.csv *.txt)");
}

ImportData CsvImportAdapter::parse(const QString& filePath,
                                   QString* errorMessage,
                                   const QHash<QString, QVariant>& options) const
{
    ImportData data;
    data.sourceFormat = QStringLiteral("csv");

    CsvParser parser;   // 编码自动检测（BOM/内容检测/系统默认）
    const QVariant delim = options.value(QStringLiteral("delimiter"));
    if (delim.isValid() && !delim.toChar().isNull()) {
        parser.setDelimiter(delim.toChar());
        parser.setAutoDetect(false);
    }

    const CsvParseResult r = parser.parseFile(filePath);
    if (!r.success) {
        if (errorMessage) {
            *errorMessage = QCoreApplication::translate("CsvImportAdapter", "CSV 解析失败: %1 (行 %2)")
                                .arg(r.errorMessage).arg(r.errorLine);
        }
        LOG_ERROR(errorMessage ? *errorMessage : QStringLiteral("CSV 解析失败"));
        return data;
    }

    data.rows = r.rows;
    data.columnCount = r.columnCount;
    data.rowCount = r.rowCount;
    data.hasHeader = r.hasHeader;
    if (!r.rows.isEmpty()) data.headers = r.rows.first();
    return data;
}

QByteArray CsvImportAdapter::buildTemplate(const QList<ColumnDefinition>& columns) const
{
    QStringList header;
    QStringList example;
    for (const ColumnDefinition& col : columns) {
        QString h = col.name;
        if (!col.unit.isEmpty()) h += QStringLiteral("(%1)").arg(col.unit);
        header.append(h);

        // 合规示例值（按列类型），保证模板可直接导入通过校验
        switch (col.type) {
        case ColumnType::Number:  example.append(QStringLiteral("1")); break;
        case ColumnType::Date:    example.append(QStringLiteral("2026-01-01")); break;
        case ColumnType::Boolean: example.append(QStringLiteral("是")); break;
        case ColumnType::Text:
        default:                  example.append(QStringLiteral("示例")); break;
        }
    }

    QList<QStringList> rows;
    rows.append(header);
    rows.append(example);
    return CsvParser::toCsv(rows, QLatin1Char(',')).toUtf8();
}
