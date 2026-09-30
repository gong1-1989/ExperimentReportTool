#include "AnovaAnalyzer.h"
#include "StatsMath.h"

#include <vector>

AnalysisResult AnovaAnalyzer::analyze(const QList<QVariantList>& rows,
                                     const QStringList& columnNames,
                                     const QVariantMap& params,
                                     QString* error) const
{
    AnalysisResult r;
    r.analyzerId = analyzerId();

    QList<int> cols;
    const QVariantList idxList = params.value(QStringLiteral("columnIndexes")).toList();
    for (const QVariant& v : idxList) cols.append(v.toInt());
    if (cols.size() < 2) {
        if (error) *error = QStringLiteral("单因素方差分析至少需要 2 列（每组一列）");
        return r;
    }

    QStringList names;
    std::vector<std::vector<double>> groups;
    std::vector<double> all;
    for (int c : cols) {
        names << (c < columnNames.size() ? columnNames.at(c) : QStringLiteral("列%1").arg(c + 1));
        std::vector<double> g;
        for (const QVariantList& row : rows) {
            if (c >= row.size()) continue;
            bool ok = false;
            const double d = StatsMath::toDouble(row.at(c), &ok);
            if (ok) { g.push_back(d); all.push_back(d); }
        }
        groups.push_back(g);
    }
    for (const auto& g : groups) {
        if (g.size() < 2) { if (error) *error = QStringLiteral("每组有效数值需至少 2 个"); return r; }
    }

    const int k = groups.size();
    const int N = static_cast<int>(all.size());
    const double grand = StatsMath::mean(all);
    double ssb = 0.0, ssw = 0.0;
    for (int i = 0; i < k; ++i) {
        const double mi = StatsMath::mean(groups[i]);
        ssb += groups[i].size() * (mi - grand) * (mi - grand);
        for (double x : groups[i]) ssw += (x - mi) * (x - mi);
    }
    const int df1 = k - 1, df2 = N - k;
    const double msb = ssb / df1, msw = ssw / df2;
    const double F = msw == 0.0 ? 0.0 : msb / msw;
    const double p = StatsMath::fCdf(F, df1, df2);
    const double pTail = 1.0 - p;

    r.title = QStringLiteral("单因素方差分析（%1 组）").arg(k);
    r.summaryTable = {
        {QStringLiteral("组别"), QStringLiteral("样本数"), QStringLiteral("均值")},
    };
    for (int i = 0; i < k; ++i) {
        r.summaryTable << (QStringList{names.at(i), QString::number(groups[i].size()),
                                       QString::number(StatsMath::mean(groups[i]), 'g', 6)});
    }
    r.summaryTable << (QStringList{QStringLiteral(""), QString(), QString()});
    r.summaryTable << (QStringList{QStringLiteral("来源"), QStringLiteral("SS"), QStringLiteral("df"), QStringLiteral("MS"), QStringLiteral("F"), QStringLiteral("p")});
    r.summaryTable << (QStringList{QStringLiteral("组间"), QString::number(ssb, 'g', 6), QString::number(df1),
                                   QString::number(msb, 'g', 6), QString::number(F, 'g', 6), QString::number(pTail, 'g', 4)});
    r.summaryTable << (QStringList{QStringLiteral("组内"), QString::number(ssw, 'g', 6), QString::number(df2),
                                   QString::number(msw, 'g', 6), QString(), QString()});

    r.textConclusion = QStringLiteral("单因素方差分析：%1 组均值比较，F=%2，p=%3%4。")
        .arg(k).arg(QString::number(F, 'g', 4)).arg(QString::number(pTail, 'g', 4))
        .arg(pTail < 0.05 ? QStringLiteral("<0.05，组间差异显著") : QStringLiteral("≥0.05，组间差异不显著"));

    if (pTail < 0.05)
        r.warnings << QStringLiteral("差异显著仅说明至少两组不同，如需两两比较请另行分析（如 t 检验）");

    return r;
}
