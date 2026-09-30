#pragma once
// ===========================================================================
// 报表统计契约（G 域）
// ---------------------------------------------------------------------------
// 与 ExportAdapter / DataAnalyzer 同风格的扩展契约：
//   主程序负责数据获取（Repository 查询），提供者只做聚合计算与图表数据组织。
//   提供者不持有数据库连接，纯计算层 —— 便于内置与动态库插件并存。
// ===========================================================================

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <memory>

/// 统计查询范围（主程序构造，提供者只读）
struct StatsScope {
    qint64  projectId = -1;      ///< 项目过滤（<=0 表示全部项目）
    qint64  groupId   = -1;      ///< 组过滤（>0 时仅统计该组成员创建的报告，组长视角）
    qint64  userId    = -1;      ///< 用户过滤（>0 时仅统计该用户创建的报告）
    QDate   from;                ///< 创建时间起始（无效日期 = 不限）
    QDate   to;                  ///< 创建时间截止（无效日期 = 不限）
};

/// 一个维度行（如某个状态 / 某位用户 / 某个时间段）
struct StatsRow {
    QString           name;      ///< 维度值显示名
    QList<QVariant>   metrics;   ///< 指标值（与 StatsResult::metricHeaders 一一对应）
};

/// 统计结果（提供者输出）
struct StatsResult {
    QString          providerId;     ///< 提供者 ID
    QString          title;          ///< 报表标题
    QStringList      metricHeaders;  ///< 指标列头（第一列固定为维度列，此处不含维度列名）
    QList<StatsRow>  rows;           ///< 数据行
    QVariantMap      chartData;      ///< 图表数据：type(pie/bar/line) + series[{name, points[[x,y]...]}]
    QString          textConclusion; ///< 文本结论（本地规则生成）
    QStringList      warnings;       ///< 提醒（如数据不足）
};

/**
 * @brief 统计提供者契约
 *
 * 内置提供者直接继承并静态注册；动态库插件可声明实现该契约后经 PluginManager 分发。
 * 纯抽象类（非 QObject），与 DataAnalyzer 保持一致。
 */
class StatsProvider {
public:
    virtual ~StatsProvider() = default;

    virtual QString providerId() const = 0;          ///< 唯一 ID（注册键）
    virtual QString displayName() const = 0;         ///< UI 显示名
    virtual QString description() const = 0;         ///< 说明
    virtual QStringList supportedDimensions() const = 0;  ///< 支持的维度（project/user/status/time）
    virtual StatsResult compute(const StatsScope& scope, QString* error) const = 0; ///< 聚合计算
};

using StatsProviderPtr = std::shared_ptr<StatsProvider>;
