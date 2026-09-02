/**
 * @file DatabaseManager.h
 * @brief 数据库管理器头文件
 *
 * DatabaseManager 是数据库访问的核心类，采用单例模式。
 * 负责：
 * - 数据库连接管理
 * - 表结构创建与版本迁移
 * - 事务管理
 * - 提供全局数据库连接
 *
 * 所有仓储类（Repository）都通过 DatabaseManager 获取数据库连接。
 */

#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <QString>
#include <QSqlDatabase>
#include <QMutex>

/**
 * @brief 数据库管理器类（单例模式）
 *
 * 使用方式：
 * @code
 *   DatabaseManager::instance().initialize("/path/to/db.sqlite");
 *   QSqlDatabase db = DatabaseManager::instance().database();
 *   // 执行 SQL...
 *   DatabaseManager::instance().close();
 * @endcode
 */
class DatabaseManager
{
public:
    /**
     * @brief 获取单例实例
     * @return DatabaseManager 单例引用
     */
    static DatabaseManager& instance();

    // 禁止拷贝
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    /**
     * @brief 初始化数据库
     * @param dbPath 数据库文件路径
     * @return 成功返回 true
     *
     * 执行流程：
     * 1. 建立 SQLite 连接
     * 2. 启用 WAL 模式（提高并发读写性能）
     * 3. 启用外键约束
     * 4. 执行表结构创建（如果不存在）
     * 5. 执行版本迁移（如果需要）
     * 6. 初始化内置模板数据
     */
    bool initialize(const QString& dbPath);

    /**
     * @brief 获取数据库连接
     * @return QSqlDatabase 引用
     *
     * 注意：返回的是默认连接，所有线程共享。
     * 如果需要多线程访问，应使用 QSqlDatabase::cloneDatabase()。
     */
    QSqlDatabase database() const;

    /**
     * @brief 检查数据库是否已初始化
     * @return 已初始化返回 true
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief 获取当前数据库版本号
     * @return 版本号
     */
    int databaseVersion() const { return m_currentVersion; }

    /**
     * @brief 关闭数据库连接
     */
    void close();

    /**
     * @brief 执行 SQL 脚本（从文件读取）
     * @param filePath SQL 脚本文件路径
     * @return 成功返回 true
     */
    bool executeSqlFile(const QString& filePath);

    /**
     * @brief 执行单条 SQL 语句
     * @param sql SQL 语句
     * @param errorMsg 输出参数：错误信息
     * @return 成功返回 true
     */
    bool executeSql(const QString& sql, QString* errorMsg = nullptr);

    /**
     * @brief 开始事务
     * @return 成功返回 true
     */
    bool transaction();

    /**
     * @brief 提交事务
     * @return 成功返回 true
     */
    bool commit();

    /**
     * @brief 回滚事务
     * @return 成功返回 true
     */
    bool rollback();

    /**
     * @brief 获取最后一次错误信息
     * @return 错误信息字符串
     */
    QString lastError() const;

private:
    // 私有构造
    DatabaseManager();
    ~DatabaseManager();

    /**
     * @brief 创建所有数据库表
     * @return 成功返回 true
     *
     * 使用 CREATE TABLE IF NOT EXISTS，幂等操作。
     */
    bool createTables();

    /**
     * @brief 创建索引
     * @return 成功返回 true
     */
    bool createIndexes();

    /**
     * @brief 创建 FTS5 全文索引虚拟表
     * @return 成功返回 true
     */
    bool createFtsTables();

    /**
     * @brief 创建触发器（FTS 同步用）
     * @return 成功返回 true
     */
    bool createTriggers();

    /**
     * @brief 执行数据库迁移
     * @param fromVersion 源版本
     * @param toVersion 目标版本
     * @return 成功返回 true
     */
    bool migrate(int fromVersion, int toVersion);

    /**
     * @brief 获取数据库中存储的版本号
     * @return 版本号（如果不存在返回 0）
     */
    int getStoredVersion();

    /**
     * @brief 更新数据库版本号
     * @param version 新版本号
     */
    void setStoredVersion(int version);

    /**
     * @brief 初始化内置模板数据
     * @return 成功返回 true
     */
    bool seedBuiltinTemplates();

    // -----------------------------------------------------------------------
    // 成员变量
    // -----------------------------------------------------------------------

    QString m_dbPath;          ///< 数据库文件路径
    QString m_connectionName;  ///< 连接名称
    bool    m_initialized;     ///< 是否已初始化
    int     m_currentVersion;  ///< 当前数据库版本
    QMutex  m_mutex;           ///< 互斥锁（事务等操作线程安全）
};

#endif // DATABASE_MANAGER_H
