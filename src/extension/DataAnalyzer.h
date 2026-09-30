/**
 * @file DataAnalyzer.h
 * @brief 数据分析契约（B 域：数据分析）
 *
 * 分析器接收数据行快照 + 列名 + 参数，输出统一 AnalysisResult。
 * 插件不接触数据库、不接触 UI；统计计算本地执行（可离线、可审计）。
 * AI 解读层（I 域）可基于 AnalysisResult 重新生成自然语言结论，两层解耦。
 */

#ifndef DATA_ANALYZER_H
#define DATA_ANALYZER_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QVariant>
#include <QVariantMap>

/**
 * @brief 统一分析结果（主程序通用对话框渲染）
 */
struct AnalysisResult {
    QString analyzerId;          ///< 分析器标识（如 "t-test"）
    QString title;               ///< 结果标题（含具体列名）
    QList<QStringList> summaryTable;  ///< 数值结果表（第一行为表头）
    QVariantMap chartData;       ///< 图表数据（可为空）：
                                 ///<   type: "scatter"/"line"/"bar"
                                 ///<   series: [{"name","points":[[x,y],...]}]
    QString textConclusion;      ///< 自然语言结论（本地规则生成）
    QStringList warnings;        ///< 提醒（样本量不足/方差悬殊等）
};

/**
 * @brief 数据分析器抽象契约
 *
 * @param rows 数据行快照（每行按列顺序的 QVariant 值）
 * @param columnNames 列名（用于结论文本与结果表头）
 * @param params 分析参数（如列索引、显著性水平等）
 * @param error 失败时写入错误信息（成功可为空）
 */
class DataAnalyzer
{
public:
    virtual ~DataAnalyzer() = default;

    /// 唯一标识（如 "descriptive-stats"）
    virtual QString analyzerId() const = 0;
    /// 显示名称（如 "描述统计"）
    virtual QString displayName() const = 0;
    /// 适用说明（UI 展示给用户）
    virtual QString description() const = 0;
    /// 需要选择几列（1=单列，2=双列，-1=多列）
    virtual int requiredColumns() const = 0;

    virtual AnalysisResult analyze(const QList<QVariantList>& rows,
                                   const QStringList& columnNames,
                                   const QVariantMap& params,
                                   QString* error) const = 0;
};

#endif // DATA_ANALYZER_H
