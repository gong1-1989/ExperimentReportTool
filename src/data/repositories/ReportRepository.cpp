/**
 * @file ReportRepository.cpp
 * @brief 报告仓储类实现文件
 */

#include "ReportRepository.h"
#include "data/database/DatabaseManager.h"
#include "core/utils/Logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDateTime>

// ===========================================================================
// 内部工具
// ===========================================================================

Report::Ptr ReportRepository::mapToReport(const QSqlQuery& query)
{
    Report::Ptr report = Report::create();

    report->setId(query.value("id").toLongLong());
    report->setProjectId(query.value("project_id").toLongLong());
    report->setTemplateId(query.value("template_id").toLongLong());
    report->setTitle(query.value("title").toString());
    report->setStatus(Report::statusFromString(query.value("status").toString()));
    report->setAuthor(query.value("author").toString());
    report->setExperimentDate(query.value("experiment_date").toDate());
    report->setCreatedAt(query.value("created_at").toDateTime());
    report->setUpdatedAt(query.value("updated_at").toDateTime());

    // 解析内容 JSON
    report->contentFromJson(query.value("content").toString());

    return report;
}

bool ReportRepository::ftsAvailable()
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='reports_fts';");
    return query.next();
}

// ===========================================================================
// CRUD
// ===========================================================================

Report::Ptr ReportRepository::findById(qint64 id)
{
    if (id <= 0) return nullptr;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    query.prepare("SELECT * FROM reports WHERE id = :id;");
    query.bindValue(":id", id);

    if (!query.exec()) {
        LOG_ERROR(QString("findById 失败: %1").arg(query.lastError().text()));
        return nullptr;
    }

    if (query.next()) {
        return mapToReport(query);
    }
    return nullptr;
}

Report::List ReportRepository::findAll(const ReportQuery& query)
{
    Report::List result;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery sqlQuery(db);

    QString sql = "SELECT * FROM reports WHERE 1=1";
    QVariantList bindValues;

    if (query.projectId > 0) {
        sql += " AND project_id = :projectId";
        bindValues << query.projectId;
    }

    if (query.templateId > 0) {
        sql += " AND template_id = :templateId";
        bindValues << query.templateId;
    }

    if (static_cast<int>(query.status) >= 0) {
        sql += " AND status = :status";
        Report tempReport;
        tempReport.setStatus(query.status);
        bindValues << tempReport.statusToString();
    }

    if (!query.author.isEmpty()) {
        sql += " AND author = :author";
        bindValues << query.author;
    }

    if (query.dateFrom.isValid()) {
        sql += " AND experiment_date >= :dateFrom";
        bindValues << query.dateFrom;
    }
    if (query.dateTo.isValid()) {
        sql += " AND experiment_date <= :dateTo";
        bindValues << query.dateTo;
    }

    // 关键词（非 FTS 的 LIKE 降级方案）
    if (!query.keyword.isEmpty() && !ftsAvailable()) {
        sql += " AND (title LIKE :keyword OR content LIKE :keyword)";
        bindValues << QString("%%1%").arg(query.keyword);
    }

    // 排序
    const QString validSortColumns[] = {"title", "created_at", "updated_at", "experiment_date", "id"};
    QString sortColumn = "updated_at";
    for (const QString& col : validSortColumns) {
        if (query.sortBy == col) {
            sortColumn = col;
            break;
        }
    }
    sql += QString(" ORDER BY %1 %2").arg(sortColumn)
               .arg(query.sortOrder == Qt::AscendingOrder ? "ASC" : "DESC");

    // 分页
    if (query.limit > 0) {
        sql += QString(" LIMIT %1 OFFSET %2").arg(query.limit).arg(query.offset);
    }
    sql += ";";

    sqlQuery.prepare(sql);
    for (int i = 0; i < bindValues.size(); ++i) {
        sqlQuery.bindValue(i, bindValues.at(i));
    }

    if (!sqlQuery.exec()) {
        LOG_ERROR(QString("findAll 失败: %1").arg(sqlQuery.lastError().text()));
        return result;
    }

    while (sqlQuery.next()) {
        result.append(mapToReport(sqlQuery));
    }

    return result;
}

Report::List ReportRepository::findByProject(qint64 projectId)
{
    ReportQuery query;
    query.projectId = projectId;
    query.sortBy = "updated_at";
    query.sortOrder = Qt::DescendingOrder;
    return findAll(query);
}

bool ReportRepository::insert(Report::Ptr report)
{
    if (!report) return false;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO reports (project_id, template_id, title, content, status, author, experiment_date, created_at, updated_at)
        VALUES (:project_id, :template_id, :title, :content, :status, :author, :experiment_date, :created_at, :updated_at);
    )");

    const QDateTime now = QDateTime::currentDateTime();
    query.bindValue(":project_id", report->projectId());
    query.bindValue(":template_id", report->templateId() > 0 ? report->templateId() : QVariant(QVariant::LongLong));
    query.bindValue(":title", report->title());
    query.bindValue(":content", report->contentToJson());
    query.bindValue(":status", report->statusToString());
    query.bindValue(":author", report->author());
    query.bindValue(":experiment_date", report->experimentDate());
    query.bindValue(":created_at", now);
    query.bindValue(":updated_at", now);

    if (!query.exec()) {
        LOG_ERROR(QString("insert 失败: %1").arg(query.lastError().text()));
        return false;
    }

    report->setId(query.lastInsertId().toLongLong());
    report->setCreatedAt(now);
    report->setUpdatedAt(now);

    LOG_INFO(QString("报告已创建: id=%1, title='%2'").arg(report->id()).arg(report->title()));
    return true;
}

bool ReportRepository::update(const Report::Ptr& report)
{
    if (!report || !report->isPersisted()) return false;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE reports
        SET project_id = :project_id,
            template_id = :template_id,
            title = :title,
            content = :content,
            status = :status,
            author = :author,
            experiment_date = :experiment_date,
            updated_at = :updated_at
        WHERE id = :id;
    )");

    query.bindValue(":project_id", report->projectId());
    query.bindValue(":template_id", report->templateId() > 0 ? report->templateId() : QVariant(QVariant::LongLong));
    query.bindValue(":title", report->title());
    query.bindValue(":content", report->contentToJson());
    query.bindValue(":status", report->statusToString());
    query.bindValue(":author", report->author());
    query.bindValue(":experiment_date", report->experimentDate());
    query.bindValue(":updated_at", QDateTime::currentDateTime());
    query.bindValue(":id", report->id());

    if (!query.exec()) {
        LOG_ERROR(QString("update 失败: %1").arg(query.lastError().text()));
        return false;
    }

    report->setUpdatedAt(QDateTime::currentDateTime());
    return true;
}

bool ReportRepository::remove(qint64 id)
{
    if (id <= 0) return false;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    DatabaseManager::instance().transaction();

    query.prepare("DELETE FROM reports WHERE id = :id;");
    query.bindValue(":id", id);

    if (!query.exec()) {
        LOG_ERROR(QString("remove 失败: %1").arg(query.lastError().text()));
        DatabaseManager::instance().rollback();
        return false;
    }

    DatabaseManager::instance().commit();
    LOG_INFO(QString("报告已删除: id=%1").arg(id));
    return true;
}

// ===========================================================================
// 全文检索
// ===========================================================================

QList<SearchResult> ReportRepository::search(const QString& keyword,
                                               qint64 projectId,
                                               int limit)
{
    QList<SearchResult> results;

    if (keyword.trimmed().isEmpty()) return results;

    QSqlDatabase db = DatabaseManager::instance().database();

    if (ftsAvailable()) {
        // 使用 FTS5 全文索引
        QSqlQuery query(db);

        // snippet() 函数生成高亮摘要，bm25() 计算相关度分数
        QString sql = R"(
            SELECT r.*,
                   snippet(reports_fts, 1, '<mark>', '</mark>', '...', 12) AS highlight,
                   bm25(reports_fts) AS score
            FROM reports_fts
            JOIN reports r ON r.id = reports_fts.rowid
            WHERE reports_fts MATCH :keyword
        )";

        if (projectId > 0) {
            sql += " AND r.project_id = :projectId";
        }
        sql += " ORDER BY score LIMIT :limit;";

        query.prepare(sql);
        query.bindValue(":keyword", keyword);
        if (projectId > 0) {
            query.bindValue(":projectId", projectId);
        }
        query.bindValue(":limit", limit);

        if (query.exec()) {
            while (query.next()) {
                SearchResult result;
                result.report = mapToReport(query);
                result.highlight = query.value("highlight").toString();
                result.score = query.value("score").toDouble();
                results.append(result);
            }
        }
    } else {
        // 降级方案：LIKE 模糊查询
        ReportQuery q;
        q.keyword = keyword;
        q.projectId = projectId;
        q.limit = limit;

        const Report::List reports = findAll(q);
        for (const Report::Ptr& report : reports) {
            SearchResult result;
            result.report = report;
            // 简单截取标题作为摘要
            result.highlight = report->title();
            result.score = 0.0;
            results.append(result);
        }
    }

    return results;
}

// ===========================================================================
// 版本管理
// ===========================================================================

qint64 ReportRepository::saveVersion(qint64 reportId, const QString& snapshotName)
{
    if (reportId <= 0) return -1;

    Report::Ptr report = findById(reportId);
    if (!report) return -1;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO report_versions (report_id, content, snapshot_name, created_at)
        VALUES (:report_id, :content, :snapshot_name, :created_at);
    )");

    query.bindValue(":report_id", reportId);
    query.bindValue(":content", report->contentToJson());
    query.bindValue(":snapshot_name", snapshotName.isEmpty()
        ? QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
        : snapshotName);
    query.bindValue(":created_at", QDateTime::currentDateTime());

    if (!query.exec()) {
        LOG_ERROR(QString("saveVersion 失败: %1").arg(query.lastError().text()));
        return -1;
    }

    return query.lastInsertId().toLongLong();
}

QList<QPair<qint64, QString>> ReportRepository::getVersions(qint64 reportId)
{
    QList<QPair<qint64, QString>> versions;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    query.prepare("SELECT id, snapshot_name, created_at FROM report_versions WHERE report_id = :reportId ORDER BY created_at DESC;");
    query.bindValue(":reportId", reportId);

    if (query.exec()) {
        while (query.next()) {
            const qint64 id = query.value("id").toLongLong();
            const QString name = QString("%1 (%2)")
                .arg(query.value("snapshot_name").toString())
                .arg(query.value("created_at").toDateTime().toString("yyyy-MM-dd hh:mm"));
            versions.append(qMakePair(id, name));
        }
    }

    return versions;
}

QString ReportRepository::getVersionContent(qint64 versionId)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    query.prepare("SELECT content FROM report_versions WHERE id = :id;");
    query.bindValue(":id", versionId);

    if (query.exec() && query.next()) {
        return query.value("content").toString();
    }
    return QString();
}

bool ReportRepository::restoreVersion(qint64 reportId, qint64 versionId)
{
    const QString content = getVersionContent(versionId);
    if (content.isEmpty()) return false;

    Report::Ptr report = findById(reportId);
    if (!report) return false;

    // 先保存当前版本作为备份
    saveVersion(reportId, "恢复前自动备份");

    report->contentFromJson(content);
    return update(report);
}

bool ReportRepository::deleteVersion(qint64 versionId)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    query.prepare("DELETE FROM report_versions WHERE id = :id;");
    query.bindValue(":id", versionId);

    return query.exec();
}

// ===========================================================================
// 统计
// ===========================================================================

int ReportRepository::count()
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.exec("SELECT COUNT(*) FROM reports;");
    return query.next() ? query.value(0).toInt() : 0;
}

int ReportRepository::countByProject(qint64 projectId)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM reports WHERE project_id = :projectId;");
    query.bindValue(":projectId", projectId);
    return query.exec() && query.next() ? query.value(0).toInt() : 0;
}

int ReportRepository::countByStatus(ReportStatus status)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM reports WHERE status = :status;");
    Report tempReport;
    tempReport.setStatus(status);
    query.bindValue(":status", tempReport.statusToString());
    return query.exec() && query.next() ? query.value(0).toInt() : 0;
}
