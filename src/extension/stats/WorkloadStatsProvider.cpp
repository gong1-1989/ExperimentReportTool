#include "WorkloadStatsProvider.h"

#include "StatsData.h"
#include "service/UserService.h"

#include <QHash>
#include <algorithm>

StatsResult WorkloadStatsProvider::compute(const StatsScope& scope, QString* error) const
{
    StatsResult result;
    result.providerId = providerId();
    result.title = QStringLiteral("工作量统计");
    result.metricHeaders = {QStringLiteral("报告数"), QStringLiteral("审批通过数"),
                            QStringLiteral("被退回数")};

    const QList<Report::Ptr> reports = fetchStatsReports(scope);
    if (reports.isEmpty()) {
        result.textConclusion = QStringLiteral("所选范围内没有报告。");
        result.warnings.append(QStringLiteral("无数据"));
        if (error) *error = QString();
        return result;
    }

    // 按创建人聚合（保持创建顺序：按用户 ID 升序展示）
    QHash<qint64, qint64> totalByUser, approvedByUser, rejectedByUser;
    for (const Report::Ptr& r : reports) {
        const qint64 uid = r->createdBy();
        totalByUser[uid]++;
        if (r->status() == ReportStatus::Approved || r->status() == ReportStatus::Archived)
            approvedByUser[uid]++;
        if (statsIsRejected(r)) rejectedByUser[uid]++;
    }

    QList<qint64> userIds = totalByUser.keys();
    std::sort(userIds.begin(), userIds.end());

    // 图表：柱状图，x=用户，3 个系列（报告数/通过数/退回数）
    QVariantMap chart;
    QVariantList barSeries;
    QVariantList ptsTotal, ptsApproved, ptsRejected;
    int x = 0;
    qint64 sumTotal = 0, sumApproved = 0, sumRejected = 0;
    for (qint64 uid : userIds) {
        const qint64 total = totalByUser.value(uid);
        const qint64 approved = approvedByUser.value(uid);
        const qint64 rejected = rejectedByUser.value(uid);
        sumTotal += total; sumApproved += approved; sumRejected += rejected;

        const User::Ptr u = UserService::getById(uid);
        const QString name = u ? u->displayNameOrUsername()
                               : QStringLiteral("用户#%1").arg(uid);
        StatsRow row;
        row.name = name;
        row.metrics = {total, approved, rejected};
        result.rows.append(row);

        ptsTotal.append(QVariantList{QVariant(x), QVariant(total)});
        ptsApproved.append(QVariantList{QVariant(x), QVariant(approved)});
        ptsRejected.append(QVariantList{QVariant(x), QVariant(rejected)});
        ++x;
    }

    QVariantMap sTotal, sApproved, sRejected;
    sTotal.insert(QStringLiteral("name"), QStringLiteral("报告数"));
    sTotal.insert(QStringLiteral("points"), ptsTotal);
    sApproved.insert(QStringLiteral("name"), QStringLiteral("审批通过数"));
    sApproved.insert(QStringLiteral("points"), ptsApproved);
    sRejected.insert(QStringLiteral("name"), QStringLiteral("被退回数"));
    sRejected.insert(QStringLiteral("points"), ptsRejected);
    barSeries.append(sTotal);
    barSeries.append(sApproved);
    barSeries.append(sRejected);

    chart.insert(QStringLiteral("type"), QStringLiteral("bar"));
    chart.insert(QStringLiteral("series"), barSeries);
    result.chartData = chart;

    result.textConclusion = QStringLiteral("共 %1 人参与编写，报告 %2 份，审批通过 %3 份，被退回 %4 份。")
        .arg(userIds.size()).arg(sumTotal).arg(sumApproved).arg(sumRejected);

    if (error) *error = QString();
    return result;
}
