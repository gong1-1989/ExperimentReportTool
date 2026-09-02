/**
 * @file Logger.cpp
 * @brief 日志系统实现文件
 */

#include "Logger.h"

#include <QDir>
#include <QCoreApplication>
#include <QDebug>

// ===========================================================================
// 单例实现
// ===========================================================================

Logger& Logger::instance()
{
    // C++11 保证静态局部变量的初始化是线程安全的
    static Logger s_instance;
    return s_instance;
}

Logger::Logger()
    : m_maxFileSize(10 * 1024 * 1024)  // 默认 10MB
    , m_maxFileCount(5)                   // 默认保留 5 个文件
    , m_minLevel(LogLevel::Debug)         // 默认输出所有级别
    , m_initialized(false)
    , m_logToConsole(true)                 // 默认同时输出到控制台
{
    // 构造函数中不做文件操作，等待 initialize 调用
}

Logger::~Logger()
{
    close();
}

// ===========================================================================
// 初始化与配置
// ===========================================================================

void Logger::initialize(const QString& logDir,
                        qint64 maxFileSize,
                        int maxFileCount)
{
    QMutexLocker locker(&m_mutex);

    if (m_initialized) {
        // 已经初始化过，先关闭旧文件
        close();
    }

    m_logDir = logDir;
    m_maxFileSize = maxFileSize;
    m_maxFileCount = qMax(1, maxFileCount);  // 至少保留 1 个

    // 确保日志目录存在
    QDir().mkpath(m_logDir);

    // 打开日志文件（追加模式）
    const QString logPath = currentLogFilePath();
    m_logFile.setFileName(logPath);

    // QIODevice::Append - 追加写入
    // QIODevice::Text - 文本模式（换行符转换）
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        // 文件打开失败，仅输出到控制台
        qWarning() << "Logger: 无法打开日志文件:" << logPath << m_logFile.errorString();
        m_logToConsole = true;
    } else {
        // 将文本流绑定到文件
        m_logStream.setDevice(&m_logFile);
        // 设置 UTF-8 编码（Qt6 默认，Qt5 需显式设置）
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        m_logStream.setCodec("UTF-8");
#endif
    }

    m_initialized = true;

    // 输出启动标记
    const QString separator(60, '=');
    logInternal(LogLevel::Info, QString("\n%1\n  日志系统启动 - %2\n%1")
                .arg(separator)
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
}

void Logger::setLogLevel(LogLevel level)
{
    QMutexLocker locker(&m_mutex);
    m_minLevel = level;
}

// ===========================================================================
// 日志输出方法
// ===========================================================================

void Logger::debug(const QString& message)
{
    log(LogLevel::Debug, message);
}

void Logger::info(const QString& message)
{
    log(LogLevel::Info, message);
}

void Logger::warning(const QString& message)
{
    log(LogLevel::Warning, message);
}

void Logger::error(const QString& message)
{
    log(LogLevel::Error, message);
}

void Logger::critical(const QString& message)
{
    log(LogLevel::Critical, message);
}

void Logger::log(LogLevel level, const QString& message)
{
    QMutexLocker locker(&m_mutex);
    logInternal(level, message);
}

// ===========================================================================
// 内部实现（调用方已持有锁）
// ===========================================================================

void Logger::logInternal(LogLevel level, const QString& message)
{
    // 级别过滤：低于最低级别的日志不输出
    if (level < m_minLevel) {
        return;
    }

    // 格式化日志行：[时间] [级别] 消息
    const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    const QString levelStr = levelToString(level);
    const QString logLine = QString("[%1] [%2] %3")
                                 .arg(timestamp)
                                 .arg(levelStr, -8)  // 左对齐，占 8 个字符宽度
                                 .arg(message);

    // 输出到控制台（使用 qDebug 等，方便在 IDE 中查看）
    if (m_logToConsole) {
        switch (level) {
        case LogLevel::Debug:
            qDebug().noquote() << logLine;
            break;
        case LogLevel::Info:
            qInfo().noquote() << logLine;
            break;
        case LogLevel::Warning:
            qWarning().noquote() << logLine;
            break;
        case LogLevel::Error:
        case LogLevel::Critical:
            qCritical().noquote() << logLine;
            break;
        }
    }

    // 输出到文件
    if (m_logFile.isOpen()) {
        // 写入前检查是否需要滚动文件
        rotateLogFileIfNeeded();

        m_logStream << logLine << "\n";
        m_logStream.flush();  // 立即刷新，防止崩溃时丢失日志
    }
}

// ===========================================================================
// 日志文件滚动
// ===========================================================================

void Logger::rotateLogFileIfNeeded()
{
    if (!m_logFile.isOpen()) {
        return;
    }

    // 检查当前文件大小
    if (m_logFile.size() < m_maxFileSize) {
        return;  // 未超过限制，不需要滚动
    }

    // 关闭当前文件
    m_logStream.flush();
    m_logFile.close();

    const QString basePath = currentLogFilePath();

    // 滚动旧文件：app.log.4 -> 删除, app.log.3 -> app.log.4, ..., app.log -> app.log.1
    for (int i = m_maxFileCount - 1; i >= 1; --i) {
        const QString oldFile = QString("%1.%2").arg(basePath).arg(i);
        const QString newFile = QString("%1.%2").arg(basePath).arg(i + 1);

        if (i == m_maxFileCount - 1) {
            // 最老的文件直接删除
            QFile::remove(newFile);
        }

        if (QFile::exists(oldFile)) {
            QFile::remove(newFile);      // 先删除目标（如果存在）
            QFile::rename(oldFile, newFile);  // 重命名
        }
    }

    // 当前日志文件重命名为 .1
    const QString firstBackup = basePath + ".1";
    QFile::remove(firstBackup);
    QFile::rename(basePath, firstBackup);

    // 重新打开新的日志文件
    m_logFile.setFileName(basePath);
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_logStream.setDevice(&m_logFile);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        m_logStream.setCodec("UTF-8");
#endif
    }
}

// ===========================================================================
// 工具方法
// ===========================================================================

QString Logger::levelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:    return "DEBUG";
    case LogLevel::Info:     return "INFO";
    case LogLevel::Warning:  return "WARNING";
    case LogLevel::Error:    return "ERROR";
    case LogLevel::Critical: return "CRITICAL";
    }
    return "UNKNOWN";
}

QString Logger::currentLogFilePath() const
{
    // 日志文件名：应用名.log，如 ExperimentReportTool.log
    const QString appName = QCoreApplication::applicationName();
    return m_logDir + "/" + appName + ".log";
}

void Logger::close()
{
    QMutexLocker locker(&m_mutex);

    if (m_logFile.isOpen()) {
        m_logStream.flush();
        m_logFile.close();
    }
    m_initialized = false;
}
