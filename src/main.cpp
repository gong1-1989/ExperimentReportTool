/**
 * @file main.cpp
 * @brief 程序入口文件
 *
 * 负责：
 * 1. 创建 QApplication 实例
 * 2. 初始化全局资源（数据库、日志、设置）
 * 3. 显示主窗口
 * 4. 进入事件循环
 */

#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

#include "ui/MainWindow.h"
#include "ui/dialogs/LoginDialog.h"
#include "core/utils/Logger.h"
#include "core/utils/AppConstants.h"
#include "data/database/DatabaseManager.h"
#include "core/plugin/CoreServiceImpl.h"
#include "core/plugin/PluginManager.h"
#include "editor/OtherBlockEditors.h"  // BlockEditorFactory

/**
 * @brief 初始化应用程序的全局设置
 *
 * 设置组织名、应用名等，用于 QSettings 等功能的配置存储路径。
 * 同时确保数据目录存在。
 */
static void initializeApplication()
{
    // 设置应用程序元信息（QSettings 会用这些信息确定配置文件位置）
    QCoreApplication::setOrganizationName(AppConstants::ORG_NAME);
    QCoreApplication::setApplicationName(AppConstants::APP_NAME);
    QCoreApplication::setApplicationVersion(AppConstants::APP_VERSION);

    // 确保数据存储目录存在（程序目录下的 data 子目录）
    const QString dataDir = QCoreApplication::applicationDirPath() + "/data";
    QDir().mkpath(dataDir);

    // 初始化日志系统（程序目录下的 logs 子目录）
    const QString logDir = QCoreApplication::applicationDirPath() + "/logs";
    Logger::instance().initialize(logDir);
    Logger::instance().info(QString("应用程序启动，版本 %1").arg(AppConstants::APP_VERSION));
    Logger::instance().info(QString("数据目录: %1").arg(dataDir));
    Logger::instance().info(QString("日志目录: %1").arg(logDir));
}

/**
 * @brief 初始化数据库
 * @return 成功返回 true，失败返回 false
 */
static bool initializeDatabase()
{
    // 获取数据库文件路径（程序目录下的 data 子目录）
    const QString dbPath = QCoreApplication::applicationDirPath()
        + "/data/experiment_reports.db";

    // 初始化数据库管理器（单例）
    DatabaseManager& dbMgr = DatabaseManager::instance();
    if (!dbMgr.initialize(dbPath)) {
        Logger::instance().error("数据库初始化失败");
        return false;
    }

    Logger::instance().info("数据库初始化成功");
    return true;
}

/**
 * @brief 主函数
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 程序退出码
 */
int main(int argc, char *argv[])
{
    // 启用高 DPI 缩放（Qt6 默认启用，Qt5 需要显式设置）
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    // 创建 Qt 应用程序实例
    // 注意：QApplication 必须在创建任何 UI 元素之前创建
    QApplication app(argc, argv);

    // 设置应用程序样式（Fusion 风格在各平台外观一致）
    // 可选值："Fusion", "Windows", "WindowsVista", "Macintosh" 等
    app.setStyle(QStyleFactory::create("Fusion"));

    // 加载全局样式表（从资源文件读取）
    QFile qssFile(":/ui/resources/styles/app.qss");
    if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(qssFile.readAll());
        qssFile.close();
        Logger::instance().info("全局样式表已加载");
    } else {
        Logger::instance().warning("全局样式表加载失败");
    }

    // 初始化全局设置
    initializeApplication();

    // 初始化数据库，失败则退出
    if (!initializeDatabase()) {
        Logger::instance().error("数据库初始化失败，程序退出");
        return 1;
    }

    // -----------------------------------------------------------------------
    // 初始化插件框架
    // -----------------------------------------------------------------------
    const QString dataDir = QCoreApplication::applicationDirPath() + "/data";

    // 初始化核心服务
    CoreServiceImpl coreService;
    coreService.initialize(dataDir);

    // 创建插件管理器
    PluginManager pluginManager;
    pluginManager.setCoreService(&coreService);

    // 添加插件搜索目录（可执行文件同级的 plugins 目录 + 数据目录下的 plugins）
    pluginManager.addPluginDirectory(QCoreApplication::applicationDirPath() + "/plugins");
    pluginManager.addPluginDirectory(coreService.pluginDirectory());

    // 动态加载所有插件（从 plugins/ 目录扫描 .dll/.so/.dylib）
    const int pluginCount = pluginManager.loadAllPlugins();

    Logger::instance().info(QString("插件框架初始化完成，共加载 %1 个插件")
        .arg(pluginCount));

    // 设置块编辑器工厂的插件管理器，使编辑器能通过插件创建块编辑器
    BlockEditorFactory::setPluginManager(&pluginManager);

    // -----------------------------------------------------------------------
    // 用户登录验证
    // -----------------------------------------------------------------------
    LoginDialog loginDialog;
    if (loginDialog.exec() != QDialog::Accepted) {
        // 用户取消登录或关闭登录窗口，直接退出程序
        Logger::instance().info("用户取消登录，程序退出");
        pluginManager.unloadAllPlugins();
        DatabaseManager::instance().close();
        return 0;
    }

    Logger::instance().info(QString("用户 '%1' 登录成功").arg(loginDialog.currentUsername()));

    // 创建并显示主窗口
    MainWindow mainWindow;
    mainWindow.setPluginManager(&pluginManager);
    mainWindow.show();

    // 进入 Qt 事件循环
    // exec() 会阻塞直到窗口关闭，返回退出码
    const int exitCode = app.exec();

    // 清理资源
    pluginManager.unloadAllPlugins();
    coreService.shutdown();
    DatabaseManager::instance().close();
    Logger::instance().info("应用程序正常退出");

    return exitCode;
}
