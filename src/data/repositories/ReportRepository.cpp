/**
 * @file ReportRepository.cpp
 * @brief 报告仓储类实现文件
 *
 * 本文件实现了 ReportRepository 类，负责实验报告实体的持久化操作。
 * 报告是系统的核心实体，包含结构化的内容（富文本、表格、图片、图表等）。
 *
 * 主要功能：
 * - 报告的 CRUD（创建、读取、更新、删除）操作
 * - 动态条件查询（支持多条件组合、排序、分页）
 * - 全文检索（FTS5 全文索引，带降级 LIKE 方案）
 * - 报告版本管理（保存快照、恢复、删除）
 * - 报告数量统计（按项目、按状态）
 *
 * 设计说明：
 * - 采用 Repository 模式，将数据访问逻辑与业务逻辑分离
 * - 所有 SQL 操作使用预处理语句（prepare + bindValue），防止 SQL 注入
 * - 报告内容以 JSON 字符串形式存储在 content 字段中
 * - 全文检索使用 SQLite FTS5 虚拟表，通过触发器自动同步
 * - 版本管理通过 report_versions 表存储历史快照
 * - 使用 QSharedPointer 管理报告对象的生命周期
 *
 * 数据库表结构：
 * - reports：报告主表（id, project_id, template_id, title, content, status, author, ...）
 * - reports_fts：FTS5 全文索引虚拟表（通过触发器自动同步）
 * - report_versions：报告版本历史表
 */

#include "ReportRepository.h"
#include "data/database/DatabaseManager.h"
#include "core/utils/Logger.h"
#include "core/utils/AppConstants.h"
#include "core/utils/AppConfig.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDateTime>

// ===========================================================================
// 内部辅助方法
// ===========================================================================

/**
 * @brief 将数据库查询结果映射为 Report 对象
 *
 * 从 QSqlQuery 的当前行读取各字段值，构建并填充 Report 对象。
 * 报告内容从 JSON 字符串反序列化为内存中的内容块列表。
 *
 * @param query 已执行的数据库查询对象，当前行包含报告数据
 * @return Report::Ptr 填充好的报告智能指针
 *
 * @note 调用前需确保 query.next() 已返回 true，即当前行有效
 * @note content 字段存储为 JSON 字符串，需调用 contentFromJson 反序列化
 * @note template_id 可能为 NULL（未使用模板），toLongLong() 会返回 0
 */
Report::Ptr ReportRepository::mapToReport(const QSqlQuery& query)
{
    // 创建空的报告对象
    Report::Ptr report = Report::create();

    // 映射基本字段
    report->setId(query.value("id").toLongLong());                    // 报告唯一 ID
    report->setProjectId(query.value("project_id").toLongLong());     // 所属项目 ID
    report->setTemplateId(query.value("template_id").toLongLong());   // 使用的模板 ID（0 表示未使用模板）
    report->setTitle(query.value("title").toString());                // 报告标题
    report->setStatus(Report::statusFromString(query.value("status").toString())); // 报告状态
    report->setAuthor(query.value("author").toString());              // 作者/实验者
    report->setCreatedBy(query.value("created_by").toLongLong());      // 创建者用户 ID
    report->setModifiedBy(query.value("modified_by").toLongLong());    // 最后修改者用户 ID
    report->setVersion(query.value("version").toInt());                // 版本号（乐观锁）
    report->setWordCount(query.value("word_count").toInt());           // 字数统计缓存
    report->setExperimentDate(query.value("experiment_date").toDate()); // 实验日期
    report->setLastAction(query.value("last_action").toString());       // 最后工作流操作
    report->setLastActionBy(query.value("last_action_by").toLongLong()); // 最后操作人
    report->setLastActionAt(query.value("last_action_at").toDateTime()); // 最后操作时间
    report->setLastActionComment(query.value("last_action_comment").toString()); // 最后操作意见
    report->setCreatedAt(query.value("created_at").toDateTime());     // 创建时间
    report->setUpdatedAt(query.value("updated_at").toDateTime());     // 最后更新时间

    // 解析内容 JSON：将数据库中存储的 JSON 字符串转换为内存中的内容块列表
    report->contentFromJson(query.value("content").toString());

    return report;
}

Report::Ptr ReportRepository::fromQueryRow(const QSqlQuery& query)
{
    return mapToReport(query);
}

/**
 * @brief 检查 FTS5 全文索引是否可用
 *
 * 查询 sqlite_master 系统表，检查 reports_fts 虚拟表是否存在。
 * FTS5 表在数据库初始化时创建，如果创建失败则不可用。
 *
 * @return bool FTS5 可用返回 true，不可用返回 false
 *
 * @note 当 FTS5 不可用时，search() 方法会自动降级为 LIKE 模糊查询
 * @note 此方法每次调用都会执行一次查询，频繁调用时可考虑缓存结果
 */
bool ReportRepository::ftsAvailable()
{
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 查询 sqlite_master 系统表，检查 reports_fts 表是否存在
    query.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='reports_fts';");

    // 如果有结果行，说明表存在
    return query.next();
}

// ===========================================================================
// 查询操作
// ===========================================================================

/**
 * @brief 根据 ID 查找报告
 *
 * @param id 报告唯一 ID，必须大于 0
 * @return Report::Ptr 找到时返回报告对象，未找到或参数无效时返回 nullptr
 *
 * 使用场景：
 * - 打开报告编辑器时加载完整报告内容
 * - 验证报告是否存在
 * - 版本恢复前加载当前报告
 *
 * 错误处理：
 * - id <= 0 时直接返回 nullptr
 * - 数据库查询失败时记录错误日志并返回 nullptr
 */
Report::Ptr ReportRepository::findById(qint64 id)
{
    // 参数校验：无效 ID 直接返回，避免无效查询
    if (id <= 0) return nullptr;

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 使用预处理语句查询，防止 SQL 注入
    query.prepare("SELECT * FROM reports WHERE id = :id;");
    query.bindValue(":id", id);

    // 执行查询并检查错误
    if (!BaseRepository::execChecked(query, "findById ")) return nullptr;

    // 检查是否有结果行
    if (query.next()) {
        return mapToReport(query);
    }

    // 未找到
    return nullptr;
}

/**
 * @brief 动态条件查询报告列表
 *
 * 根据 ReportQuery 结构体中的条件动态构建 SQL 查询，支持：
 * - 按项目 ID 筛选
 * - 按模板 ID 筛选
 * - 按状态筛选
 * - 按作者筛选
 * - 按实验日期范围筛选
 * - 关键词搜索（FTS 不可用时降级为 LIKE）
 * - 自定义排序字段和排序方向
 * - 分页（LIMIT + OFFSET）
 *
 * @param query 查询条件结构体，包含所有可选筛选参数
 * @return Report::List 符合条件的报告列表
 *
 * 安全机制：
 * - 排序字段使用白名单验证，防止 SQL 注入
 * - 所有用户输入通过 bindValue 绑定，不直接拼接 SQL
 * - 排序方向只允许 ASC/DESC
 *
 * @note 当 query.status 为 -1 时表示不筛选状态
 * @note 当 query.limit <= 0 时表示不分页，返回所有结果
 * @note 关键词搜索在 FTS 可用时使用全文索引，否则使用 LIKE 降级方案
 */
Report::List ReportRepository::findAll(const ReportQuery& query)
{
    Report::List result;

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery sqlQuery(db);

    // 动态构建 SQL 查询，从 "WHERE 1=1" 开始便于追加条件
    QString sql = "SELECT * FROM reports WHERE 1=1";
    QVariantList bindValues;  // 按顺序存储绑定参数

    // 条件 1：按项目 ID 筛选
    if (query.projectId > 0) {
        sql += " AND project_id = :projectId";
        bindValues << query.projectId;
    }

    // 条件 2：按模板 ID 筛选
    if (query.templateId > 0) {
        sql += " AND template_id = :templateId";
        bindValues << query.templateId;
    }

    // 条件 3：按状态筛选
    // status 为 -1 时表示不筛选（枚举值从 0 开始）
    if (static_cast<int>(query.status) >= 0) {
        sql += " AND status = :status";
        // 创建临时 Report 对象，设置状态后调用 statusToString()
        // 因为 statusToString() 是实例方法，需要通过对象调用
        Report tempReport;
        tempReport.setStatus(query.status);
        bindValues << tempReport.statusToString();
    }

    // 条件 4：按作者筛选
    if (!query.author.isEmpty()) {
        sql += " AND author = :author";
        bindValues << query.author;
    }

    // 条件 5：按实验日期范围筛选（起始日期）
    if (query.dateFrom.isValid()) {
        sql += " AND experiment_date >= :dateFrom";
        bindValues << query.dateFrom;
    }

    // 条件 5：按实验日期范围筛选（结束日期）
    if (query.dateTo.isValid()) {
        sql += " AND experiment_date <= :dateTo";
        bindValues << query.dateTo;
    }

    // 条件 6：关键词搜索（仅在 FTS 不可用时使用 LIKE 降级方案）
    // FTS 可用时应使用 search() 方法进行全文检索
    if (!query.keyword.isEmpty() && !ftsAvailable()) {
        sql += " AND (title LIKE :keyword OR content LIKE :keyword)";
        // 使用 %keyword% 进行模糊匹配
        bindValues << QString("%%1%").arg(query.keyword);
    }

    // 排序：使用白名单验证排序字段，防止 SQL 注入
    const QString validSortColumns[] = {"title", "created_at", "updated_at", "experiment_date", "id"};
    QString sortColumn = "updated_at";  // 默认按更新时间排序
    for (const QString& col : validSortColumns) {
        if (query.sortBy == col) {
            sortColumn = col;
            break;
        }
    }
    // 排序方向只允许 ASC 或 DESC
    sql += QString(" ORDER BY %1 %2").arg(sortColumn)
               .arg(query.sortOrder == Qt::AscendingOrder ? "ASC" : "DESC");

    // 分页：LIMIT + OFFSET
    if (query.limit > 0) {
        sql += QString(" LIMIT %1 OFFSET %2").arg(query.limit).arg(query.offset);
    }
    sql += ";";

    // 准备并执行查询
    sqlQuery.prepare(sql);
    // 按索引顺序绑定参数值（因为使用的是位置占位符 :0, :1, ...）
    for (int i = 0; i < bindValues.size(); ++i) {
        sqlQuery.bindValue(i, bindValues.at(i));
    }

    // 执行查询并检查错误
    if (!sqlQuery.exec()) {
        LOG_ERROR(QString("findAll 失败: %1\nSQL: %2").arg(sqlQuery.lastError().text(), sqlQuery.lastQuery()));
        return result;
    }

    // 遍历所有结果行
    while (sqlQuery.next()) {
        result.append(mapToReport(sqlQuery));
    }

    return result;
}

/**
 * @brief 按项目 ID 查询报告列表
 *
 * 这是 findAll 的便捷方法，预设了按项目筛选和按更新时间降序排序。
 *
 * @param projectId 项目 ID
 * @return Report::List 该项目下的报告列表，按更新时间降序排列
 *
 * 使用场景：
 * - 在项目树中展开项目时显示该项目的所有报告
 * - 在报告列表中显示当前项目的报告
 */
Report::List ReportRepository::findByProject(qint64 projectId)
{
    ReportQuery query;
    query.projectId = projectId;
    query.sortBy = "updated_at";
    query.sortOrder = Qt::DescendingOrder;
    return findAll(query);
}

// ===========================================================================
// 写入操作
// ===========================================================================

/**
 * @brief 插入新报告到数据库
 *
 * @param report 要插入的报告对象（智能指针）
 * @return bool 插入成功返回 true，失败返回 false
 *
 * 执行流程：
 * 1. 校验指针非空
 * 2. 执行 INSERT 语句
 * 3. 获取数据库自动生成的 ID 并回写到对象
 * 4. 设置创建时间和更新时间
 * 5. FTS 全文索引通过触发器自动更新
 *
 * @note 插入成功后，report 对象的 id、createdAt、updatedAt 会被更新
 * @note 报告内容会被序列化为 JSON 字符串存储
 * @note template_id 为 0 时存储为 NULL（表示未使用模板）
 * @note 插入操作会触发 reports_ai 触发器，自动同步 FTS 索引
 */
bool ReportRepository::insert(Report::Ptr report)
{
    // 空指针检查
    if (!report) return false;

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 准备插入语句，使用 R"(...)" 原始字符串字面量避免转义
    query.prepare(R"(
        INSERT INTO reports (project_id, template_id, title, content, status, author, created_by, modified_by, version, word_count, experiment_date, last_action, last_action_by, last_action_at, last_action_comment, created_at, updated_at)
        VALUES (:project_id, :template_id, :title, :content, :status, :author, :created_by, :modified_by, :version, :word_count, :experiment_date, :last_action, :last_action_by, :last_action_at, :last_action_comment, :created_at, :updated_at);
    )");

    // 绑定参数值
    const QDateTime now = QDateTime::currentDateTime();
    query.bindValue(":project_id", report->projectId());
    // template_id 为 0 时存储为 NULL（SQLite 中 NULL 表示无值）
    query.bindValue(":template_id", report->templateId() > 0 ? report->templateId() : QVariant());
    query.bindValue(":title", report->title());
    query.bindValue(":content", report->contentToJson());  // 报告内容序列化为 JSON
    query.bindValue(":status", report->statusToString());
    query.bindValue(":author", report->author());
    query.bindValue(":created_by", report->createdBy() > 0 ? report->createdBy() : QVariant());
    query.bindValue(":modified_by", report->modifiedBy() > 0 ? report->modifiedBy() : QVariant());
    query.bindValue(":version", 1);
    query.bindValue(":word_count", report->wordCount());
    query.bindValue(":experiment_date", report->experimentDate());
    query.bindValue(":last_action", report->lastAction());
    query.bindValue(":last_action_by", report->lastActionBy() > 0 ? report->lastActionBy() : QVariant());
    query.bindValue(":last_action_at", report->lastActionAt().isValid() ? report->lastActionAt() : QVariant());
    query.bindValue(":last_action_comment", report->lastActionComment());
    query.bindValue(":created_at", now);
    query.bindValue(":updated_at", now);

    // 执行插入
    if (!BaseRepository::execChecked(query, "insert ")) return false;

    // 获取数据库自动生成的自增 ID，回写到对象
    report->setId(query.lastInsertId().toLongLong());
    report->setCreatedAt(now);
    report->setUpdatedAt(now);

    LOG_INFO(QString("报告已创建: id=%1, title='%2'").arg(report->id()).arg(report->title()));
    return true;
}

/**
 * @brief 更新已存在的报告
 *
 * @param report 要更新的报告对象（智能指针引用）
 * @return bool 更新成功返回 true，失败返回 false
 *
 * 执行流程：
 * 1. 校验指针非空且对象已持久化（id > 0）
 * 2. 执行 UPDATE 语句，更新所有可变字段
 * 3. 更新对象的 updatedAt 字段
 * 4. FTS 全文索引通过触发器自动更新
 *
 * @note 只更新可变字段：project_id、template_id、title、content、status、author、experiment_date、updated_at
 * @note id 和 created_at 不可修改
 * @note 更新操作会触发 reports_au 触发器，自动同步 FTS 索引
 * @note 未持久化的对象（id <= 0）会被拒绝，应使用 insert 方法
 */
bool ReportRepository::update(const Report::Ptr& report)
{
    // 校验：指针非空且已持久化
    if (!report || !report->isPersisted()) return false;

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 准备更新语句
    query.prepare(R"(
        UPDATE reports
        SET project_id = :project_id,
            template_id = :template_id,
            title = :title,
            content = :content,
            status = :status,
            author = :author,
            modified_by = :modified_by,
            version = version + 1,
            word_count = :word_count,
            experiment_date = :experiment_date,
            last_action = :last_action,
            last_action_by = :last_action_by,
            last_action_at = :last_action_at,
            last_action_comment = :last_action_comment,
            updated_at = :updated_at
        WHERE id = :id;
    )");

    // 绑定参数值
    query.bindValue(":project_id", report->projectId());
    query.bindValue(":template_id", report->templateId() > 0 ? report->templateId() : QVariant());
    query.bindValue(":title", report->title());
    query.bindValue(":content", report->contentToJson());
    query.bindValue(":status", report->statusToString());
    query.bindValue(":author", report->author());
    query.bindValue(":modified_by", report->modifiedBy() > 0 ? report->modifiedBy() : QVariant());
    query.bindValue(":word_count", report->wordCount());
    query.bindValue(":experiment_date", report->experimentDate());
    query.bindValue(":last_action", report->lastAction());
    query.bindValue(":last_action_by", report->lastActionBy() > 0 ? report->lastActionBy() : QVariant());
    query.bindValue(":last_action_at", report->lastActionAt().isValid() ? report->lastActionAt() : QVariant());
    query.bindValue(":last_action_comment", report->lastActionComment());
    query.bindValue(":updated_at", QDateTime::currentDateTime());
    query.bindValue(":id", report->id());

    // 执行更新
    if (!BaseRepository::execChecked(query, "update ")) return false;

    // 同步更新对象的时间戳
    report->setUpdatedAt(QDateTime::currentDateTime());
    return true;
}

/**
 * @brief 删除报告
 *
 * @param id 要删除的报告 ID
 * @return bool 删除成功返回 true，失败返回 false
 *
 * 执行流程：
 * 1. 校验 id > 0
 * 2. 开启事务
 * 3. 执行 DELETE 语句删除报告
 * 4. 提交事务
 * 5. FTS 全文索引通过触发器自动删除
 *
 * @warning 此操作不可恢复，删除后报告及其所有版本将永久丢失
 * @note id <= 0 时直接返回 false
 * @note 删除操作会触发 reports_ad 触发器，自动删除 FTS 索引
 * @note 目前未级联删除关联的数据表、附件等，调用方需自行处理
 * @todo 考虑添加级联删除或外键约束
 */
bool ReportRepository::remove(qint64 id)
{
    // 参数校验
    if (id <= 0) return false;

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 开启事务，保证删除操作的原子性
    if (!DatabaseManager::instance().transaction()) {
        LOG_ERROR("删除报告事务启动失败，取消删除");
        return false;
    }

    // 准备删除语句
    query.prepare("DELETE FROM reports WHERE id = :id;");
    query.bindValue(":id", id);

    // 执行删除
    if (!query.exec()) {
        LOG_ERROR(QString("remove 失败: %1\nSQL: %2").arg(query.lastError().text(), query.lastQuery()));
        DatabaseManager::instance().rollback();
        return false;
    }

    // 提交事务
    if (!DatabaseManager::instance().commit()) {
        LOG_ERROR("删除报告事务提交失败");
        DatabaseManager::instance().rollback();
        return false;
    }
    LOG_INFO(QString("报告已删除: id=%1").arg(id));
    return true;
}

// ===========================================================================
// 全文检索
// ===========================================================================

/**
 * @brief 全文检索报告
 *
 * 使用 SQLite FTS5 全文索引进行高效的全文检索，支持：
 * - 关键词匹配（标题和内容）
 * - 按项目 ID 筛选
 * - 结果数量限制
 * - 相关度排序（BM25 算法）
 * - 高亮摘要（snippet 函数）
 *
 * 当 FTS5 不可用时，自动降级为 LIKE 模糊查询。
 *
 * @param keyword 搜索关键词
 * @param projectId 项目 ID（0 表示不限制项目）
 * @param limit 最大返回结果数
 * @return QList<SearchResult> 搜索结果列表，按相关度降序排列
 *
 * SearchResult 结构体包含：
 * - report：匹配的报告对象
 * - highlight：带 <mark> 标签的高亮摘要
 * - score：BM25 相关度分数（越低越相关）
 *
 * @note FTS5 可用时使用 MATCH 语法，支持高级查询（AND/OR/NOT、短语搜索等）
 * @note 降级方案使用 LIKE，性能较差且不支持相关度排序
 * @note 关键词为空时直接返回空列表
 */
QList<SearchResult> ReportRepository::search(const QString& keyword,
                                               qint64 projectId,
                                               int limit)
{
    QList<SearchResult> results;

    // 关键词为空时直接返回空列表
    if (keyword.trimmed().isEmpty()) return results;

    QSqlDatabase db = BaseRepository::db();

    if (ftsAvailable()) {
        // ================================================================
        // 方案一：使用 FTS5 全文索引（高性能）
        // ================================================================
        QSqlQuery query(db);

        // 使用 FTS5 的 snippet() 函数生成高亮摘要，bm25() 函数计算相关度分数
        // snippet(reports_fts, 1, '<mark>', '</mark>', '...', 12) 参数说明：
        //   - 第1个参数：FTS 表名
        //   - 第2个参数：要生成摘要的列索引（1 = content 列）
        //   - 第3个参数：匹配开始标记
        //   - 第4个参数：匹配结束标记
        //   - 第5个参数：省略号文本
        //   - 第6个参数：摘要最大 token 数
        QString sql = R"(
            SELECT r.*,
                   snippet(reports_fts, 1, '<mark>', '</mark>', '...', 12) AS highlight,
                   bm25(reports_fts) AS score
            FROM reports_fts
            JOIN reports r ON r.id = reports_fts.rowid
            WHERE reports_fts MATCH :keyword
        )";

        // 可选：按项目 ID 筛选
        if (projectId > 0) {
            sql += " AND r.project_id = :projectId";
        }
        // 按相关度排序，限制结果数量
        sql += " ORDER BY score LIMIT :limit;";

        // 准备并执行查询
        query.prepare(sql);
        query.bindValue(":keyword", keyword);
        if (projectId > 0) {
            query.bindValue(":projectId", projectId);
        }
        query.bindValue(":limit", limit);

        // 遍历搜索结果
        if (query.exec()) {
            while (query.next()) {
                SearchResult result;
                result.report = mapToReport(query);                    // 映射报告对象
                result.highlight = query.value("highlight").toString(); // 高亮摘要
                result.score = query.value("score").toDouble();         // BM25 相关度分数
                results.append(result);
            }
        }
    } else {
        // ================================================================
        // 方案二：降级为 LIKE 模糊查询（FTS 不可用时）
        // ================================================================
        ReportQuery q;
        q.keyword = keyword;
        q.projectId = projectId;
        q.limit = limit;

        // 复用 findAll 方法进行 LIKE 查询
        const Report::List reports = findAll(q);
        for (const Report::Ptr& report : reports) {
            SearchResult result;
            result.report = report;
            // 降级方案没有高亮功能，直接使用标题作为摘要
            result.highlight = report->title();
            result.score = 0.0;  // LIKE 查询没有相关度分数
            results.append(result);
        }
    }

    return results;
}

// ===========================================================================
// 版本管理
// ===========================================================================

/**
 * @brief 保存报告版本快照
 *
 * 将当前报告内容保存为一个历史版本，可用于后续恢复。
 *
 * @param reportId 报告 ID
 * @param snapshotName 快照名称（为空时自动使用当前时间）
 * @return qint64 版本 ID，失败返回 -1
 *
 * 使用场景：
 * - 用户手动保存版本（"保存为版本"按钮）
 * - 自动保存前创建备份
 * - 恢复版本前自动备份当前内容
 *
 * @note 版本内容存储在 report_versions 表中
 * @note 快照名称为空时自动格式化为 "yyyy-MM-dd hh:mm:ss"
 * @note 每次保存都会创建新记录，不会覆盖历史版本
 */
qint64 ReportRepository::saveVersion(qint64 reportId, const QString& snapshotName)
{
    // 参数校验
    if (reportId <= 0) return -1;

    // 先加载报告，确保报告存在且能获取最新内容
    Report::Ptr report = findById(reportId);
    if (!report) return -1;

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 准备插入语句
    query.prepare(R"(
        INSERT INTO report_versions (report_id, content, snapshot_name, created_at)
        VALUES (:report_id, :content, :snapshot_name, :created_at);
    )");

    // 绑定参数值
    query.bindValue(":report_id", reportId);
    query.bindValue(":content", report->contentToJson());  // 保存当前内容快照
    // 快照名称为空时自动使用当前时间
    query.bindValue(":snapshot_name", snapshotName.isEmpty()
        ? QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
        : snapshotName);
    query.bindValue(":created_at", QDateTime::currentDateTime());

    // 执行插入
    if (!BaseRepository::execChecked(query, "saveVersion ")) return -1;

    // 返回新创建的版本 ID
    const qint64 newVersionId = query.lastInsertId().toLongLong();

    // 版本数限制：超过最大版本数时删除最旧的版本
    const int maxVersions = AppConfig::instance().maxVersions();
    if (maxVersions > 0) {
        QSqlQuery countQuery(db);
        countQuery.prepare("SELECT COUNT(*) FROM report_versions WHERE report_id = :report_id;");
        countQuery.bindValue(":report_id", reportId);
        if (countQuery.exec() && countQuery.next()) {
            const int total = countQuery.value(0).toInt();
            if (total > maxVersions) {
                // 删除最旧的 (total - maxVersions) 个版本
                QSqlQuery deleteQuery(db);
                deleteQuery.prepare(R"(
                    DELETE FROM report_versions
                    WHERE report_id = :report_id
                    AND id IN (
                        SELECT id FROM report_versions
                        WHERE report_id = :report_id
                        ORDER BY created_at ASC
                        LIMIT :limit
                    );
                )");
                deleteQuery.bindValue(":report_id", reportId);
                deleteQuery.bindValue(":limit", total - maxVersions);
                if (!deleteQuery.exec()) {
                    LOG_WARNING(QString("清理旧版本失败: %1").arg(deleteQuery.lastError().text()));
                }
            }
        }
    }

    return newVersionId;
}

/**
 * @brief 获取报告的所有版本列表
 *
 * @param reportId 报告 ID
 * @return QList<QPair<qint64, QString>> 版本列表，每项为 (版本ID, 显示名称)
 *
 * 显示名称格式为："快照名称 (创建时间)"
 * 例如："初稿 (2024-01-15 14:30)"
 *
 * 使用场景：
 * - 在版本历史对话框中显示版本列表
 * - 供用户选择要恢复或查看的版本
 *
 * @note 版本按创建时间降序排列（最新的在前）
 * @note 返回的是 QPair 列表，不是完整的版本对象（节省内存）
 */
QList<QPair<qint64, QString>> ReportRepository::getVersions(qint64 reportId)
{
    QList<QPair<qint64, QString>> versions;

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 查询版本列表，按创建时间降序排列
    query.prepare("SELECT id, snapshot_name, created_at FROM report_versions WHERE report_id = :reportId ORDER BY created_at DESC;");
    query.bindValue(":reportId", reportId);

    // 遍历结果
    if (query.exec()) {
        while (query.next()) {
            const qint64 id = query.value("id").toLongLong();
            // 格式化显示名称：快照名称 + 创建时间
            const QString name = QString("%1 (%2)")
                .arg(query.value("snapshot_name").toString())
                .arg(query.value("created_at").toDateTime().toString("yyyy-MM-dd hh:mm"));
            versions.append(qMakePair(id, name));
        }
    }

    return versions;
}

/**
 * @brief 获取指定版本的内容
 *
 * @param versionId 版本 ID
 * @return QString 版本内容的 JSON 字符串，失败返回空字符串
 *
 * 使用场景：
 * - 恢复版本时获取历史内容
 * - 查看版本差异时获取版本内容
 *
 * @note 返回的是 JSON 字符串，需要调用 Report::contentFromJson() 反序列化
 */
QString ReportRepository::getVersionContent(qint64 versionId)
{
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 查询版本内容
    query.prepare("SELECT content FROM report_versions WHERE id = :id;");
    query.bindValue(":id", versionId);

    // 执行查询并返回内容
    if (query.exec() && query.next()) {
        return query.value("content").toString();
    }
    return QString();
}

/**
 * @brief 恢复报告到指定版本
 *
 * 将报告内容恢复为指定历史版本的内容。
 * 恢复前会自动保存当前版本作为备份，防止误操作。
 *
 * @param reportId 报告 ID
 * @param versionId 要恢复的版本 ID
 * @return bool 恢复成功返回 true，失败返回 false
 *
 * 执行流程：
 * 1. 获取指定版本的内容
 * 2. 加载当前报告对象
 * 3. 自动保存当前版本作为备份（快照名称："恢复前自动备份"）
 * 4. 将版本内容反序列化到报告对象
 * 5. 调用 update() 保存到数据库
 *
 * @note 恢复操作会创建一个新的版本记录作为备份，不会删除任何历史版本
 * @note 如果版本内容为空或报告不存在，恢复失败
 * @warning 恢复操作会覆盖当前报告内容，但会自动创建备份版本
 */
bool ReportRepository::restoreVersion(qint64 reportId, qint64 versionId)
{
    // 获取指定版本的内容
    const QString content = getVersionContent(versionId);
    if (content.isEmpty()) return false;

    // 加载当前报告对象
    Report::Ptr report = findById(reportId);
    if (!report) return false;

    // 先保存当前版本作为备份，防止误操作导致数据丢失
    saveVersion(reportId, "恢复前自动备份");

    // 将历史版本内容反序列化到报告对象
    report->contentFromJson(content);

    // 保存到数据库
    return update(report);
}

/**
 * @brief 删除指定版本
 *
 * @param versionId 版本 ID
 * @return bool 删除成功返回 true，失败返回 false
 *
 * @warning 此操作不可恢复，删除后版本将永久丢失
 * @note 只删除版本记录，不影响当前报告内容
 */
bool ReportRepository::deleteVersion(qint64 versionId)
{
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 准备删除语句
    query.prepare("DELETE FROM report_versions WHERE id = :id;");
    query.bindValue(":id", versionId);

    // 执行删除并返回结果
    return query.exec();
}

// ===========================================================================
// 统计操作
// ===========================================================================

/**
 * @brief 统计报告总数
 *
 * @return int 报告总数（所有项目），查询失败时返回 0
 *
 * 使用场景：
 * - 系统状态统计
 * - 仪表盘显示
 */
int ReportRepository::count()
{
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 使用 COUNT(*) 聚合查询
    query.exec("SELECT COUNT(*) FROM reports;");

    // 读取第一行第一列的值
    return query.next() ? query.value(0).toInt() : 0;
}

/**
 * @brief 按项目统计报告数量
 *
 * @param projectId 项目 ID
 * @return int 该项目下的报告数量，查询失败时返回 0
 *
 * 使用场景：
 * - 在项目树中显示报告数量徽标
 * - 项目详情页统计
 */
int ReportRepository::countByProject(qint64 projectId)
{
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 按项目 ID 统计
    query.prepare("SELECT COUNT(*) FROM reports WHERE project_id = :projectId;");
    query.bindValue(":projectId", projectId);

    // 执行查询并返回计数值
    return query.exec() && query.next() ? query.value(0).toInt() : 0;
}

/**
 * @brief 按状态统计报告数量
 *
 * @param status 报告状态（Draft/Submitted/Reviewed）
 * @return int 该状态下的报告数量，查询失败时返回 0
 *
 * 使用场景：
 * - 仪表盘按状态统计
 * - 工作流程进度展示
 *
 * @note 使用临时 Report 对象调用 statusToString() 将枚举转换为字符串
 */
int ReportRepository::countByStatus(ReportStatus status)
{
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 按状态统计
    query.prepare("SELECT COUNT(*) FROM reports WHERE status = :status;");
    // 根据状态枚举直接转换为字符串（不创建临时 Report 对象）
    QString statusStr;
    switch (status) {
    case ReportStatus::Draft:     statusStr = AppConstants::REPORT_STATUS_DRAFT; break;
    case ReportStatus::Submitted: statusStr = AppConstants::REPORT_STATUS_SUBMITTED; break;
    case ReportStatus::Reviewed:  statusStr = AppConstants::REPORT_STATUS_REVIEWED; break;
    case ReportStatus::Approved:  statusStr = AppConstants::REPORT_STATUS_APPROVED; break;
    case ReportStatus::Archived:  statusStr = AppConstants::REPORT_STATUS_ARCHIVED; break;
    }
    query.bindValue(":status", statusStr);

    // 执行查询并返回计数值
    return query.exec() && query.next() ? query.value(0).toInt() : 0;
}
