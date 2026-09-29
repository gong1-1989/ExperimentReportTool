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
#include <QTranslator>
#include <QLibraryInfo>
#include <QCoreApplication>

#include "ui/MainWindow.h"
#include "ui/dialogs/LoginDialog.h"
#include "core/utils/Logger.h"
#include "core/utils/AppConstants.h"
#include "core/utils/AppConfig.h"
#include "core/utils/UserSession.h"
#include "data/database/DatabaseManager.h"
#include "core/plugin/PluginManager.h"

/**
 * @brief 解析日志级别字符串（--log-level= 或环境变量 ERT_LOG_LEVEL）
 */
static LogLevel parseLogLevel(const QString& s)
{
    const QString v = s.trimmed().toLower();
    if (v == "debug")    return LogLevel::Debug;
    if (v == "warning")  return LogLevel::Warning;
    if (v == "error")    return LogLevel::Error;
    return LogLevel::Info;
}

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
    // 日志级别策略：命令行 --log-level= > 环境变量 ERT_LOG_LEVEL > 构建默认
    // （Debug 构建默认 debug；Release 构建默认 info，Debug 日志编译为空）
    LogLevel logLevel = LogLevel::Info;
#ifndef QT_NO_DEBUG_OUTPUT
    logLevel = LogLevel::Debug;
#endif
    const QByteArray envLevel = qgetenv("ERT_LOG_LEVEL");
    if (!envLevel.isEmpty()) logLevel = parseLogLevel(QString::fromLatin1(envLevel));
    const QStringList args = QCoreApplication::arguments();
    for (const QString& a : args) {
        if (a.startsWith("--log-level=", Qt::CaseInsensitive)) {
            logLevel = parseLogLevel(a.mid(12));
            break;
        }
    }
    Logger::instance().setLogLevel(logLevel);
    LOG_DEBUG(QString("日志级别: %1").arg(
        logLevel == LogLevel::Debug ? "debug" :
        logLevel == LogLevel::Warning ? "warning" :
        logLevel == LogLevel::Error ? "error" : "info"));
    Logger::instance().info(QString("应用程序启动，版本 %1").arg(AppConstants::APP_VERSION));
    LOG_DEBUG(QString("数据目录: %1").arg(dataDir));
    LOG_DEBUG(QString("日志目录: %1").arg(logDir));
}

/**
 * @brief 初始化数据库
 * @return 成功返回 true，失败返回 false
 */
static bool initializeDatabase()
{
    // 从配置文件读取数据库路径，支持共享文件夹
    // 配置为空时使用默认路径（程序目录下的 data 子目录）
    QString dbPath = AppConfig::instance().databasePath();
    if (dbPath.isEmpty()) {
        dbPath = QCoreApplication::applicationDirPath()
            + "/data/experiment_reports.db";
    }

    // 初始化数据库管理器（单例）
    DatabaseManager& dbMgr = DatabaseManager::instance();
    if (!dbMgr.initialize(dbPath)) {
        Logger::instance().error("数据库初始化失败");
        return false;
    }

    Logger::instance().debug(QString("数据库初始化成功: %1").arg(dbPath));
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

    // 加载 Qt 自带的中文翻译（使 QMessageBox/QInputDialog/QFileDialog 的标准按钮
    // OK/Cancel/Save/Discard 等显示为中文"确定/取消/保存/放弃"）
    // 依次尝试：Qt 安装目录翻译目录 → 程序目录 translations → 程序目录
    // （发布部署时需将 qtbase_zh_CN.qm 拷贝到程序目录或 translations 子目录）
    {
        // 静态存储期：程序退出时销毁，晚于 app 析构，安全且无泄漏（clang-analyzer 不再误报）
        static QTranslator qtTranslator;
        const QStringList candidates = {
            QLibraryInfo::path(QLibraryInfo::TranslationsPath) + "/qtbase_zh_CN",
            QLibraryInfo::path(QLibraryInfo::TranslationsPath) + "/qt_zh_CN",
            QCoreApplication::applicationDirPath() + "/translations/qtbase_zh_CN",
            QCoreApplication::applicationDirPath() + "/qtbase_zh_CN",
        };
        bool loaded = false;
        for (const QString& path : candidates) {
            if (qtTranslator.load(path)) {
                app.installTranslator(&qtTranslator);
                LOG_DEBUG(QString("Qt 中文翻译已加载: %1").arg(path));
                loaded = true;
                break;
            }
        }
        if (!loaded) {
            Logger::instance().warning(
                "未找到 Qt 中文翻译文件（qtbase_zh_CN.qm），标准对话框按钮将显示英文");
        }
    }

    // 设置应用程序样式（Fusion 风格在各平台外观一致）
    // 可选值："Fusion", "Windows", "WindowsVista", "Macintosh" 等
    app.setStyle(QStyleFactory::create("Fusion"));

    // 加载全局样式表（从资源文件读取）
    QFile qssFile(":/ui/resources/styles/app.qss");
    if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(qssFile.readAll());
        qssFile.close();
        LOG_DEBUG("全局样式表已加载");
    } else {
        Logger::instance().warning("全局样式表加载失败");
    }

    // 从配置读取全局字体和字号（覆盖 qss 中的默认值）
    {
        QFont appFont;
        appFont.setFamily(AppConfig::instance().fontFamily());
        appFont.setPixelSize(AppConfig::instance().baseFontSize());
        app.setFont(appFont);
        LOG_DEBUG(QString("全局字体已设置: %1, %2px")
            .arg(appFont.family()).arg(appFont.pixelSize()));
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

    // 创建插件管理器（独立插件管理器）
    PluginManager pluginManager;

    // 添加插件搜索目录（可执行文件同级的 plugins 目录）
    pluginManager.addPluginDirectory(QCoreApplication::applicationDirPath() + "/plugins");

    // 动态加载所有插件（从 plugins/ 目录扫描 .dll/.so/.dylib）
    const int pluginCount = pluginManager.loadAllPlugins();

    Logger::instance().info(QString("插件框架初始化完成，共加载 %1 个插件")
        .arg(pluginCount));

    // -----------------------------------------------------------------------
    // 用户登录验证（循环：登出后重新回到登录界面）
    // -----------------------------------------------------------------------
    int exitCode = 0;
    while (true) {
        LoginDialog loginDialog;
        if (loginDialog.exec() != QDialog::Accepted) {
            // 用户取消登录或关闭登录窗口，直接退出程序
            Logger::instance().info("用户取消登录，程序退出");
            exitCode = 0;
            break;
        }

        Logger::instance().info(QString("用户 '%1' 登录成功").arg(loginDialog.currentUsername()));

        // 创建并显示主窗口
        MainWindow mainWindow;
        mainWindow.setPluginManager(&pluginManager);
        mainWindow.show();

        // 进入 Qt 事件循环（exec 阻塞直到窗口关闭）
        exitCode = app.exec();

        // 主窗口关闭后：若会话已清除（登出），回到登录界面重新登录；
        // 否则（正常退出）结束程序
        if (UserSession::instance().isLoggedIn()) {
            break;
        }
        Logger::instance().info("用户已登出，返回登录界面");
    }

    // 清理资源
    pluginManager.unloadAllPlugins();
    DatabaseManager::instance().close();
    Logger::instance().info("应用程序正常退出");

    return exitCode;
}
