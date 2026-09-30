/**
 * @file AuditLog.h
 * @brief 审计日志实体
 *
 * 记录敏感操作（登录、工作流流转、删除、备份恢复、用户管理等），
 * 用于合规追溯与问题排查。
 */

#ifndef AUDIT_LOG_H
#define AUDIT_LOG_H

#include <QString>
#include <QDateTime>
#include <QSharedPointer>

class AuditLog
{
public:
    using Ptr = QSharedPointer<AuditLog>;

    AuditLog() = default;

    qint64 id() const { return m_id; }
    void setId(qint64 id) { m_id = id; }

    qint64 userId() const { return m_userId; }
    void setUserId(qint64 id) { m_userId = id; }

    QString username() const { return m_username; }
    void setUsername(const QString& name) { m_username = name; }

    QString action() const { return m_action; }
    void setAction(const QString& action) { m_action = action; }

    QString detail() const { return m_detail; }
    void setDetail(const QString& detail) { m_detail = detail; }

    qint64 reportId() const { return m_reportId; }
    void setReportId(qint64 id) { m_reportId = id; }

    QDateTime createdAt() const { return m_createdAt; }
    void setCreatedAt(const QDateTime& t) { m_createdAt = t; }

    static Ptr create() { return Ptr(new AuditLog()); }

private:
    qint64 m_id = -1;
    qint64 m_userId = -1;
    QString m_username;
    QString m_action;
    QString m_detail;
    qint64 m_reportId = -1;
    QDateTime m_createdAt;
};

#endif // AUDIT_LOG_H
