/**
 * @file OutlierAnalyzer.h
 * @brief 异常值检测分析器（内置注册，3σ / 箱线图 IQR 法）
 */

#ifndef OUTLIER_ANALYZER_H
#define OUTLIER_ANALYZER_H

#include "../DataAnalyzer.h"

class OutlierAnalyzer : public DataAnalyzer
{
public:
    QString analyzerId() const override { return QStringLiteral("outlier-detection"); }
    QString displayName() const override { return QStringLiteral("异常值检测"); }
    QString description() const override { return QStringLiteral("3σ 或箱线图(IQR)法标记异常值"); }
    int requiredColumns() const override { return 1; }
    AnalysisResult analyze(const QList<QVariantList>& rows,
                           const QStringList& columnNames,
                           const QVariantMap& params,
                           QString* error) const override;
};

#endif // OUTLIER_ANALYZER_H
