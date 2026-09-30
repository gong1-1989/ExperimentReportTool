#pragma once
// ===========================================================================
// 统计提供者注册表（G 域）
// ===========================================================================

#include "StatsProvider.h"

class StatsRegistry {
public:
    static StatsRegistry& instance();

    void registerProvider(const StatsProviderPtr& provider);
    QList<StatsProviderPtr> providers() const;              ///< 按注册顺序
    StatsProviderPtr providerById(const QString& id) const;

    /// 惰性注册全部内置统计提供者（首次调用时执行一次）
    void ensureBuiltinProviders();

private:
    StatsRegistry() = default;
    QList<StatsProviderPtr> m_providers;
};
