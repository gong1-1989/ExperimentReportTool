/**
 * @file CoreService.h
 * @brief 核心服务接口头文件
 *
 * CoreService 是插件与主程序之间的桥梁，提供数据库、日志、设置、
 * 事件总线等核心服务的访问接口。插件通过此接口与主程序交互，
 * 不需要直接依赖具体实现类。
 */

#ifndef CORE_SERVICE_H
#define CORE_SERVICE_H

#include <QString>
#include <QVariant>
#include <QObject>

// 前向声明
class DatabaseManager;
class Logger;
class QSettings;
class EventBus;

/**
 * @brief 核心服务接口
 *
 * 插件在 initialize() 时接收此接口，通过它访问主程序提供的核心服务。
 *
 * @code
 * bool MyPlugin::initialize(CoreService* core) {
 *     core->logger()->info("MyPlugin initialized");
 *     core->settings()->setValue("myplugin/enabled", true);
 *     return true;
 * }
 * @endcode
 */
class CoreService
{
public:
    virtual ~CoreService() = default;

    // ========================================================================
    // 数据库服务
    // ========================================================================

    /**
     * @brief 获取数据库管理器
     * @return 数据库管理器实例指针
     */
    virtual DatabaseManager* database() const = 0;

    // ========================================================================
    // 日志服务
    // ========================================================================

    /**
     * @brief 获取日志器
     * @return 日志器实例指针
     */
    virtual Logger* logger() const = 0;

    // ========================================================================
    // 设置服务
    // ========================================================================

    /**
     * @brief 获取应用程序设置
     * @return QSettings 实例指针
     */
    virtual QSettings* settings() const = 0;

    /**
     * @brief 获取插件专属设置
     * @param pluginId 插件 IID
     * @return 该插件的设置组（已自动定位到插件命名空间）
     */
    virtual QSettings* pluginSettings(const QString& pluginId) const = 0;

    // ========================================================================
    // 事件总线
    // ========================================================================

    /**
     * @brief 获取事件总线
     * @return 事件总线实例指针
     */
    virtual EventBus* eventBus() const = 0;

    // ========================================================================
    // 应用信息
    // ========================================================================

    /**
     * @brief 获取应用程序数据目录
     * @return 数据目录路径
     */
    virtual QString dataDirectory() const = 0;

    /**
     * @brief 获取插件目录
     * @return 插件目录路径
     */
    virtual QString pluginDirectory() const = 0;

    /**
     * @brief 获取应用程序版本
     * @return 版本字符串
     */
    virtual QString appVersion() const = 0;
};

#endif // CORE_SERVICE_H
