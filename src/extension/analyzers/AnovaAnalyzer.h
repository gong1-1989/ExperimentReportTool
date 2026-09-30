/**
 * @file AnovaAnalyzer.h
 * @brief 单因素方差分析器（内置注册：每列一组，比较组间差异）
 */

#ifndef ANOVA_ANALYZER_H
#define ANOVA_ANALYZER_H

#include "../DataAnalyzer.h"

class AnovaAnalyzer : public DataAnalyzer
{
public:
    QString analyzerId() const override { return QStringLiteral("anova"); }
    QString displayName() const override { return QStringLiteral("单因素方差分析"); }
    QString description() const override { return QStringLiteral("多列（每组一列）均值差异检验"); }
    int requiredColumns() const override { return -1; }
    AnalysisResult analyze(const QList<QVariantList>& rows,
                           const QStringList& columnNames,
                           const QVariantMap& params,
                           QString* error) const override;
};

#endif // ANOVA_ANALYZER_H
