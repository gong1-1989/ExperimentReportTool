/**
 * @file DescriptiveStatsAnalyzer.h
 * @brief 描述统计分析器（内置注册）
 */

#ifndef DESCRIPTIVE_STATS_ANALYZER_H
#define DESCRIPTIVE_STATS_ANALYZER_H

#include "../DataAnalyzer.h"

class DescriptiveStatsAnalyzer : public DataAnalyzer
{
public:
    QString analyzerId() const override { return QStringLiteral("descriptive-stats"); }
    QString displayName() const override { return QStringLiteral("描述统计"); }
    QString description() const override { return QStringLiteral("样本数/均值/中位数/标准差/极差/变异系数"); }
    int requiredColumns() const override { return 1; }
    AnalysisResult analyze(const QList<QVariantList>& rows,
                           const QStringList& columnNames,
                           const QVariantMap& params,
                           QString* error) const override;
};

#endif // DESCRIPTIVE_STATS_ANALYZER_H
