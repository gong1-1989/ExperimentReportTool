#include "StatusStatsProvider.h"

#include "StatsData.h"

StatsResult StatusStatsProvider::compute(const StatsScope& scope, QString* error) const
{
    StatsResult result;
    result.providerId = providerId();
    result.title = QStringLiteral("报告状态分布");
    result.metricHeaders = {QStringLiteral("数量"), QStringLiteral("占比")};

    const QList<Report::Ptr> reports = fetchStatsReports(scope);
    if (reports.isEmpty()) {
        result.textConclusion = QStringLiteral("所选范围内没有报告。");
        result.warnings.append(QStringLiteral("无数据"));
        if (error) *error = QString();
        return result;
    }

    qint64 counts[5] = {0, 0, 0, 0, 0};
    for (const Report::Ptr& r : reports) {
        switch (r->status()) {
        case ReportStatus::Draft:     counts[0]++; break;
        case ReportStatus::Submitted: counts[1]++; break;
        case ReportStatus::Reviewed:  counts[2]++; break;
        case ReportStatus::Approved:  counts[3]++; break;
        case ReportStatus::Archived:  counts[4]++; break;
        }
    }

    const QString names[5] = {
        QStringLiteral("草稿"), QStringLiteral("待审核"),
        QStringLiteral("待审批"), QStringLiteral("已审批"), QStringLiteral("已归档")};

    QVariantMap chart;
    QVariantList pieSeries;
    const qint64 total = reports.size();
    for (int i = 0; i < 5; ++i) {
        if (counts[i] == 0) continue;
        const double pct = 100.0 * counts[i] / total;
        StatsRow row;
        row.name = names[i];
        row.metrics = {counts[i], QString::number(pct, 'f', 1) + QStringLiteral("%")};
        result.rows.append(row);
        // 饼图：每个扇区一个系列（name=扇区名，points=[[i, value]]）
        QVariantMap s;
        s.insert(QStringLiteral("name"), names[i]);
        s.insert(QStringLiteral("points"), QVariantList{
            QVariantList{QVariant(i), QVariant(counts[i])}});
        pieSeries.append(s);
    }
    chart.insert(QStringLiteral("type"), QStringLiteral("pie"));
    chart.insert(QStringLiteral("series"), pieSeries);
    result.chartData = chart;

    // 结论
    const int completed = counts[3] + counts[4];
    result.textConclusion = QStringLiteral("范围内共 %1 份报告；已审批/已归档 %2 份（%3%），草稿 %4 份（%5%）。")
        .arg(total).arg(completed)
        .arg(QString::number(100.0 * completed / total, 'f', 1))
        .arg(counts[0]).arg(QString::number(100.0 * counts[0] / total, 'f', 1));

    if (error) *error = QString();
    return result;
}
