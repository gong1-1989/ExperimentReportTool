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
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QStyleFactory>

#include "core/utils/AppConstants.h"
#include "core/utils/Logger.h"
#include "data/database/DatabaseManager.h"
#include "ui/MainWindow.h"

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

    // 确保数据存储目录存在
    // QStandardPaths::AppDataLocation 在各平台的路径：
    //   Windows: C:/Users/<user>/AppData/Local/<org>/<app>
    //   macOS:   ~/Library/Application Support/<org>/<app>
    //   Linux:   ~/.local/share/<org>/<app>
    //const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    //QCoreApplication::applicationDirPath() 在EXE文件同目录下；
    const QString dataDir=QCoreApplication::applicationDirPath();
    //QDir().mkpath(dataDir);

    // 初始化日志系统
    Logger::instance().initialize(dataDir + "/logs");
    Logger::instance().info(QString("应用程序启动，版本 %1").arg(AppConstants::APP_VERSION));
    Logger::instance().info(QString("数据目录: %1").arg(dataDir));
}

/**
 * @brief 初始化数据库
 * @return 成功返回 true，失败返回 false
 */
static bool initializeDatabase()
{
    // 获取数据库文件路径
    const QString dbPath = QCoreApplication::applicationDirPath()
                           + "/experiment_reports.db";

    // 初始化数据库管理器（单例）
    DatabaseManager &dbMgr = DatabaseManager::instance();
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

    // 初始化全局设置
    initializeApplication();

    // 初始化数据库，失败则退出
    if (!initializeDatabase()) {
        Logger::instance().error("数据库初始化失败，程序退出");
        return 1;
    }

    // 创建并显示主窗口
    MainWindow mainWindow;
    mainWindow.show();

    // 进入 Qt 事件循环
    // exec() 会阻塞直到窗口关闭，返回退出码
    const int exitCode = app.exec();

    // 清理资源
    DatabaseManager::instance().close();
    Logger::instance().info("应用程序正常退出");

    return exitCode;
}
