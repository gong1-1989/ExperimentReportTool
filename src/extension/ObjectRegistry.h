#pragma once
// ===========================================================================
// 文档对象提供者注册表（D 域）
// ===========================================================================

#include "extension/DocumentObjectProvider.h"

class ObjectRegistry {
public:
    static ObjectRegistry& instance();

    void registerProvider(const DocumentObjectProviderPtr& provider);
    QList<DocumentObjectProviderPtr> providers() const;          ///< 按注册顺序
    DocumentObjectProviderPtr providerById(const QString& id);  ///< 查询时自确保内置注册

    /// 惰性注册全部内置文档对象提供者（首次调用时执行一次）
    void ensureBuiltinProviders();

private:
    ObjectRegistry() = default;
    QList<DocumentObjectProviderPtr> m_providers;
};
