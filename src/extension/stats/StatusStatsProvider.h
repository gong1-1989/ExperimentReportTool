#pragma once

#include "extension/StatsProvider.h"

/// 报告状态分布（饼图：数量 + 占比）
class StatusStatsProvider : public StatsProvider {
public:
    QString providerId() const override { return QStringLiteral("status_distribution"); }
    QString displayName() const override { return QStringLiteral("报告状态分布"); }
    QString description() const override
    {
        return QStringLiteral("按当前状态统计报告数量与占比（草稿/待审核/待审批/已审批/已归档）");
    }
    QStringList supportedDimensions() const override { return {QStringLiteral("status")}; }
    StatsResult compute(const StatsScope& scope, QString* error) const override;
};
