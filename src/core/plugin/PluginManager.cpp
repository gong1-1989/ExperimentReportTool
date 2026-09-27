/**
 * @file PluginManager.cpp
 * @brief 插件管理器实现文件
 */

#include "PluginManager.h"
#include "CoreService.h"
#include "core/utils/Logger.h"

#include <QPluginLoader>
#include <QDir>
#include <QDebug>

// ============================================================================
// 构造与析构
// ============================================================================

PluginManager::PluginManager(QObject* parent)
    : QObject(parent)
    , m_core(nullptr)
{
}

PluginManager::~PluginManager()
{
    unloadAllPlugins();
}

// ============================================================================
// 配置
// ============================================================================

void PluginManager::setCoreService(CoreService* core)
{
    m_core = core;
}

void PluginManager::addPluginDirectory(const QString& path)
{
    if (!m_pluginDirs.contains(path)) {
        m_pluginDirs.append(path);
    }
}

void PluginManager::registerBuiltinPlugin(PluginInterface* plugin)
{
    if (!plugin) return;

    // 检查是否已注册
    for (PluginInterface* p : m_plugins) {
        if (p->iid() == plugin->iid()) {
            return;  // 已存在
        }
    }

    // 初始化内置插件
    if (m_core && plugin->initialize(m_core)) {
        m_plugins.append(plugin);

        PluginInfo info;
        info.iid = plugin->iid();
        info.name = plugin->name();
        info.version = plugin->version();
        info.description = plugin->description();
        info.author = plugin->author();
        info.category = plugin->category();
        info.filePath = QString();
        info.isBuiltin = true;
        info.isLoaded = true;
        info.isEnabled = true;
        info.instance = plugin;
        m_pluginInfos.append(info);

        Logger::instance().info(QString("内置插件已加载: %1 (%2)").arg(plugin->name(), plugin->version()));
        emit pluginLoaded(plugin->iid());
    }
}

// ============================================================================
// 加载与卸载
// ============================================================================

int PluginManager::loadAllPlugins()
{
    int count = 0;

    // 1. 扫描所有插件目录
    for (const QString& dirPath : m_pluginDirs) {
        QDir dir(dirPath);
        if (dir.exists()) {
            scanDirectory(dir);
        }
    }

    // 2. 按依赖顺序排序
    sortByDependencies();

    // 3. 初始化所有已加载的插件
    for (PluginInterface* plugin : m_plugins) {
        plugin->initialized();
    }

    count = m_plugins.size();
    Logger::instance().info(QString("插件加载完成，共 %1 个插件").arg(count));
    emit allPluginsLoaded();

    return count;
}

void PluginManager::unloadAllPlugins()
{
    // 逆序卸载（先加载的后卸载）
    for (int i = m_plugins.size() - 1; i >= 0; --i) {
        PluginInterface* plugin = m_plugins[i];
        if (plugin) {
            plugin->shutdown();
            emit pluginUnloaded(plugin->iid());
        }
    }

    // 卸载动态库插件
    for (QPluginLoader* loader : m_loaders) {
        loader->unload();
        delete loader;
    }
    m_loaders.clear();

    m_plugins.clear();
    m_pluginInfos.clear();
}

bool PluginManager::loadPlugin(const QString& iid)
{
    // 查找已扫描但未加载的插件
    for (const PluginInfo& info : m_pluginInfos) {
        if (info.iid == iid && !info.isLoaded) {
            // 尝试从文件加载
            if (!info.filePath.isEmpty()) {
                return tryLoadPlugin(info.filePath);
            }
        }
    }
    return false;
}

void PluginManager::unloadPlugin(const QString& iid)
{
    // 找到插件
    for (int i = 0; i < m_plugins.size(); ++i) {
        if (m_plugins[i]->iid() == iid) {
            m_plugins[i]->shutdown();
            emit pluginUnloaded(iid);

            // 如果是动态库插件，卸载库
            for (int j = 0; j < m_loaders.size(); ++j) {
                QObject* instance = m_loaders[j]->instance();
                QObject* pluginObj = dynamic_cast<QObject*>(m_plugins[i]);
                if (instance == pluginObj) {
                    m_loaders[j]->unload();
                    delete m_loaders[j];
                    m_loaders.removeAt(j);
                    break;
                }
            }

            m_plugins.removeAt(i);

            // 更新信息
            for (PluginInfo& info : m_pluginInfos) {
                if (info.iid == iid) {
                    info.isLoaded = false;
                    info.instance = nullptr;
                }
            }
            break;
        }
    }
}

// ============================================================================
// 查询
// ============================================================================

QList<PluginInterface*> PluginManager::loadedPlugins() const
{
    return m_plugins;
}

PluginInterface* PluginManager::pluginByIid(const QString& iid) const
{
    for (PluginInterface* p : m_plugins) {
        if (p->iid() == iid) {
            return p;
        }
    }
    return nullptr;
}

QList<PluginInfo> PluginManager::allPluginInfo() const
{
    return m_pluginInfos;
}

bool PluginManager::isLoaded(const QString& iid) const
{
    return pluginByIid(iid) != nullptr;
}

// ============================================================================
// 内部方法
// ============================================================================

void PluginManager::scanDirectory(const QDir& dir)
{
    // 查找所有动态库文件
    const QStringList filters = {
        QStringLiteral("*.dll"),   // Windows
        QStringLiteral("*.so"),    // Linux
        QStringLiteral("*.dylib")  // macOS
    };

    const QFileInfoList files = dir.entryInfoList(filters, QDir::Files);

    for (const QFileInfo& file : files) {
        tryLoadPlugin(file.absoluteFilePath());
    }
}

bool PluginManager::tryLoadPlugin(const QString& filePath)
{
    QPluginLoader* loader = new QPluginLoader(filePath);

    // 检查是否是 Qt 插件
    if (!loader->metaData().value("IID").isString()) {
        delete loader;
        return false;
    }

    // 加载插件实例
    QObject* instance = loader->instance();
    if (!instance) {
        Logger::instance().warning(QString("插件加载失败: %1 - %2").arg(filePath, loader->errorString()));
        delete loader;
        return false;
    }

    // 检查是否实现了 PluginInterface
    PluginInterface* plugin = qobject_cast<PluginInterface*>(instance);
    if (!plugin) {
        Logger::instance().warning(QString("插件未实现 PluginInterface: %1").arg(filePath));
        loader->unload();
        delete loader;
        return false;
    }

    // 检查是否已加载
    if (isLoaded(plugin->iid())) {
        Logger::instance().info(QString("插件已存在，跳过: %1").arg(plugin->name()));
        loader->unload();
        delete loader;
        return false;
    }

    // 检查依赖
    if (!checkDependencies(plugin)) {
        Logger::instance().warning(QString("插件依赖不满足: %1").arg(plugin->name()));
        loader->unload();
        delete loader;
        return false;
    }

    // 初始化插件
    if (m_core && plugin->initialize(m_core)) {
        m_plugins.append(plugin);
        m_loaders.append(loader);

        PluginInfo info;
        info.iid = plugin->iid();
        info.name = plugin->name();
        info.version = plugin->version();
        info.description = plugin->description();
        info.author = plugin->author();
        info.category = plugin->category();
        info.filePath = filePath;
        info.isBuiltin = false;
        info.isLoaded = true;
        info.isEnabled = true;
        info.instance = plugin;
        m_pluginInfos.append(info);

        Logger::instance().info(QString("插件已加载: %1 (%2)").arg(plugin->name(), plugin->version()));
        emit pluginLoaded(plugin->iid());
        return true;
    }

    loader->unload();
    delete loader;
    return false;
}

bool PluginManager::checkDependencies(PluginInterface* plugin) const
{
    const QStringList deps = plugin->dependencies();
    for (const QString& dep : deps) {
        if (!isLoaded(dep)) {
            return false;
        }
    }
    return true;
}

void PluginManager::sortByDependencies()
{
    // 简单的拓扑排序（按依赖数量排序）
    std::sort(m_plugins.begin(), m_plugins.end(),
        [](PluginInterface* a, PluginInterface* b) {
            return a->dependencies().size() < b->dependencies().size();
        });
}
