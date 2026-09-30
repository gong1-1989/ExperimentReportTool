#pragma once

#include "extension/StatsProvider.h"

/// 时间趋势：按时间段（日/周/月）统计新建报告数与完成数（已审批+已归档）
class TrendStatsProvider : public StatsProvider {
public:
    QString providerId() const override { return QStringLiteral("trend"); }
    QString displayName() const override { return QStringLiteral("时间趋势"); }
    QString description() const override
    {
        return QStringLiteral("按日/周/月统计新建报告数与完成数（已审批+已归档）");
    }
    QStringList supportedDimensions() const override { return {QStringLiteral("time")}; }
    StatsResult compute(const StatsScope& scope, QString* error) const override;
};
