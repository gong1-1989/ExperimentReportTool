/**
 * @file Group.h
 * @brief 组（班级/课题组）实体头文件
 *
 * 组是用户的归属单位：组长管理本组组员，组员的报告在本组内可见。
 * 超级管理员和总管不属于任何组（groupId = -1）。
 */

#ifndef GROUP_H
#define GROUP_H

#include <QString>
#include <QDateTime>
#include <QSharedPointer>
#include <QList>

/**
 * @brief 组实体类
 */
class Group
{
public:
    using Ptr = QSharedPointer<Group>;
    using List = QList<Ptr>;

    Group();
    ~Group();

    // -----------------------------------------------------------------------
    // 属性访问
    // -----------------------------------------------------------------------

    qint64 id() const { return m_id; }
    void setId(qint64 id) { m_id = id; }

    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    QString description() const { return m_description; }
    void setDescription(const QString& desc) { m_description = desc; }

    qint64 leaderId() const { return m_leaderId; }
    void setLeaderId(qint64 userId) { m_leaderId = userId; }

    QDateTime createdAt() const { return m_createdAt; }
    void setCreatedAt(const QDateTime& dt) { m_createdAt = dt; }

    QDateTime updatedAt() const { return m_updatedAt; }
    void setUpdatedAt(const QDateTime& dt) { m_updatedAt = dt; }

    // -----------------------------------------------------------------------
    // 工具方法
    // -----------------------------------------------------------------------

    bool isNew() const { return m_id <= 0; }

    static Ptr create() { return Ptr(new Group()); }

private:
    qint64  m_id = -1;          ///< 组 ID
    QString m_name;              ///< 组名称（如"物理1班"、"光学课题组"）
    QString m_description;       ///< 组描述
    qint64  m_leaderId = -1;    ///< 组长用户 ID
    QDateTime m_createdAt;       ///< 创建时间
    QDateTime m_updatedAt;       ///< 最后更新时间
};

#endif // GROUP_H
