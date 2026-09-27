/**
 * @file CoreServiceImpl.cpp
 * @brief 核心服务实现类源文件
 */

#include "CoreServiceImpl.h"
#include "data/database/DatabaseManager.h"
#include "core/utils/Logger.h"
#include "core/utils/AppConstants.h"
#include "core/eventbus/EventBus.h"

#include <QSettings>
#include <QDir>
#include <QCoreApplication>

// ============================================================================
// 构造与析构
// ============================================================================

CoreServiceImpl::CoreServiceImpl()
    : m_database(nullptr)
    , m_settings(nullptr)
    , m_eventBus(nullptr)
{
}

CoreServiceImpl::~CoreServiceImpl()
{
    shutdown();
}

// ============================================================================
// 初始化与关闭
// ============================================================================

bool CoreServiceImpl::initialize(const QString& dataDir)
{
    m_dataDir = dataDir;

    // 插件目录：数据目录下的 plugins 子目录
    m_pluginDir = QDir(dataDir).filePath("plugins");
    QDir().mkpath(m_pluginDir);

    // 创建设置
    m_settings = new QSettings(AppConstants::ORG_NAME, AppConstants::APP_NAME);

    // 创建事件总线（单例模式，获取实例指针）
    m_eventBus = &EventBus::instance();

    // 数据库由主程序初始化，这里获取单例
    m_database = &DatabaseManager::instance();

    return true;
}

void CoreServiceImpl::shutdown()
{
    if (m_settings) {
        m_settings->sync();
        delete m_settings;
        m_settings = nullptr;
    }

    if (m_eventBus) {
        // 事件总线是单例，由 Qt 管理生命周期，这里不 delete
        m_eventBus = nullptr;
    }

    // 数据库由主程序管理，这里不删除
    m_database = nullptr;
}

// ============================================================================
// CoreService 接口实现
// ============================================================================

DatabaseManager* CoreServiceImpl::database() const
{
    return m_database;
}

Logger* CoreServiceImpl::logger() const
{
    return &Logger::instance();
}

QSettings* CoreServiceImpl::settings() const
{
    return m_settings;
}

QSettings* CoreServiceImpl::pluginSettings(const QString& pluginId) const
{
    if (!m_settings) return nullptr;

    // 定位到插件专属设置组
    m_settings->beginGroup(QString("plugins/%1").arg(pluginId));
    return m_settings;
    // 注意：调用方需要调用 endGroup()
}

EventBus* CoreServiceImpl::eventBus() const
{
    return m_eventBus;
}

QString CoreServiceImpl::dataDirectory() const
{
    return m_dataDir;
}

QString CoreServiceImpl::pluginDirectory() const
{
    return m_pluginDir;
}

QString CoreServiceImpl::appVersion() const
{
    return AppConstants::APP_VERSION;
}
