/**
 * @file Logger.h
 * @brief 日志系统头文件
 *
 * 提供线程安全的日志记录功能，支持：
 * - 多级日志（Debug/Info/Warning/Error/Critical）
 * - 同时输出到控制台和文件
 * - 日志文件按大小自动滚动
 * - 单例模式，全局访问
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>

/**
 * @brief 日志级别枚举
 */
enum class LogLevel {
    Debug,      ///< 调试信息，最详细
    Info,       ///< 一般信息
    Warning,    ///< 警告信息
    Error,      ///< 错误信息
    Critical    ///< 严重错误，可能导致程序崩溃
};

/**
 * @brief 日志记录器类（单例模式）
 *
 * 使用方式：
 * @code
 *   Logger::instance().initialize("/path/to/log/dir");
 *   Logger::instance().info("这是一条信息日志");
 *   Logger::instance().error("发生错误: " + errorString);
 * @endcode
 *
 * 也可以使用便捷宏：
 * @code
 *   LOG_INFO("这是一条信息日志");
 *   LOG_ERROR("发生错误: %1", errorString);
 * @endcode
 */
class Logger
{
public:
    /**
     * @brief 获取单例实例
     * @return Logger 单例引用
     *
     * 线程安全的单例实现（C++11 静态局部变量初始化保证线程安全）
     */
    static Logger& instance();

    // 禁止拷贝和赋值（单例模式）
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief 初始化日志系统
     * @param logDir 日志文件存储目录
     * @param maxFileSize 单个日志文件最大大小（字节），默认 10MB
     * @param maxFileCount 保留的日志文件数量，默认 5 个
     *
     * 必须在使用日志功能前调用一次。
     * 如果目录不存在会自动创建。
     */
    void initialize(const QString& logDir,
                    qint64 maxFileSize = 10 * 1024 * 1024,
                    int maxFileCount = 5);

    /**
     * @brief 设置最低日志级别
     * @param level 低于此级别的日志将被忽略
     *
     * 例如设置为 LogLevel::Info，则 Debug 级别日志不会输出。
     */
    void setLogLevel(LogLevel level);

    // -----------------------------------------------------------------------
    // 各级别日志输出方法
    // -----------------------------------------------------------------------

    /// 输出 Debug 级别日志
    void debug(const QString& message);

    /// 输出 Info 级别日志
    void info(const QString& message);

    /// 输出 Warning 级别日志
    void warning(const QString& message);

    /// 输出 Error 级别日志
    void error(const QString& message);

    /// 输出 Critical 级别日志
    void critical(const QString& message);

    /**
     * @brief 通用日志输出方法
     * @param level 日志级别
     * @param message 日志消息
     *
     * 其他便捷方法（debug/info 等）内部都调用此方法。
     */
    void log(LogLevel level, const QString& message);

    /**
     * @brief 内部日志输出（调用方必须已持有 m_mutex 锁）
     * @param level 日志级别
     * @param message 日志消息
     *
     * 与 log() 的区别：此方法不额外加锁，供内部已加锁的方法调用。
     */
    void logInternal(LogLevel level, const QString& message);

    /**
     * @brief 关闭日志系统，刷新并关闭文件
     *
     * 程序退出时自动调用，也可手动调用。
     */
    void close();

private:
    // 私有构造函数（单例模式）
    Logger();
    ~Logger();

    /**
     * @brief 将日志级别转换为字符串
     * @param level 日志级别
     * @return 级别字符串（如 "INFO", "ERROR"）
     */
    static QString levelToString(LogLevel level);

    /**
     * @brief 检查并执行日志文件滚动
     *
     * 当当前日志文件大小超过 maxFileSize 时，
     * 将现有文件重命名为 .1, .2 ... 并创建新文件。
     */
    void rotateLogFileIfNeeded();

    /**
     * @brief 获取当前日志文件路径
     * @return 日志文件完整路径
     */
    QString currentLogFilePath() const;

    // -----------------------------------------------------------------------
    // 成员变量
    // -----------------------------------------------------------------------

    QMutex m_mutex;           ///< 互斥锁，保证多线程安全
    QFile m_logFile;          ///< 日志文件对象
    QTextStream m_logStream;  ///< 文本流，用于写入文件
    QString m_logDir;         ///< 日志目录
    qint64 m_maxFileSize;     ///< 单个日志文件最大大小
    int m_maxFileCount;       ///< 保留的日志文件数量
    LogLevel m_minLevel;      ///< 最低输出级别
    bool m_initialized;       ///< 是否已初始化
    bool m_logToConsole;      ///< 是否同时输出到控制台
};

// ---------------------------------------------------------------------------
// 便捷日志宏
//
// 使用宏可以自动记录文件名、函数名和行号，便于调试定位。
// 例如：LOG_INFO("初始化完成") 会输出 [INFO] main.cpp:42 - 初始化完成
// ---------------------------------------------------------------------------

/// 输出 Debug 日志（带文件位置信息）
#define LOG_DEBUG(msg) \
    Logger::instance().debug(QString("%1:%2 - %3").arg(__FILE__).arg(__LINE__).arg(msg))

/// 输出 Info 日志（带文件位置信息）
#define LOG_INFO(msg) \
    Logger::instance().info(QString("%1:%2 - %3").arg(__FILE__).arg(__LINE__).arg(msg))

/// 输出 Warning 日志（带文件位置信息）
#define LOG_WARNING(msg) \
    Logger::instance().warning(QString("%1:%2 - %3").arg(__FILE__).arg(__LINE__).arg(msg))

/// 输出 Error 日志（带文件位置信息）
#define LOG_ERROR(msg) \
    Logger::instance().error(QString("%1:%2 - %3").arg(__FILE__).arg(__LINE__).arg(msg))

#endif // LOGGER_H
