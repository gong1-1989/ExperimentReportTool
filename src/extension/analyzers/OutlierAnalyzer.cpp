#include "OutlierAnalyzer.h"
#include "StatsMath.h"

#include <QJsonArray>
#include <QJsonObject>
#include <vector>

AnalysisResult OutlierAnalyzer::analyze(const QList<QVariantList>& rows,
                                       const QStringList& columnNames,
                                       const QVariantMap& params,
                                       QString* error) const
{
    AnalysisResult r;
    r.analyzerId = analyzerId();
    const int col = params.value(QStringLiteral("columnIndex")).toInt();
    const QString method = params.value(QStringLiteral("method")).toString();
    const bool useIqr = (method == QStringLiteral("iqr"));
    if (col < 0 || rows.isEmpty()) {
        if (error) *error = QStringLiteral("未选择有效列");
        return r;
    }
    const QString colName = col < columnNames.size() ? columnNames.at(col) : QStringLiteral("列%1").arg(col + 1);
    r.title = QStringLiteral("异常值检测：%1（%2）").arg(colName, useIqr ? QStringLiteral("IQR 法") : QStringLiteral("3σ 法"));

    struct Point { double v; int row; };
    std::vector<Point> pts;
    for (int i = 0; i < rows.size(); ++i) {
        if (col >= rows.at(i).size()) continue;
        bool ok = false;
        const double d = StatsMath::toDouble(rows.at(i).at(col), &ok);
        if (ok) pts.push_back({d, i});
    }
    if (pts.size() < 2) {
        if (error) *error = QStringLiteral("有效数值少于 2 个，无法检测异常值");
        return r;
    }

    double lo, hi;
    QString methodName;
    if (useIqr) {
        std::vector<double> vals;
        for (const Point& p : pts) vals.push_back(p.v);
        std::sort(vals.begin(), vals.end());
        const double q1 = vals[vals.size() / 4];
        const double q3 = vals[vals.size() * 3 / 4];
        const double iqr = q3 - q1;
        lo = q1 - 1.5 * iqr;
        hi = q3 + 1.5 * iqr;
        methodName = QStringLiteral("IQR");
        r.summaryTable = {{QStringLiteral("统计量"), QStringLiteral("数值")},
                          {QStringLiteral("下界 Q1-1.5IQR"), QString::number(lo, 'g', 6)},
                          {QStringLiteral("上界 Q3+1.5IQR"), QString::number(hi, 'g', 6)}};
    } else {
        std::vector<double> vals;
        for (const Point& p : pts) vals.push_back(p.v);
        const double mu = StatsMath::mean(vals);
        const double sd = StatsMath::stddev(vals);
        lo = mu - 3.0 * sd;
        hi = mu + 3.0 * sd;
        methodName = QStringLiteral("3σ");
        r.summaryTable = {{QStringLiteral("统计量"), QStringLiteral("数值")},
                          {QStringLiteral("均值"), QString::number(mu, 'g', 6)},
                          {QStringLiteral("标准差"), QString::number(sd, 'g', 6)},
                          {QStringLiteral("下界 μ-3σ"), QString::number(lo, 'g', 6)},
                          {QStringLiteral("上界 μ+3σ"), QString::number(hi, 'g', 6)}};
    }

    // 异常点收集 + 散点序列
    QList<QStringList> outlierRows;
    QJsonArray allPts, badPts;
    for (const Point& p : pts) {
        QJsonArray pt = {QJsonValue(static_cast<double>(p.row + 1)), QJsonValue(p.v)};
        allPts.append(QJsonValue(pt));
        if (p.v < lo || p.v > hi) {
            badPts.append(QJsonValue(pt));
            outlierRows << (QStringList{QString::number(p.row + 1), QString::number(p.v, 'g', 6)});
        }
    }
    r.summaryTable << (QStringList{QStringLiteral("异常数"), QString::number(outlierRows.size())});
    if (!outlierRows.isEmpty()) {
        r.summaryTable << (QStringList{QStringLiteral("行号"), QStringLiteral("数值")});
        for (const QStringList& row : outlierRows) r.summaryTable << row;
    }

    r.chartData = QJsonObject{
        {QStringLiteral("type"), QStringLiteral("scatter")},
        {QStringLiteral("series"), QJsonArray{
            QJsonObject{{QStringLiteral("name"), colName}, {QStringLiteral("points"), allPts}},
            QJsonObject{{QStringLiteral("name"), QStringLiteral("异常值")}, {QStringLiteral("points"), badPts}}
        }}
    }.toVariantMap();

    if (outlierRows.isEmpty()) {
        r.textConclusion = QStringLiteral("%1：%2 法未检出异常值（%3 个有效数值均在 %4 ~ %5 内）。")
            .arg(colName, methodName).arg(pts.size())
            .arg(QString::number(lo, 'g', 4)).arg(QString::number(hi, 'g', 4));
    } else {
        r.textConclusion = QStringLiteral("%1：%2 法检出 %3 个异常值（行号 %4）。")
            .arg(colName, methodName).arg(outlierRows.size())
            .arg([&]{ QStringList s; for (const QStringList& row : outlierRows) s << row.first(); return s.join(QStringLiteral(", ")); }());
        r.warnings << QStringLiteral("异常值仅提示可能的数据错误，请结合实验记录确认后再处理");
    }

    return r;
}
