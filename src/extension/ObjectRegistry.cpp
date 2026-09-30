#include "ObjectRegistry.h"

#include "objects/AttachmentCardProvider.h"
#include "objects/MediaRefProvider.h"

ObjectRegistry& ObjectRegistry::instance()
{
    static ObjectRegistry s_registry;
    return s_registry;
}

void ObjectRegistry::registerProvider(const DocumentObjectProviderPtr& provider)
{
    if (!provider) return;
    for (const DocumentObjectProviderPtr& p : m_providers) {
        if (p->objectTypeId() == provider->objectTypeId()) return;  // 已注册则跳过（内置优先）
    }
    m_providers.append(provider);
}

QList<DocumentObjectProviderPtr> ObjectRegistry::providers() const
{
    return m_providers;
}

DocumentObjectProviderPtr ObjectRegistry::providerById(const QString& id)
{
    ensureBuiltinProviders();  // 渲染入口为纯函数（无 UI 入口），查询时自确保注册
    for (const DocumentObjectProviderPtr& p : m_providers) {
        if (p->objectTypeId() == id) return p;
    }
    return nullptr;
}

void ObjectRegistry::ensureBuiltinProviders()
{
    static const bool s_ready = []() {
        ObjectRegistry& r = ObjectRegistry::instance();
        r.registerProvider(std::make_shared<AttachmentCardProvider>());
        r.registerProvider(std::make_shared<MediaRefProvider>());
        return true;
    }();
    Q_UNUSED(s_ready);
}
