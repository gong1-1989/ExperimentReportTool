/**
 * @file Tag.h
 * @brief 标签实体头文件
 *
 * 标签用于对报告进行分类和筛选。
 * 一个报告可以有多个标签，一个标签可以属于多个报告（多对多关系）。
 */

#ifndef TAG_H
#define TAG_H

#include <QString>
#include <QDateTime>
#include <QSharedPointer>
#include <QList>
#include <QColor>

/**
 * @brief 标签实体类
 */
class Tag
{
public:
    using Ptr = QSharedPointer<Tag>;
    using List = QList<Ptr>;

    Tag();
    ~Tag();

    // -----------------------------------------------------------------------
    // 属性访问
    // -----------------------------------------------------------------------

    qint64 id() const { return m_id; }
    void setId(qint64 id) { m_id = id; }

    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    QString color() const { return m_color; }
    void setColor(const QString& color) { m_color = color; }

    QString description() const { return m_description; }
    void setDescription(const QString& desc) { m_description = desc; }

    QDateTime createdAt() const { return m_createdAt; }
    void setCreatedAt(const QDateTime& dt) { m_createdAt = dt; }

    int usageCount() const { return m_usageCount; }
    void setUsageCount(int count) { m_usageCount = count; }

    // -----------------------------------------------------------------------
    // 工具方法
    // -----------------------------------------------------------------------

    /// 是否为新标签（未保存到数据库）
    bool isNew() const { return m_id <= 0; }

    /// 获取标签颜色（如果未设置，根据名称生成默认颜色）
    QColor effectiveColor() const;

    /// 创建新标签的工厂方法
    static Ptr create(const QString& name = QString());

    // -----------------------------------------------------------------------
    // 预设颜色列表
    // -----------------------------------------------------------------------

    static QStringList presetColors();

private:
    qint64 m_id;            ///< 标签 ID
    QString m_name;          ///< 标签名称
    QString m_color;         ///< 标签颜色（十六进制，如 "#FF5733"）
    QString m_description;   ///< 标签描述
    QDateTime m_createdAt;   ///< 创建时间
    int m_usageCount;        ///< 使用次数（查询时填充）
};

#endif // TAG_H
