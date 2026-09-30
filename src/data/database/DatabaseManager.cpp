/**
 * @file DatabaseManager.cpp
 * @brief 数据库管理器实现文件
 */

#include "DatabaseManager.h"
#include "core/utils/Logger.h"
#include "core/utils/AppConstants.h"
#include "data/repositories/UserRepository.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>

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
    m_mainThread = QThread::currentThread();  // 记录主连接所属线程
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

    LOG_DEBUG(QString("数据库已打开: %1").arg(dbPath));

    // -----------------------------------------------------------------------
    // SQLite 性能与功能设置
    // -----------------------------------------------------------------------

    // WAL 模式：允许并发读写，大幅提升写入性能
    // 注意：WAL 模式在网络文件系统上可能有问题，但本地文件没问题
    QSqlQuery pragmaQuery(db);
    pragmaQuery.exec("PRAGMA journal_mode = WAL;");

    // 启用外键约束（SQLite 默认关闭）
    pragmaQuery.exec("PRAGMA foreign_keys = ON;");

    // 同步模式：NORMAL 在 WAL 模式下足够安全，且性能更好
    // FULL 更安全但慢，NORMAL 是推荐值
    pragmaQuery.exec("PRAGMA synchronous = NORMAL;");

    // 缓存大小：设置为 64MB（单位是页，默认页大小 4096 字节）
    // 64 * 1024 * 1024 / 4096 = 16384 页
    pragmaQuery.exec("PRAGMA cache_size = -16384;");

    // 忙等待超时：多人共享数据库时，另一会话短暂持锁不会立刻报
    // "database is locked"，最多等待 5 秒（配合 WAL 显著降低并发写冲突）
    pragmaQuery.exec("PRAGMA busy_timeout = 5000;");

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
        LOG_DEBUG(QString("需要数据库迁移: %1 -> %2").arg(storedVersion).arg(targetVersion));
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

    // 初始化默认用户（admin 和 test）
    UserRepository::initializeDefaultUsers();

    m_initialized = true;
    LOG_DEBUG(QString("数据库初始化完成，版本: %1").arg(m_currentVersion));
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
            created_by      INTEGER DEFAULT -1,
            modified_by     INTEGER DEFAULT -1,
            version         INTEGER DEFAULT 1,
            word_count      INTEGER DEFAULT 0,
            experiment_date DATE,
            last_action     TEXT DEFAULT '',
            last_action_by  INTEGER DEFAULT -1,
            last_action_at  DATETIME,
            last_action_comment TEXT DEFAULT '',
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
            visibility  TEXT DEFAULT 'public',
            created_by  INTEGER DEFAULT -1,
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
            report_id   INTEGER NOT NULL DEFAULT 0,
            name        TEXT NOT NULL,
            columns     TEXT DEFAULT '[]',
            rows        TEXT DEFAULT '[]',
            created_at  DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at  DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";

    // -----------------------------------------------------------------------
    // 标签表
    // -----------------------------------------------------------------------
    const QString createTags = R"(
        CREATE TABLE IF NOT EXISTS tags (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            name        TEXT NOT NULL UNIQUE,
            color       TEXT DEFAULT '#4A90D9',
            description TEXT DEFAULT '',
            created_at  TEXT DEFAULT ''
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
    // 用户表（多用户支持）
    // -----------------------------------------------------------------------
    const QString createUsers = R"(
        CREATE TABLE IF NOT EXISTS users (
            id                   INTEGER PRIMARY KEY AUTOINCREMENT,
            username             TEXT NOT NULL UNIQUE,
            password_hash        TEXT NOT NULL,
            display_name         TEXT DEFAULT '',
            role                 TEXT DEFAULT 'user',
            must_change_password INTEGER DEFAULT 0,
            disabled             INTEGER DEFAULT 0,
            group_id             INTEGER DEFAULT -1,
            created_at           DATETIME DEFAULT CURRENT_TIMESTAMP,
            last_login_at        DATETIME
        );
    )";

    // -----------------------------------------------------------------------
    // 组（班级/课题组）表
    // -----------------------------------------------------------------------
    const QString createGroups = R"(
        CREATE TABLE IF NOT EXISTS groups (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            name        TEXT NOT NULL,
            description TEXT DEFAULT '',
            leader_id   INTEGER DEFAULT -1,
            created_at  DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at  DATETIME DEFAULT CURRENT_TIMESTAMP
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
        createUsers,
        createGroups,
        createAppMeta
    };

    for (const QString& sql : statements) {
        // QSqlDatabase::exec() 返回 QSqlQuery 对象，不能直接用 ! 取反
        // 需要通过 QSqlQuery::isActive() 或 lastError() 判断执行结果
        QSqlQuery query(db);
        if (!query.exec(sql)) {
            LOG_ERROR(QString("建表失败: %1\nSQL: %2")
                         .arg(query.lastError().text())
                         .arg(sql.left(200)));
            return false;
        }
    }

    LOG_DEBUG("所有表创建完成");
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
        "CREATE INDEX IF NOT EXISTS idx_templates_category ON templates(category);",
        // 按标签反向查报告（WHERE tag_id=:id）时主键 (report_id, tag_id) 无前缀索引可用
        "CREATE INDEX IF NOT EXISTS idx_report_tags_tag_id ON report_tags(tag_id);"
    };

    for (const QString& sql : indexes) {
        // 使用 QSqlQuery 对象执行 SQL，正确检查执行结果
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

    // 使用 QSqlQuery 对象执行 SQL，正确检查执行结果
    QSqlQuery ftsQuery(db);
    if (!ftsQuery.exec(createFts)) {
        // FTS5 可能不可用（某些 Qt 编译版本未启用），记录警告但不失败
        LOG_WARNING(QString("FTS5 全文索引不可用: %1").arg(ftsQuery.lastError().text()));
        LOG_WARNING("将使用 LIKE 模糊查询作为降级方案");
        return true;
    }

    LOG_DEBUG("FTS5 全文索引可用，搜索走全文检索");
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
        // 使用 QSqlQuery 对象执行 SQL，正确检查执行结果
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
    QSqlDatabase db = database();
    QSqlQuery query(db);

    // 整个迁移过程包在单个事务中：中途任一 ALTER/CREATE 失败则整体回滚，
    // 避免留下"部分迁移"状态（前几步已执行但数据库版本号未更新）
    if (!db.transaction()) {
        LOG_ERROR(QString("迁移事务启动失败: %1").arg(db.lastError().text()));
        return false;
    }

    // 辅助函数：检查表中是否存在某列
    auto hasColumn = [&](const QString& table, const QString& column) -> bool {
        QSqlQuery colQuery(db);
        colQuery.exec(QString("PRAGMA table_info(%1);").arg(table));
        while (colQuery.next()) {
            if (colQuery.value(1).toString() == column) return true;
        }
        return false;  // 列不存在（正常判断结果，非迁移失败，不能回滚事务）
    };

    // v1 -> v2: tags 表添加 description 和 created_at 字段
    if (fromVersion < 2) {
        LOG_DEBUG("执行 v1 -> v2 数据库迁移: tags 表添加字段");

        // 检查列是否已存在（避免重复添加）
        bool hasDescription = false;
        bool hasCreatedAt = false;
        query.exec("PRAGMA table_info(tags);");
        while (query.next()) {
            const QString colName = query.value(1).toString();
            if (colName == "description") hasDescription = true;
            if (colName == "created_at") hasCreatedAt = true;
        }

        if (!hasDescription) {
            if (!query.exec("ALTER TABLE tags ADD COLUMN description TEXT DEFAULT '';")) {
                LOG_ERROR(QString("迁移失败: 添加 description 列 - %1").arg(query.lastError().text()));
                db.rollback();

                return false;
            }
        }

        if (!hasCreatedAt) {
            if (!query.exec("ALTER TABLE tags ADD COLUMN created_at TEXT DEFAULT '';")) {
                LOG_ERROR(QString("迁移失败: 添加 created_at 列 - %1").arg(query.lastError().text()));
                db.rollback();

                return false;
            }
        }

        LOG_DEBUG("v1 -> v2 迁移完成");
    }

    // v2 -> v3: 添加 users 表，projects/reports 表加 created_by 和 version 字段
    if (fromVersion < 3) {
        LOG_DEBUG("执行 v2 -> v3 数据库迁移: 多用户支持");

        // 1. 创建 users 表
        const QString createUsers = R"(
            CREATE TABLE IF NOT EXISTS users (
                id                   INTEGER PRIMARY KEY AUTOINCREMENT,
                username             TEXT NOT NULL UNIQUE,
                password_hash        TEXT NOT NULL,
                display_name         TEXT DEFAULT '',
                role                 TEXT DEFAULT 'user',
                must_change_password INTEGER DEFAULT 0,
                created_at           DATETIME DEFAULT CURRENT_TIMESTAMP,
                last_login_at        DATETIME
            );
        )";
        if (!query.exec(createUsers)) {
            LOG_ERROR(QString("迁移失败: 创建 users 表 - %1").arg(query.lastError().text()));
            db.rollback();

            return false;
        }

        // 2. projects 表加 created_by 字段
        if (!hasColumn("projects", "created_by")) {
            if (!query.exec("ALTER TABLE projects ADD COLUMN created_by INTEGER DEFAULT -1;")) {
                LOG_ERROR(QString("迁移失败: projects 添加 created_by - %1").arg(query.lastError().text()));
                db.rollback();

                return false;
            }
        }

        // 4. reports 表加 created_by 字段
        if (!hasColumn("reports", "created_by")) {
            if (!query.exec("ALTER TABLE reports ADD COLUMN created_by INTEGER DEFAULT -1;")) {
                LOG_ERROR(QString("迁移失败: reports 添加 created_by - %1").arg(query.lastError().text()));
                db.rollback();

                return false;
            }
        }

        // 5. reports 表加 version 字段（乐观锁）
        if (!hasColumn("reports", "version")) {
            if (!query.exec("ALTER TABLE reports ADD COLUMN version INTEGER DEFAULT 1;")) {
                LOG_ERROR(QString("迁移失败: reports 添加 version - %1").arg(query.lastError().text()));
                db.rollback();

                return false;
            }
        }

        LOG_DEBUG("v2 -> v3 迁移完成");
    }

    // v3 -> v4: reports 表加 word_count 字段（字数统计缓存）
    if (fromVersion <= 3) {
        if (!hasColumn("reports", "word_count")) {
            if (!query.exec("ALTER TABLE reports ADD COLUMN word_count INTEGER DEFAULT 0;")) {
                LOG_ERROR(QString("迁移失败: reports 添加 word_count - %1").arg(query.lastError().text()));
                db.rollback();

                return false;
            }
        }
        LOG_DEBUG("v3 -> v4 迁移完成");
    }

    // v4 -> v5: 重建 data_tables 表，去掉 report_id 外键约束
    //（全局数据表约定 report_id=0，外键约束下插入失败导致数据表保存失败）
    if (fromVersion < 5) {
        LOG_DEBUG("执行 v4 -> v5 数据库迁移: 重建 data_tables 去掉外键");
        if (query.exec("ALTER TABLE data_tables RENAME TO data_tables_old;")) {
            const QString recreate = R"(
                CREATE TABLE data_tables (
                    id          INTEGER PRIMARY KEY AUTOINCREMENT,
                    report_id   INTEGER NOT NULL DEFAULT 0,
                    name        TEXT NOT NULL,
                    columns     TEXT DEFAULT '[]',
                    rows        TEXT DEFAULT '[]',
                    created_at  DATETIME DEFAULT CURRENT_TIMESTAMP,
                    updated_at  DATETIME DEFAULT CURRENT_TIMESTAMP
                );
            )";
            if (!query.exec(recreate)) {
                LOG_ERROR(QString("迁移失败: 重建 data_tables - %1").arg(query.lastError().text()));
                db.rollback();
                return false;
            }
            if (!query.exec("INSERT INTO data_tables (id, report_id, name, columns, rows, created_at, updated_at) "
                            "SELECT id, report_id, name, columns, rows, created_at, updated_at FROM data_tables_old;")) {
                LOG_ERROR(QString("迁移失败: 拷贝 data_tables 数据 - %1").arg(query.lastError().text()));
                db.rollback();
                return false;
            }
            if (!query.exec("DROP TABLE data_tables_old;")) {
                LOG_ERROR(QString("迁移失败: 删除旧 data_tables - %1").arg(query.lastError().text()));
                db.rollback();
                return false;
            }
            LOG_DEBUG("v4 -> v5 迁移完成");
        } else {
            LOG_WARNING("data_tables 重命名失败（可能表不存在），跳过 v4->v5 迁移");
        }
    }

    // v5 -> v6: reports 表加 modified_by（最后修改者用户 ID）
    if (fromVersion < 6) {
        if (!hasColumn("reports", "modified_by")) {
            LOG_DEBUG("执行 v5 -> v6 数据库迁移: reports 添加 modified_by");
            if (!query.exec("ALTER TABLE reports ADD COLUMN modified_by INTEGER DEFAULT -1;")) {
                LOG_ERROR(QString("迁移失败: reports 添加 modified_by - %1").arg(query.lastError().text()));
                db.rollback();
                return false;
            }
        }
    }

    // v6 -> v7: 四级角色+组+工作流+模板可见性
    if (fromVersion < 7) {
        LOG_DEBUG("执行 v6 -> v7 数据库迁移: 角色/组/工作流/模板可见性");

        // 1. users 表加 disabled + group_id
        if (!hasColumn("users", "disabled")) {
            if (!query.exec("ALTER TABLE users ADD COLUMN disabled INTEGER DEFAULT 0;")) {
                LOG_ERROR(QString("迁移失败: users 添加 disabled - %1").arg(query.lastError().text()));
                db.rollback(); return false;
            }
        }
        if (!hasColumn("users", "group_id")) {
            if (!query.exec("ALTER TABLE users ADD COLUMN group_id INTEGER DEFAULT -1;")) {
                LOG_ERROR(QString("迁移失败: users 添加 group_id - %1").arg(query.lastError().text()));
                db.rollback(); return false;
            }
        }

        // 2. 新建 groups 表
        const QString createGroupsMigrate = R"(
            CREATE TABLE IF NOT EXISTS groups (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                name        TEXT NOT NULL,
                description TEXT DEFAULT '',
                leader_id   INTEGER DEFAULT -1,
                created_at  DATETIME DEFAULT CURRENT_TIMESTAMP,
                updated_at  DATETIME DEFAULT CURRENT_TIMESTAMP
            );
        )";
        if (!query.exec(createGroupsMigrate)) {
            LOG_ERROR(QString("迁移失败: 创建 groups 表 - %1").arg(query.lastError().text()));
            db.rollback(); return false;
        }

        // 3. reports 表加 last_action 四字段（工作流最后操作记录）
        if (!hasColumn("reports", "last_action")) {
            if (!query.exec("ALTER TABLE reports ADD COLUMN last_action TEXT DEFAULT '';")) {
                LOG_ERROR(QString("迁移失败: reports 添加 last_action - %1").arg(query.lastError().text()));
                db.rollback(); return false;
            }
        }
        if (!hasColumn("reports", "last_action_by")) {
            if (!query.exec("ALTER TABLE reports ADD COLUMN last_action_by INTEGER DEFAULT -1;")) {
                LOG_ERROR(QString("迁移失败: reports 添加 last_action_by - %1").arg(query.lastError().text()));
                db.rollback(); return false;
            }
        }
        if (!hasColumn("reports", "last_action_at")) {
            if (!query.exec("ALTER TABLE reports ADD COLUMN last_action_at DATETIME;")) {
                LOG_ERROR(QString("迁移失败: reports 添加 last_action_at - %1").arg(query.lastError().text()));
                db.rollback(); return false;
            }
        }
        if (!hasColumn("reports", "last_action_comment")) {
            if (!query.exec("ALTER TABLE reports ADD COLUMN last_action_comment TEXT DEFAULT '';")) {
                LOG_ERROR(QString("迁移失败: reports 添加 last_action_comment - %1").arg(query.lastError().text()));
                db.rollback(); return false;
            }
        }

        // 4. templates 表加 visibility（私有/全局）+ created_by（创建者）
        if (!hasColumn("templates", "visibility")) {
            if (!query.exec("ALTER TABLE templates ADD COLUMN visibility TEXT DEFAULT 'public';")) {
                LOG_ERROR(QString("迁移失败: templates 添加 visibility - %1").arg(query.lastError().text()));
                db.rollback(); return false;
            }
        }
        if (!hasColumn("templates", "created_by")) {
            if (!query.exec("ALTER TABLE templates ADD COLUMN created_by INTEGER DEFAULT -1;")) {
                LOG_ERROR(QString("迁移失败: templates 添加 created_by - %1").arg(query.lastError().text()));
                db.rollback(); return false;
            }
        }

        // 5. 角色字符串迁移：admin→super_admin, user→member（旧数据兼容）
        query.prepare("UPDATE users SET role = 'super_admin' WHERE role = 'admin';");
        if (!query.exec()) {
            LOG_ERROR(QString("迁移失败: 角色 admin→super_admin - %1").arg(query.lastError().text()));
            db.rollback(); return false;
        }
        query.prepare("UPDATE users SET role = 'member' WHERE role = 'user';");
        if (!query.exec()) {
            LOG_ERROR(QString("迁移失败: 角色 user→member - %1").arg(query.lastError().text()));
            db.rollback(); return false;
        }

        LOG_DEBUG("v6 -> v7 迁移完成");
    }

    // v7 -> v8: 审计日志表（敏感操作留痕，供合规追溯）
    if (fromVersion < 8) {
        LOG_DEBUG("执行 v7 -> v8 数据库迁移: 审计日志表");
        const QString createAuditLogs = R"(
            CREATE TABLE IF NOT EXISTS audit_logs (
                id         INTEGER PRIMARY KEY AUTOINCREMENT,
                user_id    INTEGER NOT NULL DEFAULT -1,
                username   TEXT DEFAULT '',
                action     TEXT NOT NULL,
                detail     TEXT DEFAULT '',
                report_id  INTEGER DEFAULT -1,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP
            );
        )";
        if (!query.exec(createAuditLogs)) {
            LOG_ERROR(QString("迁移失败: 创建 audit_logs 表 - %1").arg(query.lastError().text()));
            db.rollback(); return false;
        }
        LOG_DEBUG("v7 -> v8 迁移完成");
    }

    Q_UNUSED(toVersion);
    LOG_DEBUG("数据库迁移完成");
    db.commit();
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
    if (!query.exec()) {
        LOG_ERROR(QString("写入数据库版本号失败: %1").arg(query.lastError().text()));
    }
}

// ===========================================================================
// 内置数据初始化
// ===========================================================================

bool DatabaseManager::seedBuiltinTemplates()
{
    QSqlDatabase db = database();

    // 检查是否已有内置模板：仅接受最新 v4（纯段落）模板，其余一律重建
    QSqlQuery check(db);
    check.exec("SELECT structure FROM templates WHERE is_builtin = 1 LIMIT 1;");
    if (check.next()) {
        const QString structure = check.value(0).toString();
        if (structure.contains("\"version\": 4")) {
            return true;  // 已是 v4 模板
        }
        // 旧版内置模板：删除后重建（保留自定义模板）
        LOG_DEBUG("检测到旧版内置模板，删除重建为 v4 纯段落模板");
        QSqlQuery del(db);
        if (!del.exec("DELETE FROM templates WHERE is_builtin = 1;")) {
            LOG_ERROR(QString("删除旧版内置模板失败: %1").arg(del.lastError().text()));
            return false;
        }
    }

    // 插入通用实验报告模板
    QSqlQuery insert(db);
    insert.prepare(R"(
        INSERT INTO templates (name, category, description, structure, is_builtin)
        VALUES (:name, :category, :description, :structure, 1);
    )");

    // 通用模板结构（新版 v2：连续文档 + 结构化对象锚点）
    const QString tableObjId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString chartObjId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // 模板标题全部用普通段落 <p> + 显式字号（不引入 h1/h2/h3 标题语义，
    // 标题只是大一号的普通文字，与编辑器的"无标题/正文分类"保持一致）
    const QString documentHtml = QString(
        "<p style=\"font-size:20pt;\">实验名称</p>\n<p></p>\n"
        "<p style=\"font-size:20pt;\">实验目的</p>\n<p></p>\n"
        "<p style=\"font-size:20pt;\">实验原理</p>\n<p></p>\n"
        "<p style=\"font-size:20pt;\">实验器材</p>\n<p></p>\n"
        "<p style=\"font-size:20pt;\">实验步骤</p>\n<p></p>\n"
        "<p style=\"font-size:20pt;\">实验数据</p>\n<p><img src=\"object://table/%1\"/></p>\n"
        "<p style=\"font-size:20pt;\">数据分析与图表</p>\n<p><img src=\"object://chart/%2\"/></p>\n"
        "<p style=\"font-size:20pt;\">实验结论</p>\n<p></p>\n"
        "<p style=\"font-size:20pt;\">误差分析</p>\n<p></p>\n"
        "<p style=\"font-size:20pt;\">思考题</p>\n<p></p>\n").arg(tableObjId, chartObjId);

    QJsonObject root;
    root["version"] = 4;
    root["document"] = documentHtml;
    QJsonArray objects;
    QJsonObject tableObj;
    tableObj["id"] = tableObjId;
    tableObj["type"] = "table";
    tableObj["data"] = QJsonObject();
    QJsonObject chartObj;
    chartObj["id"] = chartObjId;
    chartObj["type"] = "chart";
    chartObj["data"] = QJsonObject();
    objects.append(tableObj);
    objects.append(chartObj);
    root["objects"] = objects;

    const QString generalJson = QString::fromUtf8(
        QJsonDocument(root).toJson(QJsonDocument::Compact));

    insert.bindValue(":name", "通用实验报告");
    insert.bindValue(":category", "general");
    insert.bindValue(":description", "适用于各类实验的通用报告模板");
    insert.bindValue(":structure", generalJson);

    if (!insert.exec()) {
        LOG_ERROR(QString("插入内置模板失败: %1").arg(db.lastError().text()));
        return false;
    }

    LOG_DEBUG("内置模板初始化完成");
    return true;
}

// ===========================================================================
// 公共接口
// ===========================================================================

QSqlDatabase DatabaseManager::database() const
{
    // QSqlDatabase 连接绑定创建线程，不可跨线程使用。
    // 主线程直接返回主连接；其他线程（如导出后台线程）惰性创建线程本地连接
    // （SQLite WAL 模式支持多线程并发读，busy_timeout 处理短暂写锁竞争）。
    if (QThread::currentThread() == m_mainThread) {
        return QSqlDatabase::database(m_connectionName);
    }

    static thread_local bool tlsReady = false;
    static thread_local QSqlDatabase tlsDb;
    if (!tlsReady) {
        tlsReady = true;
        const QString name = QStringLiteral("ert_thread_%1")
            .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
        tlsDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
        if (!m_dbPath.isEmpty()) {
            tlsDb.setDatabaseName(m_dbPath);
            if (tlsDb.open()) {
                QSqlQuery q(tlsDb);
                q.exec("PRAGMA journal_mode = WAL;");
                q.exec("PRAGMA foreign_keys = ON;");
                q.exec("PRAGMA busy_timeout = 5000;");
            } else {
                LOG_ERROR(QStringLiteral("线程本地数据库连接打开失败: %1")
                              .arg(tlsDb.lastError().text()));
            }
        }
    }
    return tlsDb;
}

void DatabaseManager::close()
{
    QMutexLocker locker(&m_mutex);

    if (m_initialized) {
        {
            // 块作用域：确保 db/pragmaQuery 在 removeDatabase 前销毁，避免 "connection still in use"
            QSqlDatabase db = QSqlDatabase::database(m_connectionName);
            if (db.isOpen()) {
                QSqlQuery pragmaQuery(db);
                pragmaQuery.exec("PRAGMA wal_checkpoint(TRUNCATE);");
                pragmaQuery.finish();
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(m_connectionName);
        m_initialized = false;
        LOG_DEBUG("数据库已关闭");
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
            LOG_ERROR(QString("SQL 执行失败: %1\n语句: %2").arg(err, trimmed.left(200)));
            if (errorMsg) *errorMsg = err;
            allOk = false;
            break;
        }
    }

    return allOk;
}

bool DatabaseManager::transaction()
{
    if (!database().transaction()) {
        LOG_ERROR(QString("事务启动失败: %1").arg(database().lastError().text()));
        return false;
    }
    LOG_DEBUG("事务已启动");
    return true;
}

bool DatabaseManager::commit()
{
    if (!database().commit()) {
        LOG_ERROR(QString("事务提交失败: %1").arg(database().lastError().text()));
        return false;
    }
    LOG_DEBUG("事务已提交");
    return true;
}

bool DatabaseManager::rollback()
{
    if (!database().rollback()) {
        LOG_ERROR(QString("事务回滚失败: %1").arg(database().lastError().text()));
        return false;
    }
    LOG_DEBUG("事务已回滚");
    return true;
}

QString DatabaseManager::lastError() const
{
    return database().lastError().text();
}

// ===========================================================================
// 统一错误处理辅助方法
// ===========================================================================

bool DatabaseManager::executeQuery(QSqlQuery& query, const QString& operation, bool logError)
{
    // 执行查询
    if (!query.exec()) {
        if (logError) {
            // 记录详细的错误日志，包括操作描述、SQL语句和错误信息
            LOG_ERROR(QString("数据库操作失败 [%1]: %2\nSQL: %3")
                         .arg(operation,
                              query.lastError().text(),
                              query.lastQuery()));
        }
        return false;
    }

    // 执行成功
    LOG_DEBUG(QString("数据库操作成功 [%1], 影响行数: %2")
                  .arg(operation)
                  .arg(query.numRowsAffected()));

    return true;
}

QSqlQuery& DatabaseManager::executeQueryWithResult(QSqlQuery& query, const QString& operation, bool& ok)
{
    // 执行查询
    ok = executeQuery(query, operation);
    return query;
}
