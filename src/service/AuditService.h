/**
 * @file AuditService.h
 * @brief 审计日志服务
 *
 * 统一入口：敏感操作成功后调用 AuditService::log 记录。
 * 自动附加当前登录用户信息（UserSession）。
 */

#ifndef AUDIT_SERVICE_H
#define AUDIT_SERVICE_H

#include <QString>

class AuditService
{
public:
    /// 记录一条审计日志（自动附加当前用户）
    static void log(const QString& action,
                    const QString& detail = QString(),
                    qint64 reportId = -1);
};

#endif // AUDIT_SERVICE_H
