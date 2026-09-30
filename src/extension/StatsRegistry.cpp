#include "StatsRegistry.h"

#include "stats/StatusStatsProvider.h"
#include "stats/WorkloadStatsProvider.h"
#include "stats/TrendStatsProvider.h"

StatsRegistry& StatsRegistry::instance()
{
    static StatsRegistry s_registry;
    return s_registry;
}

void StatsRegistry::registerProvider(const StatsProviderPtr& provider)
{
    if (!provider) return;
    for (const StatsProviderPtr& p : m_providers) {
        if (p->providerId() == provider->providerId()) return;  // 已注册则跳过（内置优先）
    }
    m_providers.append(provider);
}

QList<StatsProviderPtr> StatsRegistry::providers() const
{
    return m_providers;
}

StatsProviderPtr StatsRegistry::providerById(const QString& id) const
{
    for (const StatsProviderPtr& p : m_providers) {
        if (p->providerId() == id) return p;
    }
    return nullptr;
}

void StatsRegistry::ensureBuiltinProviders()
{
    static const bool s_ready = []() {
        StatsRegistry& r = StatsRegistry::instance();
        r.registerProvider(std::make_shared<StatusStatsProvider>());
        r.registerProvider(std::make_shared<WorkloadStatsProvider>());
        r.registerProvider(std::make_shared<TrendStatsProvider>());
        return true;
    }();
    Q_UNUSED(s_ready);
}
