#pragma once
// ===========================================================================
// 报表统计公共取数（G 域）
// ---------------------------------------------------------------------------
// 报告量级小（百级），统一 findAll 后内存过滤，避免为统计扩展 Repository 接口。
// ===========================================================================

#include "extension/StatsProvider.h"
#include "core/models/Report.h"
#include "data/repositories/ReportRepository.h"
#include "service/GroupService.h"

#include <QSet>

/// 按统计范围获取报告（projectId 走 SQL 过滤，组/用户/创建时间走内存过滤）
inline QList<Report::Ptr> fetchStatsReports(const StatsScope& scope)
{
    ReportQuery query;
    query.projectId = scope.projectId;

    QList<Report::Ptr> out;
    QSet<qint64> groupUserIds;
    if (scope.groupId > 0) {
        const User::List members = GroupService::getGroupMembers(scope.groupId);
        for (const User::Ptr& u : members) groupUserIds.insert(u->id());
    }

    const QList<Report::Ptr> all = ReportRepository::findAll(query);
    out.reserve(all.size());
    for (const Report::Ptr& r : all) {
        if (scope.groupId > 0 && !groupUserIds.contains(r->createdBy())) continue;
        if (scope.userId > 0 && r->createdBy() != scope.userId) continue;
        if (scope.from.isValid() && r->createdAt().date() < scope.from) continue;
        if (scope.to.isValid() && r->createdAt().date() > scope.to) continue;
        out.append(r);
    }
    return out;
}

/// 状态显示名（与报告列表一致）
inline QString statsStatusName(ReportStatus status)
{
    switch (status) {
    case ReportStatus::Draft:     return QStringLiteral("草稿");
    case ReportStatus::Submitted: return QStringLiteral("待审核");
    case ReportStatus::Reviewed:  return QStringLiteral("待审批");
    case ReportStatus::Approved:  return QStringLiteral("已审批");
    case ReportStatus::Archived:  return QStringLiteral("已归档");
    }
    return QStringLiteral("草稿");
}

/// 最近一次动作为"退回"（组长审核退回 / 总管审批退回）
inline bool statsIsRejected(const Report::Ptr& r)
{
    const QString a = r->lastAction();
    return a == QStringLiteral("review_reject") || a == QStringLiteral("approve_reject");
}
