/**
 * @file ImportValidator.cpp
 * @brief 导入校验管道实现
 */

#include "import/ImportValidator.h"

#include <QDate>
#include <QObject>
#include <QSet>

namespace {

/// 布尔合法值集合（宽匹配：中英文/数字）
bool isBooleanValue(const QString& raw)
{
    static const QStringList values = {
        QStringLiteral("是"), QStringLiteral("否"),
        QStringLiteral("true"), QStringLiteral("false"),
        QStringLiteral("TRUE"), QStringLiteral("FALSE"),
        QStringLiteral("1"), QStringLiteral("0"),
        QStringLiteral("真"), QStringLiteral("假"),
    };
    return values.contains(raw);
}

/// 日期解析（支持多种常见格式）
bool parseDateValue(const QString& raw)
{
    static const QStringList formats = {
        QStringLiteral("yyyy-MM-dd"), QStringLiteral("yyyy/M/d"),
        QStringLiteral("yyyy年M月d日"), QStringLiteral("yyyy.MM.dd"),
        QStringLiteral("yyyyMMdd"),
    };
    for (const QString& fmt : formats) {
        if (QDate::fromString(raw, fmt).isValid()) return true;
    }
    return false;
}

} // namespace

QHash<int, int> ImportValidator::buildColumnMapping(const QStringList& fileHeaders,
                                                    const QList<ColumnDefinition>& targetColumns,
                                                    QStringList* missingRequired,
                                                    QStringList* unknownColumns)
{
    QHash<int, int> mapping;
    QSet<int> matchedTargets;

    for (int c = 0; c < fileHeaders.size(); ++c) {
        const QString name = fileHeaders.at(c).trimmed();
        if (name.isEmpty()) continue;
        bool found = false;
        for (int t = 0; t < targetColumns.size(); ++t) {
            if (targetColumns.at(t).name.trimmed() == name) {
                mapping.insert(c, t);
                matchedTargets.insert(t);
                found = true;
                break;
            }
        }
        if (!found && unknownColumns) unknownColumns->append(name);
    }

    if (missingRequired) {
        for (int t = 0; t < targetColumns.size(); ++t) {
            if (targetColumns.at(t).required && !matchedTargets.contains(t)) {
                missingRequired->append(targetColumns.at(t).name);
            }
        }
    }
    return mapping;
}

QList<CellIssue> ImportValidator::validate(const ImportData& data, bool hasHeader,
                                           const QList<ColumnDefinition>& targetColumns,
                                           const QHash<int, int>& columnMapping)
{
    QList<CellIssue> issues;

    const int startRow = hasHeader ? 1 : 0;
    for (int r = startRow; r < data.rows.size(); ++r) {
        const QStringList& row = data.rows.at(r);
        for (int c = 0; c < row.size(); ++c) {
            const int targetCol = columnMapping.value(c, -1);
            if (targetCol < 0 || targetCol >= targetColumns.size()) continue;

            const ColumnDefinition& col = targetColumns.at(targetCol);
            const QString raw = row.at(c).trimmed();

            // 空值：必填列报错，其余跳过
            if (raw.isEmpty()) {
                if (col.required) {
                    CellIssue issue;
                    issue.row = r - startRow;
                    issue.col = c;
                    issue.message = QObject::tr("必填列“%1”为空").arg(col.name);
                    issue.level = CellIssue::Level::Error;
                    issues.append(issue);
                }
                continue;
            }

            // 类型与范围校验
            QString why;
            if (!matchesType(col.type, raw, &why)) {
                CellIssue issue;
                issue.row = r - startRow;
                issue.col = c;
                issue.message = QObject::tr("列“%1”：%2").arg(col.name).arg(why);
                issue.level = CellIssue::Level::Error;
                issues.append(issue);
                continue;
            }

            // Number 范围检查
            if (col.type == ColumnType::Number) {
                bool ok = false;
                const double v = raw.toDouble(&ok);
                if (ok && (v < col.minValue || v > col.maxValue)) {
                    CellIssue issue;
                    issue.row = r - startRow;
                    issue.col = c;
                    issue.message = QObject::tr("列“%1”超出范围 [%2, %3]")
                                        .arg(col.name)
                                        .arg(col.minValue, 0, 'g', 6)
                                        .arg(col.maxValue, 0, 'g', 6);
                    issue.level = CellIssue::Level::Error;
                    issues.append(issue);
                }
            }
        }
    }
    return issues;
}

bool ImportValidator::matchesType(ColumnType type, const QString& raw, QString* why)
{
    switch (type) {
    case ColumnType::Number: {
        bool ok = false;
        raw.toDouble(&ok);
        if (!ok) { if (why) *why = QObject::tr("数值格式无效"); return false; }
        return true;
    }
    case ColumnType::Date:
        if (!parseDateValue(raw)) { if (why) *why = QObject::tr("日期格式无效（如 2026-01-01）"); return false; }
        return true;
    case ColumnType::Boolean:
        if (!isBooleanValue(raw)) { if (why) *why = QObject::tr("布尔值无效（是/否、true/false、1/0）"); return false; }
        return true;
    case ColumnType::Text:
    default:
        return true;   // 文本不校验
    }
}
