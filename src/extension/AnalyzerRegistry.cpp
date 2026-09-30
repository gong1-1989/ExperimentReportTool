#include "AnalyzerRegistry.h"
#include "DataAnalyzer.h"
#include "analyzers/DescriptiveStatsAnalyzer.h"
#include "analyzers/TTestAnalyzer.h"
#include "analyzers/AnovaAnalyzer.h"
#include "analyzers/LinearRegressionAnalyzer.h"
#include "analyzers/OutlierAnalyzer.h"

AnalyzerRegistry* AnalyzerRegistry::s_instance = nullptr;

AnalyzerRegistry& AnalyzerRegistry::instance()
{
    if (!s_instance) s_instance = new AnalyzerRegistry();
    return *s_instance;
}

void AnalyzerRegistry::registerAnalyzer(DataAnalyzer* analyzer)
{
    if (!analyzer || m_analyzers.contains(analyzer)) return;
    m_analyzers.append(analyzer);
}

DataAnalyzer* AnalyzerRegistry::analyzerById(const QString& id) const
{
    for (DataAnalyzer* a : m_analyzers) {
        if (a->analyzerId() == id) return a;
    }
    return nullptr;
}

QList<DataAnalyzer*> AnalyzerRegistry::allAnalyzers() const
{
    return m_analyzers;
}

void AnalyzerRegistry::ensureBuiltinAnalyzers()
{
    static bool done = false;
    if (done) return;
    done = true;
    static DescriptiveStatsAnalyzer s_desc;
    static TTestAnalyzer s_ttest;
    static AnovaAnalyzer s_anova;
    static LinearRegressionAnalyzer s_reg;
    static OutlierAnalyzer s_outlier;
    registerAnalyzer(&s_desc);
    registerAnalyzer(&s_ttest);
    registerAnalyzer(&s_anova);
    registerAnalyzer(&s_reg);
    registerAnalyzer(&s_outlier);
}
