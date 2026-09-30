/**
 * @file AnalyzerRegistry.h
 * @brief 数据分析器注册表（B 域）
 */

#ifndef ANALYZER_REGISTRY_H
#define ANALYZER_REGISTRY_H

#include <QList>
#include <QString>

class DataAnalyzer;

class AnalyzerRegistry
{
public:
    static AnalyzerRegistry& instance();

    void registerAnalyzer(DataAnalyzer* analyzer);

    /// 按 ID 查找（找不到返回 nullptr）
    DataAnalyzer* analyzerById(const QString& id) const;

    /// 全部分析器（注册顺序）
    QList<DataAnalyzer*> allAnalyzers() const;

    /// 惰性注册内置分析器（描述统计/t检验/ANOVA/回归/异常值）
    void ensureBuiltinAnalyzers();

private:
    AnalyzerRegistry() = default;
    static AnalyzerRegistry* s_instance;
    QList<DataAnalyzer*> m_analyzers;
};

#endif // ANALYZER_REGISTRY_H
