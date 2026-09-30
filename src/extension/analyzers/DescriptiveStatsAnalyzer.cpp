#include "DescriptiveStatsAnalyzer.h"
#include "StatsMath.h"

#include <QStringList>
#include <vector>

AnalysisResult DescriptiveStatsAnalyzer::analyze(const QList<QVariantList>& rows,
                                                 const QStringList& columnNames,
                                                 const QVariantMap& params,
                                                 QString* error) const
{
    AnalysisResult r;
    r.analyzerId = analyzerId();
    const int col = params.value(QStringLiteral("columnIndex")).toInt();
    if (col < 0 || rows.isEmpty() || col >= rows.size()) {
        if (error) *error = QStringLiteral("未选择有效列");
        return r;
    }
    const QString colName = col < columnNames.size() ? columnNames.at(col) : QStringLiteral("列%1").arg(col + 1);
    r.title = QStringLiteral("描述统计：%1").arg(colName);

    std::vector<double> v;
    for (const QVariantList& row : rows) {
        if (col >= row.size()) continue;
        bool ok = false;
        const double d = StatsMath::toDouble(row.at(col), &ok);
        if (ok) v.push_back(d);
    }
    if (v.empty()) {
        if (error) *error = QStringLiteral("所选列没有可计算的数值数据");
        return r;
    }

    const double m = StatsMath::mean(v);
    const double sd = StatsMath::stddev(v);
    const double med = StatsMath::median(v);
    double mn = v.front(), mx = v.front();
    for (double x : v) { mn = std::min(mn, x); mx = std::max(mx, x); }
    const double cv = m != 0.0 ? sd / std::fabs(m) * 100.0 : 0.0;

    r.summaryTable = {
        {QStringLiteral("统计量"), QStringLiteral("数值")},
        {QStringLiteral("样本数"), QString::number(v.size())},
        {QStringLiteral("最小值"), QString::number(mn, 'g', 6)},
        {QStringLiteral("最大值"), QString::number(mx, 'g', 6)},
        {QStringLiteral("均值"), QString::number(m, 'g', 6)},
        {QStringLiteral("中位数"), QString::number(med, 'g', 6)},
        {QStringLiteral("标准差"), QString::number(sd, 'g', 6)},
        {QStringLiteral("极差"), QString::number(mx - mn, 'g', 6)},
        {QStringLiteral("变异系数 (%)"), QString::number(cv, 'g', 4)},
    };

    r.textConclusion = QStringLiteral("%1：样本数 %2，均值 %3，标准差 %4，范围 %5 ~ %6。")
        .arg(colName).arg(v.size())
        .arg(QString::number(m, 'g', 4)).arg(QString::number(sd, 'g', 4))
        .arg(QString::number(mn, 'g', 4)).arg(QString::number(mx, 'g', 4));

    if (v.size() < 3)
        r.warnings << QStringLiteral("样本量不足 3，标准差等统计量仅供参考");
    if (sd == 0.0 && v.size() > 1)
        r.warnings << QStringLiteral("数据无离散度（标准差为 0）");

    return r;
}
