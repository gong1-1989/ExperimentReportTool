/**
 * @file DatabaseManager.cpp
 * @brief 数据库管理器实现文件
 */

#include "DatabaseManager.h"
#include "core/utils/Logger.h"
#include "core/utils/AppConstants.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

// 数据库连接名称（使用唯一名称避免冲突）
static const char* CONNECTION_NAME = "experiment_report_main";

// ===========================================================================
// 单例实现
// ===========================================================================

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager s_instance;
    return s_instance;
}

DatabaseManager::DatabaseManager()
    : m_connectionName(QLatin1String(CONNECTION_NAME))
    , m_initialized(false)
    , m_currentVersion(0)
{
}

DatabaseManager::~DatabaseManager()
{
    close();
}

// ===========================================================================
// 初始化
// ===========================================================================

bool DatabaseManager::initialize(const QString& dbPath)
{
    QMutexLocker locker(&m_mutex);

    if (m_initialized) {
        LOG_WARNING("数据库已经初始化，跳过");
        return true;
    }

    m_dbPath = dbPath;

    // 确保数据库文件的目录存在
    QDir().mkpath(QFileInfo(dbPath).absolutePath());

    // 添加 SQLite 数据库连接
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(dbPath);

    // 打开数据库
    if (!db.open()) {
        LOG_ERROR(QString("无法打开数据库: %1").arg(db.lastError().text()));
        return false;
    }

    LOG_INFO(QString("数据库已打开: %1").arg(dbPath));

    // -----------------------------------------------------------------------
    // SQLite 性能与功能设置
    // -----------------------------------------------------------------------

    // WAL 模式：允许并发读写，大幅提升写入性能
    // 注意：WAL 模式在网络文件系统上可能有问题，但本地文件没问题
    db.exec("PRAGMA journal_mode = WAL;");

    // 启用外键约束（SQLite 默认关闭）
    db.exec("PRAGMA foreign_keys = ON;");

    // 同步模式：NORMAL 在 WAL 模式下足够安全，且性能更好
    // FULL 更安全但慢，NORMAL 是推荐值
    db.exec("PRAGMA synchronous = NORMAL;");

    // 缓存大小：设置为 64MB（单位是页，默认页大小 4096 字节）
    // 64 * 1024 * 1024 / 4096 = 16384 页
    db.exec("PRAGMA cache_size = -16384;");

    // -----------------------------------------------------------------------
    // 创建表结构
    // -----------------------------------------------------------------------

    if (!createTables()) {
        LOG_ERROR("创建表结构失败");
        return false;
    }

    if (!createIndexes()) {
        LOG_ERROR("创建索引失败");
        return false;
    }

    if (!createFtsTables()) {
        LOG_ERROR("创建全文索引表失败");
        return false;
    }

    if (!createTriggers()) {
        LOG_ERROR("创建触发器失败");
        return false;
    }

    // -----------------------------------------------------------------------
    // 版本管理与迁移
    // -----------------------------------------------------------------------

    const int storedVersion = getStoredVersion();
    const int targetVersion = AppConstants::DATABASE_VERSION;

    if (storedVersion < targetVersion) {
        LOG_INFO(QString("需要数据库迁移: %1 -> %2").arg(storedVersion).arg(targetVersion));
        if (!migrate(storedVersion, targetVersion)) {
            LOG_ERROR("数据库迁移失败");
            return false;
        }
        setStoredVersion(targetVersion);
    }

    m_currentVersion = targetVersion;

    // -----------------------------------------------------------------------
    // 初始化内置数据
    // -----------------------------------------------------------------------

    if (!seedBuiltinTemplates()) {
        LOG_WARNING("初始化内置模板失败（不影响核心功能）");
    }

    m_initialized = true;
    LOG_INFO(QString("数据库初始化完成，版本: %1").arg(m_currentVersion));
    return true;
}

// ===========================================================================
// 表结构创建
// ===========================================================================

bool DatabaseManager::createTables()
{
    QSqlDatabase db = database();

    // -----------------------------------------------------------------------
    // 项目表（支持树状结构）
    // -----------------------------------------------------------------------
    const QString createProjects = R"(
        CREATE TABLE IF NOT EXISTS projects (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            name        TEXT NOT NULL,
            type        TEXT DEFAULT '',
            description TEXT DEFAULT '',
            status      TEXT DEFAULT 'active',
            owner       TEXT DEFAULT '',
            parent_id   INTEGER DEFAULT -1,
            created_at  DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at  DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (parent_id) REFERENCES projects(id) ON DELETE SET NULL
        );
    )";

    // -----------------------------------------------------------------------
    // 报告表
    // -----------------------------------------------------------------------
    const QString createReports = R"(
        CREATE TABLE IF NOT EXISTS reports (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            project_id      INTEGER NOT NULL,
            template_id     INTEGER DEFAULT -1,
            title           TEXT NOT NULL,
            content         TEXT DEFAULT '[]',
            status          TEXT DEFAULT 'draft',
            author          TEXT DEFAULT '',
            experiment_date DATE,
            created_at      DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at      DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE
        );
    )";

    // -----------------------------------------------------------------------
    // 报告版本快照表
    // -----------------------------------------------------------------------
    const QString createReportVersions = R"(
        CREATE TABLE IF NOT EXISTS report_versions (
            id            INTEGER PRIMARY KEY AUTOINCREMENT,
            report_id     INTEGER NOT NULL,
            content       TEXT NOT NULL,
            snapshot_name TEXT DEFAULT '',
            created_at    DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (report_id) REFERENCES reports(id) ON DELETE CASCADE
        );
    )";

    // -----------------------------------------------------------------------
    // 模板表
    // -----------------------------------------------------------------------
    const QString createTemplates = R"(
        CREATE TABLE IF NOT EXISTS templates (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            name        TEXT NOT NULL,
            category    TEXT DEFAULT 'general',
            description TEXT DEFAULT '',
            structure   TEXT DEFAULT '[]',
            is_builtin  INTEGER DEFAULT 0,
            created_at  DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at  DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";

    // -----------------------------------------------------------------------
    // 实验数据表
    // -----------------------------------------------------------------------
    const QString createDataTables = R"(
        CREATE TABLE IF NOT EXISTS data_tables (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            report_id   INTEGER NOT NULL,
            name        TEXT NOT NULL,
            columns     TEXT DEFAULT '[]',
            rows        TEXT DEFAULT '[]',
            created_at  DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at  DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (report_id) REFERENCES reports(id) ON DELETE CASCADE
        );
    )";

    // -----------------------------------------------------------------------
    // 标签表
    // -----------------------------------------------------------------------
    const QString createTags = R"(
        CREATE TABLE IF NOT EXISTS tags (
            id    INTEGER PRIMARY KEY AUTOINCREMENT,
            name  TEXT NOT NULL UNIQUE,
            color TEXT DEFAULT '#4A90D9'
        );
    )";

    // -----------------------------------------------------------------------
    // 报告-标签关联表（多对多）
    // -----------------------------------------------------------------------
    const QString createReportTags = R"(
        CREATE TABLE IF NOT EXISTS report_tags (
            report_id INTEGER NOT NULL,
            tag_id    INTEGER NOT NULL,
            PRIMARY KEY (report_id, tag_id),
            FOREIGN KEY (report_id) REFERENCES reports(id) ON DELETE CASCADE,
            FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE
        );
    )";

    // -----------------------------------------------------------------------
    // 附件表
    // -----------------------------------------------------------------------
    const QString createAttachments = R"(
        CREATE TABLE IF NOT EXISTS attachments (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            report_id   INTEGER NOT NULL,
            file_name   TEXT NOT NULL,
            stored_path TEXT NOT NULL,
            file_size   INTEGER DEFAULT 0,
            mime_type   TEXT DEFAULT 'application/octet-stream',
            uploaded_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (report_id) REFERENCES reports(id) ON DELETE CASCADE
        );
    )";

    // -----------------------------------------------------------------------
    // 应用设置表（存储数据库版本等元信息）
    // -----------------------------------------------------------------------
    const QString createAppMeta = R"(
        CREATE TABLE IF NOT EXISTS app_meta (
            key   TEXT PRIMARY KEY,
            value TEXT
        );
    )";

    // 执行所有建表语句
    const QStringList statements = {
        createProjects,
        createReports,
        createReportVersions,
        createTemplates,
        createDataTables,
        createTags,
        createReportTags,
        createAttachments,
        createAppMeta
    };

    for (const QString& sql : statements) {
        QSqlQuery query(db);
        if (!query.exec(sql)) {
            LOG_ERROR(QString("建表失败: %1\nSQL: %2")
                         .arg(query.lastError().text())
                         .arg(sql.left(200)));
            return false;
        }
    }

    LOG_INFO("所有表创建完成");
    return true;
}

// ===========================================================================
// 索引创建
// ===========================================================================

bool DatabaseManager::createIndexes()
{
    QSqlDatabase db = database();

    // 常用查询索引，提升检索性能
    const QStringList indexes = {
        "CREATE INDEX IF NOT EXISTS idx_reports_project_id ON reports(project_id);",
        "CREATE INDEX IF NOT EXISTS idx_reports_status ON reports(status);",
        "CREATE INDEX IF NOT EXISTS idx_reports_experiment_date ON reports(experiment_date);",
        "CREATE INDEX IF NOT EXISTS idx_reports_updated_at ON reports(updated_at);",
        "CREATE INDEX IF NOT EXISTS idx_projects_parent_id ON projects(parent_id);",
        "CREATE INDEX IF NOT EXISTS idx_projects_status ON projects(status);",
        "CREATE INDEX IF NOT EXISTS idx_report_versions_report_id ON report_versions(report_id);",
        "CREATE INDEX IF NOT EXISTS idx_data_tables_report_id ON data_tables(report_id);",
        "CREATE INDEX IF NOT EXISTS idx_attachments_report_id ON attachments(report_id);",
        "CREATE INDEX IF NOT EXISTS idx_templates_category ON templates(category);"
    };

    for (const QString& sql : indexes) {
        QSqlQuery query(db);
        if (!query.exec(sql)) {
            LOG_ERROR(QString("创建索引失败: %1").arg(query.lastError().text()));
            return false;
        }
    }

    return true;
}

// ===========================================================================
// FTS5 全文索引
// ===========================================================================

bool DatabaseManager::createFtsTables()
{
    QSqlDatabase db = database();

    // 创建 FTS5 虚拟表，用于报告全文检索
    // content=reports 表示这是一个"外部内容"FTS 表，数据来源是 reports 表
    // content_rowid=id 表示使用 reports.id 作为行号
    const QString createFts = R"(
        CREATE VIRTUAL TABLE IF NOT EXISTS reports_fts USING fts5(
            title,
            content,
            tags,
            content='reports',
            content_rowid='id'
        );
    )";

    QSqlQuery query(db);
    if (!query.exec(createFts)) {
        // FTS5 可能不可用（某些 Qt 编译版本未启用），记录警告但不失败
        LOG_WARNING(QString("FTS5 全文索引不可用: %1").arg(query.lastError().text()));
        LOG_WARNING("将使用 LIKE 模糊查询作为降级方案");
        return true;
    }

    return true;
}

// ===========================================================================
// 触发器（FTS 同步）
// ===========================================================================

bool DatabaseManager::createTriggers()
{
    QSqlDatabase db = database();

    // 检查 FTS 表是否存在
    QSqlQuery check(db);
    check.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='reports_fts';");
    if (!check.next()) {
        // FTS 表不存在，跳过触发器创建
        return true;
    }

    // 当 reports 表插入/更新/删除时，自动同步 FTS 索引
    const QStringList triggers = {
        // 插入触发器
        R"(
        CREATE TRIGGER IF NOT EXISTS reports_ai AFTER INSERT ON reports BEGIN
            INSERT INTO reports_fts(rowid, title, content, tags)
            VALUES (new.id, new.title, new.content, '');
        END;
        )",
        // 删除触发器
        R"(
        CREATE TRIGGER IF NOT EXISTS reports_ad AFTER DELETE ON reports BEGIN
            INSERT INTO reports_fts(reports_fts, rowid, title, content, tags)
            VALUES ('delete', old.id, old.title, old.content, '');
        END;
        )",
        // 更新触发器（先删后插）
        R"(
        CREATE TRIGGER IF NOT EXISTS reports_au AFTER UPDATE ON reports BEGIN
            INSERT INTO reports_fts(reports_fts, rowid, title, content, tags)
            VALUES ('delete', old.id, old.title, old.content, '');
            INSERT INTO reports_fts(rowid, title, content, tags)
            VALUES (new.id, new.title, new.content, '');
        END;
        )"
    };

    for (const QString& sql : triggers) {
        QSqlQuery query(db);
        if (!query.exec(sql)) {
            LOG_WARNING(QString("创建触发器失败: %1").arg(query.lastError().text()));
            // 触发器失败不影响核心功能
        }
    }

    return true;
}

// ===========================================================================
// 数据库迁移
// ===========================================================================

bool DatabaseManager::migrate(int fromVersion, int toVersion)
{
    // 当前只有版本 1，暂无迁移逻辑
    // 未来版本升级时，在这里添加增量迁移脚本
    // 例如：if (fromVersion < 2) { /* v1 -> v2 迁移 */ }

    Q_UNUSED(fromVersion);
    Q_UNUSED(toVersion);

    LOG_INFO("数据库迁移完成（当前版本无需增量迁移）");
    return true;
}

// ===========================================================================
// 版本管理
// ===========================================================================

int DatabaseManager::getStoredVersion()
{
    QSqlDatabase db = database();
    QSqlQuery query(db);
    query.prepare("SELECT value FROM app_meta WHERE key = 'database_version';");
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;  // 不存在则视为版本 0
}

void DatabaseManager::setStoredVersion(int version)
{
    QSqlDatabase db = database();
    QSqlQuery query(db);
    // INSERT OR REPLACE：不存在则插入，存在则更新
    query.prepare("INSERT OR REPLACE INTO app_meta (key, value) VALUES ('database_version', :version);");
    query.bindValue(":version", QString::number(version));
    query.exec();
}

// ===========================================================================
// 内置数据初始化
// ===========================================================================

bool DatabaseManager::seedBuiltinTemplates()
{
    QSqlDatabase db = database();

    // 检查是否已有内置模板
    QSqlQuery check(db);
    check.exec("SELECT COUNT(*) FROM templates WHERE is_builtin = 1;");
    if (check.next() && check.value(0).toInt() > 0) {
        return true;  // 已有内置模板，跳过
    }

    // 插入通用实验报告模板
    QSqlQuery insert(db);
    insert.prepare(R"(
        INSERT INTO templates (name, category, description, structure, is_builtin)
        VALUES (:name, :category, :description, :structure, 1);
    )");

    // 通用模板结构
    const QJsonArray generalStructure = {
        QJsonObject{{"type", "heading1"}, {"data", QJsonObject{{"text", "实验名称"}}}},
        QJsonObject{{"type", "paragraph"}, {"data", QJsonObject{{"text", ""}}}},
        QJsonObject{{"type", "heading1"}, {"data", QJsonObject{{"text", "实验目的"}}}},
        QJsonObject{{"type", "paragraph"}, {"data", QJsonObject{{"text", ""}}}},
        QJsonObject{{"type", "heading1"}, {"data", QJsonObject{{"text", "实验原理"}}}},
        QJsonObject{{"type", "paragraph"}, {"data", QJsonObject{{"text", ""}}}},
        QJsonObject{{"type", "heading1"}, {"data", QJsonObject{{"text", "实验器材"}}}},
        QJsonObject{{"type", "paragraph"}, {"data", QJsonObject{{"text", ""}}}},
        QJsonObject{{"type", "heading1"}, {"data", QJsonObject{{"text", "实验步骤"}}}},
        QJsonObject{{"type", "numbered_list"}, {"data", QJsonObject{{"items", QJsonArray()}}}},
        QJsonObject{{"type", "heading1"}, {"data", QJsonObject{{"text", "实验数据"}}}},
        QJsonObject{{"type", "table"}, {"data", QJsonObject()}},
        QJsonObject{{"type", "heading1"}, {"data", QJsonObject{{"text", "数据分析与图表"}}}},
        QJsonObject{{"type", "chart"}, {"data", QJsonObject()}},
        QJsonObject{{"type", "heading1"}, {"data", QJsonObject{{"text", "实验结论"}}}},
        QJsonObject{{"type", "paragraph"}, {"data", QJsonObject{{"text", ""}}}},
        QJsonObject{{"type", "heading1"}, {"data", QJsonObject{{"text", "误差分析"}}}},
        QJsonObject{{"type", "paragraph"}, {"data", QJsonObject{{"text", ""}}}},
        QJsonObject{{"type", "heading1"}, {"data", QJsonObject{{"text", "思考题"}}}},
        QJsonObject{{"type", "paragraph"}, {"data", QJsonObject{{"text", ""}}}}
    };

    const QString generalJson = QString::fromUtf8(
        QJsonDocument(generalStructure).toJson(QJsonDocument::Compact));

    insert.bindValue(":name", "通用实验报告");
    insert.bindValue(":category", "general");
    insert.bindValue(":description", "适用于各类实验的通用报告模板");
    insert.bindValue(":structure", generalJson);

    if (!insert.exec()) {
        LOG_ERROR(QString("插入内置模板失败: %1").arg(db.lastError().text()));
        return false;
    }

    LOG_INFO("内置模板初始化完成");
    return true;
}

// ===========================================================================
// 公共接口
// ===========================================================================

QSqlDatabase DatabaseManager::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

void DatabaseManager::close()
{
    QMutexLocker locker(&m_mutex);

    if (m_initialized) {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        if (db.isOpen()) {
            // 确保所有数据写入磁盘
            db.exec("PRAGMA wal_checkpoint(TRUNCATE);");
            db.close();
        }
        QSqlDatabase::removeDatabase(m_connectionName);
        m_initialized = false;
        LOG_INFO("数据库已关闭");
    }
}

bool DatabaseManager::executeSqlFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR(QString("无法打开 SQL 文件: %1").arg(filePath));
        return false;
    }

    const QString sql = QString::fromUtf8(file.readAll());
    file.close();

    return executeSql(sql);
}

bool DatabaseManager::executeSql(const QString& sql, QString* errorMsg)
{
    QSqlDatabase db = database();
    QSqlQuery query(db);

    // 支持多语句执行（以分号分隔）
    // 注意：QSqlQuery 一次只能执行一条语句，这里简单分割
    // 更复杂的脚本应使用事务 + 逐条执行
    const QStringList statements = sql.split(';', Qt::SkipEmptyParts);

    bool allOk = true;
    for (const QString& stmt : statements) {
        const QString trimmed = stmt.trimmed();
        if (trimmed.isEmpty()) continue;

        if (!query.exec(trimmed)) {
            const QString err = query.lastError().text();
            LOG_ERROR(QString("SQL 执行失败: %1\n语句: %2").arg(err).arg(trimmed.left(200)));
            if (errorMsg) *errorMsg = err;
            allOk = false;
            break;
        }
    }

    return allOk;
}

bool DatabaseManager::transaction()
{
    return database().transaction();
}

bool DatabaseManager::commit()
{
    return database().commit();
}

bool DatabaseManager::rollback()
{
    return database().rollback();
}

QString DatabaseManager::lastError() const
{
    return database().lastError().text();
}
