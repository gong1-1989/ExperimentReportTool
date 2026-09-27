/**
 * @file PluginManager.h
 * @brief 插件管理器头文件
 *
 * 负责插件的扫描、加载、初始化和生命周期管理。
 * 支持动态加载 Qt 插件（.dll/.so）和内置插件。
 */

#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <QObject>
#include <QList>
#include <QStringList>
#include <QDir>

#include "PluginInterface.h"

// 前向声明
class CoreService;
class QPluginLoader;

/**
 * @brief 插件信息结构体
 */
struct PluginInfo {
    QString iid;           ///< 插件唯一标识符
    QString name;          ///< 插件名称
    QString version;       ///< 插件版本
    QString description;   ///< 插件描述
    QString author;        ///< 插件作者
    QString category;      ///< 插件类别
    QString filePath;      ///< 插件文件路径（内置插件为空）
    bool isBuiltin;        ///< 是否为内置插件
    bool isLoaded;         ///< 是否已加载
    bool isEnabled;        ///< 是否启用
    PluginInterface* instance;  ///< 插件实例（未加载时为 nullptr）
};

/**
 * @brief 插件管理器
 *
 * 使用方式：
 * @code
 *   PluginManager manager;
 *   manager.setCoreService(coreService);
 *   manager.addPluginDirectory("plugins");
 *   manager.loadAllPlugins();
 * @endcode
 */
class PluginManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit PluginManager(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~PluginManager() override;

    // ========================================================================
    // 配置
    // ========================================================================

    /**
     * @brief 设置核心服务
     * @param core 核心服务接口
     */
    void setCoreService(CoreService* core);

    /**
     * @brief 添加插件搜索目录
     * @param path 目录路径
     */
    void addPluginDirectory(const QString& path);

    /**
     * @brief 注册内置插件
     * @param plugin 插件实例
     */
    void registerBuiltinPlugin(PluginInterface* plugin);

    // ========================================================================
    // 加载与卸载
    // ========================================================================

    /**
     * @brief 加载所有插件（扫描目录 + 内置插件）
     * @return 成功加载的插件数量
     */
    int loadAllPlugins();

    /**
     * @brief 卸载所有插件
     */
    void unloadAllPlugins();

    /**
     * @brief 加载单个插件
     * @param iid 插件 IID
     * @return 成功返回 true
     */
    bool loadPlugin(const QString& iid);

    /**
     * @brief 卸载单个插件
     * @param iid 插件 IID
     */
    void unloadPlugin(const QString& iid);

    // ========================================================================
    // 查询
    // ========================================================================

    /**
     * @brief 获取所有已加载的插件
     * @return 插件实例列表
     */
    QList<PluginInterface*> loadedPlugins() const;

    /**
     * @brief 获取指定类型的插件
     * @tparam T 插件接口类型
     * @return 该类型的插件列表
     */
    template<typename T>
    QList<T*> pluginsOfType() const {
        QList<T*> result;
        for (PluginInterface* p : m_plugins) {
            // PluginInterface 不是 QObject 子类，使用 dynamic_cast
            T* typed = dynamic_cast<T*>(p);
            if (typed) result.append(typed);
        }
        return result;
    }

    /**
     * @brief 根据 IID 获取插件实例
     * @param iid 插件 IID
     * @return 插件实例，未找到返回 nullptr
     */
    PluginInterface* pluginByIid(const QString& iid) const;

    /**
     * @brief 获取所有插件信息
     * @return 插件信息列表
     */
    QList<PluginInfo> allPluginInfo() const;

    /**
     * @brief 检查插件是否已加载
     * @param iid 插件 IID
     * @return 已加载返回 true
     */
    bool isLoaded(const QString& iid) const;

signals:
    /**
     * @brief 插件加载完成信号
     * @param iid 插件 IID
     */
    void pluginLoaded(const QString& iid);

    /**
     * @brief 插件卸载信号
     * @param iid 插件 IID
     */
    void pluginUnloaded(const QString& iid);

    /**
     * @brief 所有插件加载完成信号
     */
    void allPluginsLoaded();

private:
    // ========================================================================
    // 内部方法
    // ========================================================================

    /// 扫描目录中的插件
    void scanDirectory(const QDir& dir);

    /// 尝试加载插件文件
    bool tryLoadPlugin(const QString& filePath);

    /// 检查依赖是否满足
    bool checkDependencies(PluginInterface* plugin) const;

    /// 按依赖顺序排序
    void sortByDependencies();

    // ========================================================================
    // 成员变量
    // ========================================================================
    CoreService* m_core;                    ///< 核心服务
    QStringList m_pluginDirs;               ///< 插件搜索目录
    QList<PluginInterface*> m_plugins;      ///< 已加载的插件
    QList<QPluginLoader*> m_loaders;        ///< 插件加载器（动态库）
    QList<PluginInfo> m_pluginInfos;        ///< 插件信息缓存
};

#endif // PLUGIN_MANAGER_H
