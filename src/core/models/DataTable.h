/**
 * @file DataTable.h
 * @brief 实验数据表实体类头文件
 *
 * DataTable 表示实验中记录的原始数据表格。
 * 支持动态行列、多种数据类型（数值/文本/日期）、单位标注和数据校验。
 * 数据表可以被报告引用，并用于生成图表。
 */

#ifndef DATATABLE_H
#define DATATABLE_H

#include <QString>
#include <QDateTime>
#include <QList>
#include <QVariant>
#include <QSharedPointer>
#include <QJsonObject>
#include <QJsonArray>

/**
 * @brief 列数据类型枚举
 */
enum class ColumnType {
    Number,   ///< 数值（整数或浮点数）
    Text,     ///< 文本
    Date,     ///< 日期
    Boolean   ///< 布尔值
};

/**
 * @brief 表格列定义结构体
 *
 * 每一列包含名称、数据类型、单位和校验规则。
 */
struct ColumnDefinition {
    QString name;           ///< 列名称
    ColumnType type;        ///< 数据类型
    QString unit;           ///< 单位（如 "m/s", "kg"）
    bool required;          ///< 是否必填
    double minValue;        ///< 数值最小值（仅 Number 类型有效）
    double maxValue;        ///< 数值最大值（仅 Number 类型有效）
    QString description;    ///< 列描述

    /// 默认构造
    ColumnDefinition()
        : type(ColumnType::Text)
        , required(false)
        , minValue(-1e18)
        , maxValue(1e18)
    {}

    /// 带名称和类型的构造
    ColumnDefinition(const QString& n, ColumnType t)
        : name(n)
        , type(t)
        , required(false)
        , minValue(-1e18)
        , maxValue(1e18)
    {}

    /// 序列化为 JSON
    QJsonObject toJson() const;
    /// 从 JSON 反序列化
    static ColumnDefinition fromJson(const QJsonObject& json);
    /// 类型转字符串
    static QString typeToString(ColumnType t);
    /// 从字符串解析类型
    static ColumnType typeFromString(const QString& str);
};

/**
 * @brief 实验数据表实体类
 *
 * 对应数据库中的 data_tables 表。
 * 列定义（columns）和数据行（rows）都以 JSON 格式存储。
 *
 * 数据行使用 QList<QVariantList> 表示，每一行是一个 QVariantList，
 * 元素顺序与列定义对应。
 */
class DataTable
{
public:
    using Ptr = QSharedPointer<DataTable>;
    using List = QList<Ptr>;

    /// 工厂方法
    static Ptr create();

    DataTable();
    ~DataTable();

    // -----------------------------------------------------------------------
    // 属性访问器
    // -----------------------------------------------------------------------

    qint64 id() const { return m_id; }
    void setId(qint64 id) { m_id = id; }

    qint64 reportId() const { return m_reportId; }
    void setReportId(qint64 id) { m_reportId = id; }

    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    QString description() const { return m_description; }
    void setDescription(const QString& desc) { m_description = desc; }

    QDateTime createdAt() const { return m_createdAt; }
    void setCreatedAt(const QDateTime& dt) { m_createdAt = dt; }

    QDateTime updatedAt() const { return m_updatedAt; }
    void setUpdatedAt(const QDateTime& dt) { m_updatedAt = dt; }

    // -----------------------------------------------------------------------
    // 列操作
    // -----------------------------------------------------------------------

    /// 获取所有列定义
    const QList<ColumnDefinition>& columns() const { return m_columns; }
    /// 设置列定义
    void setColumns(const QList<ColumnDefinition>& cols) { m_columns = cols; }
    /// 列数量
    int columnCount() const { return m_columns.size(); }
    /// 获取指定列
    const ColumnDefinition& columnAt(int index) const;
    /// 追加列
    void appendColumn(const ColumnDefinition& col);
    /// 插入列
    void insertColumn(int index, const ColumnDefinition& col);
    /// 删除列（同时删除该列的所有数据）
    void removeColumn(int index);

    // -----------------------------------------------------------------------
    // 行操作
    // -----------------------------------------------------------------------

    /// 获取所有数据行
    const QList<QVariantList>& rows() const { return m_rows; }
    /// 设置行定义
    void setData(const QList<QVariantList>& rows){m_rows=rows;}
    /// 行数量
    int rowCount() const { return m_rows.size(); }
    /// 获取指定行
    const QVariantList& rowAt(int index) const;
    /// 追加空行
    void appendRow();
    /// 追加数据行
    void appendRow(const QVariantList& row);
    /// 插入行
    void insertRow(int index);
    /// 删除行
    void removeRow(int index);
    /// 获取单元格值
    QVariant cellValue(int row, int col) const;
    /// 设置单元格值
    void setCellValue(int row, int col, const QVariant& value);

    // -----------------------------------------------------------------------
    // 数据校验
    // -----------------------------------------------------------------------

    /**
     * @brief 校验整个表格的数据
     * @return 错误信息列表（空列表表示全部通过）
     *
     * 检查必填项、数值范围、数据类型等。
     */
    QStringList validate() const;

    /**
     * @brief 获取指定列的数值数据（用于图表生成）
     * @param colIndex 列索引
     * @return 数值列表（非数值的单元格被跳过）
     */
    QVector<double> numericColumn(int colIndex) const;

    /**
     * @brief 获取指定列的文本数据
     * @param colIndex 列索引
     * @return 字符串列表
     */
    QStringList textColumn(int colIndex) const;

    // -----------------------------------------------------------------------
    // 序列化
    // -----------------------------------------------------------------------

    /// 列定义序列化为 JSON
    QString columnsToJson() const;
    /// 从 JSON 解析列定义
    void columnsFromJson(const QString& json);
    /// 数据行序列化为 JSON
    QString rowsToJson() const;
    /// 从 JSON 解析数据行
    void rowsFromJson(const QString& json);

    /// 是否已保存
    bool isPersisted() const { return m_id > 0; }

    /// 调试字符串
    QString toString() const;

private:
    qint64  m_id;           ///< 主键
    qint64  m_reportId;     ///< 所属报告 ID
    QString m_name;         ///< 表格名称
    QString m_description;  ///< 描述
    QDateTime m_createdAt;  ///< 创建时间
    QDateTime m_updatedAt;  ///< 更新时间

    QList<ColumnDefinition> m_columns;  ///< 列定义
    QList<QVariantList>     m_rows;     ///< 数据行
};

#endif // DATATABLE_H
