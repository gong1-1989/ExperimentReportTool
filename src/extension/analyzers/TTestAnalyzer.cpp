#include "TTestAnalyzer.h"
#include "StatsMath.h"

#include <vector>

AnalysisResult TTestAnalyzer::analyze(const QList<QVariantList>& rows,
                                     const QStringList& columnNames,
                                     const QVariantMap& params,
                                     QString* error) const
{
    AnalysisResult r;
    r.analyzerId = analyzerId();
    const int colA = params.value(QStringLiteral("columnIndex")).toInt();
    const int colB = params.value(QStringLiteral("secondColumnIndex")).toInt();
    const bool paired = params.value(QStringLiteral("paired")).toBool();
    if (colA < 0 || colB < 0) {
        if (error) *error = QStringLiteral("请选择两列数值数据");
        return r;
    }
    const QString nameA = colA < columnNames.size() ? columnNames.at(colA) : QStringLiteral("列%1").arg(colA + 1);
    const QString nameB = colB < columnNames.size() ? columnNames.at(colB) : QStringLiteral("列%1").arg(colB + 1);
    r.title = QStringLiteral("t 检验：%1 vs %2（%3）")
        .arg(nameA, nameB, paired ? QStringLiteral("配对") : QStringLiteral("独立样本"));

    std::vector<double> a, b;
    for (const QVariantList& row : rows) {
        if (colA < row.size()) {
            bool ok = false;
            const double d = StatsMath::toDouble(row.at(colA), &ok);
            if (ok) a.push_back(d);
        }
        if (colB < row.size()) {
            bool ok = false;
            const double d = StatsMath::toDouble(row.at(colB), &ok);
            if (ok) b.push_back(d);
        }
    }
    if (a.size() < 2 || b.size() < 2) {
        if (error) *error = QStringLiteral("两列有效数值均需至少 2 个");
        return r;
    }

    double t = 0.0, df = 0.0;
    QString method;
    if (paired) {
        // 配对：逐行差值（取两列都有效的行）
        std::vector<double> diffs;
        const size_t n = qMin(a.size(), b.size());
        for (size_t i = 0; i < n; ++i) diffs.push_back(a[i] - b[i]);
        if (diffs.size() < 2) { if (error) *error = QStringLiteral("配对数据不足"); return r; }
        const double m = StatsMath::mean(diffs);
        const double sd = StatsMath::stddev(diffs);
        df = diffs.size() - 1;
        t = sd == 0.0 ? 0.0 : m / (sd / std::sqrt(static_cast<double>(diffs.size())));
        method = QStringLiteral("配对");
        r.summaryTable = {
            {QStringLiteral("项目"), QStringLiteral("数值")},
            {QStringLiteral("配对数"), QString::number(diffs.size())},
            {QStringLiteral("差值均值"), QString::number(m, 'g', 6)},
            {QStringLiteral("差值标准差"), QString::number(sd, 'g', 6)},
        };
    } else {
        // 独立样本（Student，方差齐性）
        const double m1 = StatsMath::mean(a), m2 = StatsMath::mean(b);
        const double s1 = StatsMath::stddev(a), s2 = StatsMath::stddev(b);
        const double sp2 = ((a.size() - 1) * s1 * s1 + (b.size() - 1) * s2 * s2)
                         / (a.size() + b.size() - 2);
        df = a.size() + b.size() - 2;
        t = sp2 == 0.0 ? 0.0 : (m1 - m2) / std::sqrt(sp2 * (1.0 / a.size() + 1.0 / b.size()));
        method = QStringLiteral("独立样本");
        r.summaryTable = {
            {QStringLiteral("项目"), QStringLiteral("数值")},
            {QStringLiteral("组1 样本数/均值"), QStringLiteral("%1 / %2").arg(a.size()).arg(QString::number(m1, 'g', 6))},
            {QStringLiteral("组2 样本数/均值"), QStringLiteral("%1 / %2").arg(b.size()).arg(QString::number(m2, 'g', 6))},
        };
        r.warnings << QStringLiteral("采用方差齐性假设（Student t）；若两列方差悬殊请谨慎解读");
    }

    const double p = StatsMath::tTwoTailP(t, df);
    r.summaryTable << (QStringList{QStringLiteral("t 值"), QString::number(t, 'g', 6)});
    r.summaryTable << (QStringList{QStringLiteral("自由度"), QString::number(df)});
    r.summaryTable << (QStringList{QStringLiteral("p 值（双尾）"), QString::number(p, 'g', 4)});
    r.summaryTable << (QStringList{QStringLiteral("结论"), p < 0.05 ? QStringLiteral("差异显著 (p<0.05)") : QStringLiteral("差异不显著 (p≥0.05)")});

    r.textConclusion = QStringLiteral("%1 t 检验：%2 vs %3，t=%4，df=%5，p=%6%7。")
        .arg(method, nameA, nameB)
        .arg(QString::number(t, 'g', 4)).arg(df).arg(QString::number(p, 'g', 4))
        .arg(p < 0.05 ? QStringLiteral("<0.05，均值差异显著") : QStringLiteral("≥0.05，均值差异不显著"));

    if (paired && a.size() != b.size())
        r.warnings << QStringLiteral("两列有效数据行数不同，配对按较短列对齐");

    return r;
}
