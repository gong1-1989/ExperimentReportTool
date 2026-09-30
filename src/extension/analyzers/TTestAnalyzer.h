/**
 * @file TTestAnalyzer.h
 * @brief t 检验分析器（内置注册：独立样本 / 配对）
 */

#ifndef TTEST_ANALYZER_H
#define TTEST_ANALYZER_H

#include "../DataAnalyzer.h"

class TTestAnalyzer : public DataAnalyzer
{
public:
    QString analyzerId() const override { return QStringLiteral("t-test"); }
    QString displayName() const override { return QStringLiteral("t 检验"); }
    QString description() const override { return QStringLiteral("独立样本或配对样本的均值差异检验"); }
    int requiredColumns() const override { return 2; }
    AnalysisResult analyze(const QList<QVariantList>& rows,
                           const QStringList& columnNames,
                           const QVariantMap& params,
                           QString* error) const override;
};

#endif // TTEST_ANALYZER_H
