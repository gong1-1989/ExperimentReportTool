/**
 * @file CoreServiceImpl.h
 * @brief 核心服务实现类头文件
 *
 * 实现 CoreService 接口，封装现有的数据库、日志、设置等服务，
 * 提供给插件使用。
 */

#ifndef CORE_SERVICE_IMPL_H
#define CORE_SERVICE_IMPL_H

#include "CoreService.h"

// 前向声明
class DatabaseManager;
class Logger;
class QSettings;
class EventBus;

/**
 * @brief 核心服务实现类
 *
 * 将主程序现有的服务封装为 CoreService 接口，
 * 插件通过此接口访问核心服务。
 */
class CoreServiceImpl : public CoreService
{
public:
    /**
     * @brief 构造函数
     */
    CoreServiceImpl();

    /**
     * @brief 析构函数
     */
    ~CoreServiceImpl() override;

    // ========================================================================
    // CoreService 接口实现
    // ========================================================================

    DatabaseManager* database() const override;
    Logger* logger() const override;
    QSettings* settings() const override;
    QSettings* pluginSettings(const QString& pluginId) const override;
    EventBus* eventBus() const override;
    QString dataDirectory() const override;
    QString pluginDirectory() const override;
    QString appVersion() const override;

    // ========================================================================
    // 初始化
    // ========================================================================

    /**
     * @brief 初始化核心服务
     * @param dataDir 数据目录
     * @return 成功返回 true
     */
    bool initialize(const QString& dataDir);

    /**
     * @brief 关闭核心服务
     */
    void shutdown();

private:
    DatabaseManager* m_database;   ///< 数据库管理器
    QSettings* m_settings;         ///< 应用设置
    EventBus* m_eventBus;          ///< 事件总线
    QString m_dataDir;             ///< 数据目录
    QString m_pluginDir;           ///< 插件目录
};

#endif // CORE_SERVICE_IMPL_H
