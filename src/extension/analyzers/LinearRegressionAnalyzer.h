/**
 * @file LinearRegressionAnalyzer.h
 * @brief 线性回归分析器（内置注册：最小二乘 + 散点/拟合线图）
 */

#ifndef LINEAR_REGRESSION_ANALYZER_H
#define LINEAR_REGRESSION_ANALYZER_H

#include "../DataAnalyzer.h"

class LinearRegressionAnalyzer : public DataAnalyzer
{
public:
    QString analyzerId() const override { return QStringLiteral("linear-regression"); }
    QString displayName() const override { return QStringLiteral("线性回归"); }
    QString description() const override { return QStringLiteral("最小二乘拟合：斜率/截距/相关系数/显著性"); }
    int requiredColumns() const override { return 2; }
    AnalysisResult analyze(const QList<QVariantList>& rows,
                           const QStringList& columnNames,
                           const QVariantMap& params,
                           QString* error) const override;
};

#endif // LINEAR_REGRESSION_ANALYZER_H
