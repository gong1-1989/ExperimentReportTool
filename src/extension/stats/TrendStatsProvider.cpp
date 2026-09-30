#include "TrendStatsProvider.h"

#include "StatsData.h"

#include <QMap>

StatsResult TrendStatsProvider::compute(const StatsScope& scope, QString* error) const
{
    StatsResult result;
    result.providerId = providerId();
    result.title = QStringLiteral("时间趋势");

    const QList<Report::Ptr> reports = fetchStatsReports(scope);
    if (reports.isEmpty()) {
        result.metricHeaders = {QStringLiteral("新建"), QStringLiteral("完成")};
        result.textConclusion = QStringLiteral("所选范围内没有报告。");
        result.warnings.append(QStringLiteral("无数据"));
        if (error) *error = QString();
        return result;
    }

    // 粒度：范围 ≤31 天按日，≤180 天按周，否则按月（无参数，自动选择）
    QDate minDate = scope.from.isValid() ? scope.from : reports.first()->createdAt().date();
    QDate maxDate = scope.to.isValid() ? scope.to : reports.first()->createdAt().date();
    for (const Report::Ptr& r : reports) {
        minDate = qMin(minDate, r->createdAt().date());
        maxDate = qMax(maxDate, r->createdAt().date());
    }
    enum Granularity { Day, Week, Month } gran = Month;
    if (minDate.daysTo(maxDate) <= 31) gran = Day;
    else if (minDate.daysTo(maxDate) <= 180) gran = Week;

    auto keyOf = [gran](const QDate& d) {
        switch (gran) {
        case Day:   return d.toString(QStringLiteral("yyyy-MM-dd"));
        case Week:  { int y = 0, w = 0; d.weekNumber(&y);
                      return QStringLiteral("%1-W%2").arg(y).arg(w, 2, 10, QLatin1Char('0')); }
        default:    return d.toString(QStringLiteral("yyyy-MM"));
        }
    };

    QMap<QDate, QPair<qint64, qint64>> bucket;  // key: 段起点日期, value: (新建, 完成)
    auto startOf = [gran](const QDate& d) {
        switch (gran) {
        case Day:   return d;
        case Week:  return d.addDays(-(d.dayOfWeek() - 1));  // 周一为段起点
        default:    return QDate(d.year(), d.month(), 1);
        }
    };

    for (const Report::Ptr& r : reports) {
        const QDate seg = startOf(r->createdAt().date());
        QPair<qint64, qint64>& b = bucket[seg];
        b.first++;
        if (r->status() == ReportStatus::Approved || r->status() == ReportStatus::Archived)
            b.second++;
    }

    // 输出行 + 折线序列
    result.metricHeaders = {QStringLiteral("新建"), QStringLiteral("完成")};
    QVariantMap chart;
    QVariantList lineSeries;
    QVariantList ptsNew, ptsDone;
    int x = 0;
    qint64 sumNew = 0, sumDone = 0;
    for (auto it = bucket.begin(); it != bucket.end(); ++it, ++x) {
        StatsRow row;
        row.name = keyOf(it.key());
        row.metrics = {it.value().first, it.value().second};
        result.rows.append(row);
        sumNew += it.value().first;
        sumDone += it.value().second;
        ptsNew.append(QVariantList{QVariant(x), QVariant(it.value().first)});
        ptsDone.append(QVariantList{QVariant(x), QVariant(it.value().second)});
    }

    QVariantMap sNew, sDone;
    sNew.insert(QStringLiteral("name"), QStringLiteral("新建"));
    sNew.insert(QStringLiteral("points"), ptsNew);
    sDone.insert(QStringLiteral("name"), QStringLiteral("完成"));
    sDone.insert(QStringLiteral("points"), ptsDone);
    lineSeries.append(sNew);
    lineSeries.append(sDone);

    chart.insert(QStringLiteral("type"), QStringLiteral("line"));
    chart.insert(QStringLiteral("series"), lineSeries);
    result.chartData = chart;

    result.textConclusion = QStringLiteral("%1 至 %2，共新建 %3 份，完成 %4 份（%5%）。")
        .arg(minDate.toString(QStringLiteral("yyyy-MM-dd")))
        .arg(maxDate.toString(QStringLiteral("yyyy-MM-dd")))
        .arg(sumNew).arg(sumDone)
        .arg(QString::number(sumNew > 0 ? 100.0 * sumDone / sumNew : 0.0, 'f', 1));

    if (error) *error = QString();
    return result;
}
