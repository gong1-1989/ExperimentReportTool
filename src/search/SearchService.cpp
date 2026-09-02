/**
 * @file SearchService.cpp
 * @brief 搜索服务实现文件
 */

#include "SearchService.h"
#include "data/repositories/ProjectRepository.h"
#include "data/database/DatabaseManager.h"
#include "core/utils/Logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QSettings>
#include <QRegularExpression>

// ===========================================================================
// 构造与析构
// ===========================================================================

SearchService::SearchService(QObject* parent)
    : QObject(parent)
{
    // 从设置中加载搜索历史
    QSettings settings;
    m_searchHistory = settings.value("search/history").toStringList();
}

SearchService::~SearchService()
{
    // 保存搜索历史
    QSettings settings;
    settings.setValue("search/history", m_searchHistory);
}

// ===========================================================================
// 搜索
// ===========================================================================

QList<SearchResultItem> SearchService::search(const SearchQuery& query)
{
    if (query.keyword.trimmed().isEmpty()) {
        return QList<SearchResultItem>();
    }

    // 添加到搜索历史
    addToHistory(query.keyword);

    QList<SearchResultItem> results;

    // 优先使用 FTS5，降级为 LIKE
    if (isFtsAvailable()) {
        results = ftsSearch(query);
    } else {
        results = likeSearch(query);
    }

    // 补充项目名称等信息
    enrichResults(results);

    emit searchFinished(results);
    return results;
}

QList<SearchResultItem> SearchService::search(const QString& keyword,
                                                 qint64 projectId,
                                                 int maxResults)
{
    SearchQuery query;
    query.keyword = keyword;
    query.projectId = projectId;
    query.maxResults = maxResults;
    return search(query);
}

// ===========================================================================
// FTS5 全文搜索
// ===========================================================================

QList<SearchResultItem> SearchService::ftsSearch(const SearchQuery& query)
{
    QList<SearchResultItem> results;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery sqlQuery(db);

    // 构建 FTS 查询
    // 使用 snippet() 生成高亮摘要，bm25() 计算相关度
    QString sql = R"(
        SELECT r.*,
               snippet(reports_fts, 1, '<mark>', '</mark>', '...', 12) AS highlight,
               bm25(reports_fts) AS score
        FROM reports_fts
        JOIN reports r ON r.id = reports_fts.rowid
        WHERE reports_fts MATCH :keyword
    )";

    if (query.projectId > 0) {
        sql += " AND r.project_id = :projectId";
    }
    if (query.dateFrom.isValid()) {
        sql += " AND r.experiment_date >= :dateFrom";
    }
    if (query.dateTo.isValid()) {
        sql += " AND r.experiment_date <= :dateTo";
    }
    sql += " ORDER BY score LIMIT :limit;";

    sqlQuery.prepare(sql);
    sqlQuery.bindValue(":keyword", query.keyword);
    if (query.projectId > 0) {
        sqlQuery.bindValue(":projectId", query.projectId);
    }
    if (query.dateFrom.isValid()) {
        sqlQuery.bindValue(":dateFrom", query.dateFrom);
    }
    if (query.dateTo.isValid()) {
        sqlQuery.bindValue(":dateTo", query.dateTo);
    }
    sqlQuery.bindValue(":limit", query.maxResults);

    if (!sqlQuery.exec()) {
        LOG_ERROR(QString("FTS 搜索失败: %1").arg(sqlQuery.lastError().text()));
        // FTS 失败，降级为 LIKE
        return likeSearch(query);
    }

    while (sqlQuery.next()) {
        SearchResultItem item;
        item.report = Report::create();
        item.report->setId(sqlQuery.value("id").toLongLong());
        item.report->setProjectId(sqlQuery.value("project_id").toLongLong());
        item.report->setTitle(sqlQuery.value("title").toString());
        item.report->setAuthor(sqlQuery.value("author").toString());
        item.report->setExperimentDate(sqlQuery.value("experiment_date").toDate());
        item.report->setUpdatedAt(sqlQuery.value("updated_at").toDateTime());
        item.highlight = sqlQuery.value("highlight").toString();
        item.score = sqlQuery.value("score").toDouble();
        item.matchedAt = QDateTime::currentDateTime();
        results.append(item);
    }

    return results;
}

// ===========================================================================
// LIKE 模糊搜索（降级方案）
// ===========================================================================

QList<SearchResultItem> SearchService::likeSearch(const SearchQuery& query)
{
    QList<SearchResultItem> results;

    ReportQuery reportQuery;
    reportQuery.keyword = query.keyword;
    reportQuery.projectId = query.projectId;
    reportQuery.limit = query.maxResults;

    const Report::List reports = ReportRepository::findAll(reportQuery);

    for (const Report::Ptr& report : reports) {
        SearchResultItem item;
        item.report = report;
        item.highlight = generateSnippet(report->toPlainText(), query.keyword);
        item.score = 0.0;  // LIKE 搜索没有相关度分数
        item.matchedAt = QDateTime::currentDateTime();
        results.append(item);
    }

    return results;
}

// ===========================================================================
// 结果补充
// ===========================================================================

void SearchService::enrichResults(QList<SearchResultItem>& results)
{
    for (SearchResultItem& item : results) {
        if (item.report && item.report->projectId() > 0) {
            Project::Ptr project = ProjectRepository::findById(item.report->projectId());
            if (project) {
                item.projectName = project->name();
            }
        }
    }
}

// ===========================================================================
// 摘要生成
// ===========================================================================

QString SearchService::generateSnippet(const QString& content, const QString& keyword, int contextLength)
{
    if (content.isEmpty() || keyword.isEmpty()) {
        return QString();
    }

    // 查找关键词位置
    const int pos = content.indexOf(keyword, 0, Qt::CaseInsensitive);
    if (pos < 0) {
        // 没找到，返回前 contextLength 个字符
        return content.left(contextLength) + (content.length() > contextLength ? "..." : "");
    }

    // 计算上下文范围
    const int start = qMax(0, pos - contextLength / 2);
    const int end = qMin(content.length(), pos + keyword.length() + contextLength / 2);

    QString snippet = content.mid(start, end - start);

    // 高亮关键词
    const QString highlighted = QString("<mark>%1</mark>").arg(keyword);
    snippet.replace(keyword, highlighted, Qt::CaseInsensitive);

    // 添加省略号
    if (start > 0) snippet = "..." + snippet;
    if (end < content.length()) snippet = snippet + "...";

    return snippet;
}

// ===========================================================================
// 搜索历史
// ===========================================================================

void SearchService::addToHistory(const QString& keyword)
{
    const QString trimmed = keyword.trimmed();
    if (trimmed.isEmpty()) return;

    // 移除重复项
    m_searchHistory.removeAll(trimmed);
    // 添加到开头
    m_searchHistory.prepend(trimmed);
    // 限制数量
    while (m_searchHistory.size() > MAX_HISTORY) {
        m_searchHistory.removeLast();
    }

    emit historyChanged();
}

void SearchService::clearHistory()
{
    m_searchHistory.clear();
    QSettings settings;
    settings.remove("search/history");
    emit historyChanged();
}

// ===========================================================================
// FTS 状态与索引维护
// ===========================================================================

bool SearchService::isFtsAvailable() const
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='reports_fts';");
    return query.next();
}

bool SearchService::rebuildIndex()
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    // 清空 FTS 表
    if (!query.exec("DELETE FROM reports_fts;")) {
        LOG_ERROR(QString("清空 FTS 表失败: %1").arg(query.lastError().text()));
        return false;
    }

    // 重新插入所有报告
    query.prepare(R"(
        INSERT INTO reports_fts(rowid, title, content, tags)
        SELECT id, title, content, '' FROM reports;
    )");

    if (!query.exec()) {
        LOG_ERROR(QString("重建 FTS 索引失败: %1").arg(query.lastError().text()));
        return false;
    }

    LOG_INFO("FTS 全文索引已重建");
    return true;
}
