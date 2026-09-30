#include "LinearRegressionAnalyzer.h"
#include "StatsMath.h"

#include <QJsonArray>
#include <QJsonObject>
#include <vector>

AnalysisResult LinearRegressionAnalyzer::analyze(const QList<QVariantList>& rows,
                                                 const QStringList& columnNames,
                                                 const QVariantMap& params,
                                                 QString* error) const
{
    AnalysisResult r;
    r.analyzerId = analyzerId();
    const int xCol = params.value(QStringLiteral("xColumnIndex")).toInt();
    const int yCol = params.value(QStringLiteral("yColumnIndex")).toInt();
    if (xCol < 0 || yCol < 0) {
        if (error) *error = QStringLiteral("请选择 X、Y 两列数值数据");
        return r;
    }
    const QString xName = xCol < columnNames.size() ? columnNames.at(xCol) : QStringLiteral("列%1").arg(xCol + 1);
    const QString yName = yCol < columnNames.size() ? columnNames.at(yCol) : QStringLiteral("列%1").arg(yCol + 1);
    r.title = QStringLiteral("线性回归：%1 ~ %2").arg(yName, xName);

    // 逐行取 (x, y) 均有效的点
    std::vector<double> xs, ys;
    for (const QVariantList& row : rows) {
        if (xCol >= row.size() || yCol >= row.size()) continue;
        bool okX = false, okY = false;
        const double x = StatsMath::toDouble(row.at(xCol), &okX);
        const double y = StatsMath::toDouble(row.at(yCol), &okY);
        if (okX && okY) { xs.push_back(x); ys.push_back(y); }
    }
    if (xs.size() < 3) {
        if (error) *error = QStringLiteral("有效数据点至少需要 3 个");
        return r;
    }

    const int n = static_cast<int>(xs.size());
    const double mx = StatsMath::mean(xs), my = StatsMath::mean(ys);
    double sxx = 0.0, sxy = 0.0, syy = 0.0;
    for (int i = 0; i < n; ++i) {
        const double dx = xs[i] - mx, dy = ys[i] - my;
        sxx += dx * dx; syy += dy * dy; sxy += dx * dy;
    }
    const double slope = sxx == 0.0 ? 0.0 : sxy / sxx;
    const double intercept = my - slope * mx;
    const double r2 = (sxx == 0.0 || syy == 0.0) ? 0.0 : (sxy * sxy) / (sxx * syy);
    const double corr = std::sqrt(r2) * (sxy >= 0.0 ? 1.0 : -1.0);

    // 斜率显著性（t 检验，df = n-2）
    double p = std::numeric_limits<double>::quiet_NaN();
    double sse = 0.0;
    for (int i = 0; i < n; ++i) {
        const double yHat = slope * xs[i] + intercept;
        sse += (ys[i] - yHat) * (ys[i] - yHat);
    }
    const double mse = sse / (n - 2);
    const double seSlope = std::sqrt(mse / sxx);
    const double t = seSlope == 0.0 ? 0.0 : slope / seSlope;
    p = StatsMath::tTwoTailP(t, n - 2);

    r.summaryTable = {
        {QStringLiteral("项目"), QStringLiteral("数值")},
        {QStringLiteral("样本数"), QString::number(n)},
        {QStringLiteral("斜率"), QString::number(slope, 'g', 6)},
        {QStringLiteral("截距"), QString::number(intercept, 'g', 6)},
        {QStringLiteral("相关系数 r"), QString::number(corr, 'g', 6)},
        {QStringLiteral("决定系数 R²"), QString::number(r2, 'g', 6)},
        {QStringLiteral("斜率 p 值"), QString::number(p, 'g', 4)},
    };

    // 散点 + 拟合线
    QJsonArray pts, linePts;
    for (int i = 0; i < n; ++i)
        pts.append(QJsonValue(QJsonArray{QJsonValue(xs[i]), QJsonValue(ys[i])}));
    double xMin = xs[0], xMax = xs[0];
    for (double x : xs) { xMin = std::min(xMin, x); xMax = std::max(xMax, x); }
    const double pad = (xMax - xMin) * 0.1;
    linePts.append(QJsonValue(QJsonArray{QJsonValue(xMin - pad), QJsonValue(slope * (xMin - pad) + intercept)}));
    linePts.append(QJsonValue(QJsonArray{QJsonValue(xMax + pad), QJsonValue(slope * (xMax + pad) + intercept)}));
    r.chartData = QJsonObject{
        {QStringLiteral("type"), QStringLiteral("scatter")},
        {QStringLiteral("series"), QJsonArray{
            QJsonObject{{QStringLiteral("name"), QStringLiteral("数据点")}, {QStringLiteral("points"), pts}},
            QJsonObject{{QStringLiteral("name"), QStringLiteral("拟合线")}, {QStringLiteral("points"), linePts}}
        }}
    }.toVariantMap();

    r.textConclusion = QStringLiteral("线性回归：%1 = %2×%3 %4 %5，R²=%6，斜率 p=%7%8。")
        .arg(yName, QString::number(slope, 'g', 4), xName,
             intercept >= 0 ? QStringLiteral("+") : QStringLiteral("−"),
             QString::number(std::fabs(intercept), 'g', 4), QString::number(r2, 'g', 4),
             QString::number(p, 'g', 4),
             p < 0.05 ? QStringLiteral("<0.05，回归显著") : QStringLiteral("≥0.05，回归不显著"));

    return r;
}
