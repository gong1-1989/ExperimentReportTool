#pragma once

#include "extension/StatsProvider.h"

/// 工作量统计：按创建人聚合报告数 / 审批通过数 / 被退回数（不含字数）
class WorkloadStatsProvider : public StatsProvider {
public:
    QString providerId() const override { return QStringLiteral("workload"); }
    QString displayName() const override { return QStringLiteral("工作量统计"); }
    QString description() const override
    {
        return QStringLiteral("按创建人统计报告数、审批通过数、被退回数");
    }
    QStringList supportedDimensions() const override { return {QStringLiteral("user")}; }
    StatsResult compute(const StatsScope& scope, QString* error) const override;
};
