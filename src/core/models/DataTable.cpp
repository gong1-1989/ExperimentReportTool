/**
 * @file DataTable.cpp
 * @brief 实验数据表实体类实现文件
 */

#include "DataTable.h"

#include <QJsonDocument>

// ===========================================================================
// ColumnDefinition 实现
// ===========================================================================

QJsonObject ColumnDefinition::toJson() const
{
    QJsonObject obj;
    obj["name"] = name;
    obj["type"] = typeToString(type);
    obj["unit"] = unit;
    obj["required"] = required;
    obj["min_value"] = minValue;
    obj["max_value"] = maxValue;
    obj["description"] = description;
    return obj;
}

ColumnDefinition ColumnDefinition::fromJson(const QJsonObject& json)
{
    ColumnDefinition col;
    col.name = json.value("name").toString();
    col.type = typeFromString(json.value("type").toString());
    col.unit = json.value("unit").toString();
    col.required = json.value("required").toBool(false);
    col.minValue = json.value("min_value").toDouble(-1e18);
    col.maxValue = json.value("max_value").toDouble(1e18);
    col.description = json.value("description").toString();
    return col;
}

QString ColumnDefinition::typeToString(ColumnType t)
{
    switch (t) {
    case ColumnType::Number:  return "number";
    case ColumnType::Text:    return "text";
    case ColumnType::Date:    return "date";
    case ColumnType::Boolean: return "boolean";
    }
    return "text";
}

ColumnType ColumnDefinition::typeFromString(const QString& str)
{
    if (str == "number")  return ColumnType::Number;
    if (str == "date")    return ColumnType::Date;
    if (str == "boolean") return ColumnType::Boolean;
    return ColumnType::Text;
}

// ===========================================================================
// DataTable 工厂方法与构造
// ===========================================================================

DataTable::Ptr DataTable::create()
{
    return Ptr(new DataTable());
}

DataTable::DataTable()
    : m_id(-1)
    , m_reportId(-1)
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
{
}

DataTable::~DataTable()
{
}

// ===========================================================================
// 列操作
// ===========================================================================

const ColumnDefinition& DataTable::columnAt(int index) const
{
    static const ColumnDefinition s_empty;
    if (index >= 0 && index < m_columns.size()) {
        return m_columns.at(index);
    }
    return s_empty;
}

void DataTable::appendColumn(const ColumnDefinition& col)
{
    m_columns.append(col);
    // 为已有数据行补充空单元格
    for (QVariantList& row : m_rows) {
        while (row.size() < m_columns.size()) {
            row.append(QVariant());
        }
    }
    m_updatedAt = QDateTime::currentDateTime();
}

void DataTable::insertColumn(int index, const ColumnDefinition& col)
{
    if (index < 0) index = 0;
    if (index >= m_columns.size()) {
        appendColumn(col);
        return;
    }
    m_columns.insert(index, col);
    // 为已有数据行在对应位置插入空单元格
    for (QVariantList& row : m_rows) {
        row.insert(index, QVariant());
    }
    m_updatedAt = QDateTime::currentDateTime();
}

void DataTable::removeColumn(int index)
{
    if (index < 0 || index >= m_columns.size()) return;
    m_columns.removeAt(index);
    // 删除每行对应位置的单元格
    for (QVariantList& row : m_rows) {
        if (index < row.size()) {
            row.removeAt(index);
        }
    }
    m_updatedAt = QDateTime::currentDateTime();
}

// ===========================================================================
// 行操作
// ===========================================================================

const QVariantList& DataTable::rowAt(int index) const
{
    static const QVariantList s_empty;
    if (index >= 0 && index < m_rows.size()) {
        return m_rows.at(index);
    }
    return s_empty;
}

void DataTable::appendRow()
{
    // 创建与列数相同的空行
    QVariantList row;
    for (int i = 0; i < m_columns.size(); ++i) {
        row.append(QVariant());
    }
    m_rows.append(row);
    m_updatedAt = QDateTime::currentDateTime();
}

void DataTable::appendRow(const QVariantList& row)
{
    // 确保行长度与列数一致
    QVariantList r = row;
    while (r.size() < m_columns.size()) {
        r.append(QVariant());
    }
    while (r.size() > m_columns.size()) {
        r.removeLast();
    }
    m_rows.append(r);
    m_updatedAt = QDateTime::currentDateTime();
}

void DataTable::insertRow(int index)
{
    if (index < 0) index = 0;
    if (index >= m_rows.size()) {
        appendRow();
        return;
    }
    QVariantList row;
    for (int i = 0; i < m_columns.size(); ++i) {
        row.append(QVariant());
    }
    m_rows.insert(index, row);
    m_updatedAt = QDateTime::currentDateTime();
}

void DataTable::removeRow(int index)
{
    if (index >= 0 && index < m_rows.size()) {
        m_rows.removeAt(index);
        m_updatedAt = QDateTime::currentDateTime();
    }
}

QVariant DataTable::cellValue(int row, int col) const
{
    if (row < 0 || row >= m_rows.size()) return QVariant();
    if (col < 0 || col >= m_columns.size()) return QVariant();
    const QVariantList& r = m_rows.at(row);
    if (col < r.size()) {
        return r.at(col);
    }
    return QVariant();
}

void DataTable::setCellValue(int row, int col, const QVariant& value)
{
    if (row < 0 || row >= m_rows.size()) return;
    if (col < 0 || col >= m_columns.size()) return;

    QVariantList& r = m_rows[row];
    // 确保行有足够的单元格
    while (r.size() <= col) {
        r.append(QVariant());
    }
    r[col] = value;
    m_updatedAt = QDateTime::currentDateTime();
}

// ===========================================================================
// 数据校验
// ===========================================================================

QStringList DataTable::validate() const
{
    QStringList errors;

    for (int row = 0; row < m_rows.size(); ++row) {
        for (int col = 0; col < m_columns.size(); ++col) {
            const ColumnDefinition& colDef = m_columns.at(col);
            const QVariant& value = cellValue(row, col);
            const QString cellLabel = QString("第%1行「%2」").arg(row + 1).arg(colDef.name);

            // 必填检查
            if (colDef.required && !value.isValid()) {
                errors.append(cellLabel + "为必填项");
                continue;
            }

            if (!value.isValid() || value.isNull()) {
                continue;  // 空值且非必填，跳过
            }

            // 类型检查与范围检查
            switch (colDef.type) {
            case ColumnType::Number: {
                bool ok = false;
                const double num = value.toDouble(&ok);
                if (!ok) {
                    errors.append(cellLabel + "必须是数值");
                } else if (num < colDef.minValue || num > colDef.maxValue) {
                    errors.append(QString("%1数值 %2 超出范围 [%3, %4]")
                                      .arg(cellLabel).arg(num).arg(colDef.minValue).arg(colDef.maxValue));
                }
                break;
            }
            case ColumnType::Date: {
                if (!value.toDate().isValid()) {
                    errors.append(cellLabel + "必须是有效日期");
                }
                break;
            }
            case ColumnType::Text:
            case ColumnType::Boolean:
                // 文本和布尔类型无需额外校验
                break;
            }
        }
    }

    return errors;
}

QVector<double> DataTable::numericColumn(int colIndex) const
{
    QVector<double> result;
    if (colIndex < 0 || colIndex >= m_columns.size()) return result;

    for (const QVariantList& row : m_rows) {
        if (colIndex < row.size()) {
            bool ok = false;
            const double val = row.at(colIndex).toDouble(&ok);
            if (ok) {
                result.append(val);
            }
        }
    }
    return result;
}

QStringList DataTable::textColumn(int colIndex) const
{
    QStringList result;
    if (colIndex < 0 || colIndex >= m_columns.size()) return result;

    for (const QVariantList& row : m_rows) {
        if (colIndex < row.size()) {
            result.append(row.at(colIndex).toString());
        } else {
            result.append(QString());
        }
    }
    return result;
}

// ===========================================================================
// 序列化
// ===========================================================================

QString DataTable::columnsToJson() const
{
    QJsonArray array;
    for (const ColumnDefinition& col : m_columns) {
        array.append(col.toJson());
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

void DataTable::columnsFromJson(const QString& json)
{
    m_columns.clear();
    if (json.isEmpty()) return;

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isArray()) return;

    for (const QJsonValue& value : doc.array()) {
        if (value.isObject()) {
            m_columns.append(ColumnDefinition::fromJson(value.toObject()));
        }
    }
}

QString DataTable::rowsToJson() const
{
    QJsonArray array;
    for (const QVariantList& row : m_rows) {
        QJsonArray rowArray;
        for (const QVariant& cell : row) {
            // QVariant 转 QJsonValue
            rowArray.append(QJsonValue::fromVariant(cell));
        }
        array.append(rowArray);
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

void DataTable::rowsFromJson(const QString& json)
{
    m_rows.clear();
    if (json.isEmpty()) return;

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isArray()) return;

    for (const QJsonValue& rowValue : doc.array()) {
        if (!rowValue.isArray()) continue;
        QVariantList row;
        for (const QJsonValue& cell : rowValue.toArray()) {
            row.append(cell.toVariant());
        }
        m_rows.append(row);
    }
}

QString DataTable::toString() const
{
    return QString("DataTable(id=%1, name='%2', report=%3, cols=%4, rows=%5)")
        .arg(m_id)
        .arg(m_name)
        .arg(m_reportId)
        .arg(m_columns.size())
        .arg(m_rows.size());
}
