/**
 * @file AuditService.cpp
 * @brief 审计日志服务实现
 */

#include "AuditService.h"
#include "core/models/AuditLog.h"
#include "data/repositories/AuditRepository.h"
#include "core/utils/UserSession.h"
#include "core/models/User.h"

#include <QDateTime>

void AuditService::log(const QString& action, const QString& detail, qint64 reportId)
{
    AuditLog::Ptr log = AuditLog::create();
    if (const User::Ptr cur = UserSession::instance().currentUser()) {
        log->setUserId(cur->id());
        log->setUsername(cur->username());
    }
    log->setAction(action);
    log->setDetail(detail);
    log->setReportId(reportId);
    log->setCreatedAt(QDateTime::currentDateTime());
    AuditRepository::insert(log);
}
