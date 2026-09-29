/**
 * @file SearchService.cpp
 * @brief 搜索服务实现文件
 *
 * 搜索范围：报告标题、作者、标签、状态（不搜索报告内容）
 */

#include "SearchService.h"
#include "service/ReportService.h"
#include "data/repositories/ReportRepository.h"
#include "service/TagService.h"
#include "service/ProjectService.h"
#include "data/database/DatabaseManager.h"
#include "core/utils/Logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QSettings>

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

    QList<SearchResultItem> results;

    // 使用 LIKE 模糊搜索：标题、作者、标签、状态
    results = metadataSearch(query);

    // 全文搜索：报告内容（正文/表格/图表标题）
    contentSearch(query, results);

    // 补充项目名称等信息
    enrichResults(results);

    emit searchFinished(results);
    return results;
}

// ===========================================================================
// 全文搜索（报告内容 JSON）
// ===========================================================================

void SearchService::contentSearch(const SearchQuery& query,
                                   QList<SearchResultItem>& existing)
{
    // 收集已有报告 ID 用于去重
    QSet<qint64> existingIds;
    for (const SearchResultItem& item : existing) {
        if (item.report) existingIds.insert(item.report->id());
    }

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery sqlQuery(db);

    QString sql = "SELECT * FROM reports WHERE content LIKE :keyword";
    if (query.projectId > 0) {
        sql += " AND project_id = :projectId";
    }
    sql += " ORDER BY updated_at DESC LIMIT :limit;";

    sqlQuery.prepare(sql);
    sqlQuery.bindValue(":keyword", "%" + query.keyword + "%");
    if (query.projectId > 0) {
        sqlQuery.bindValue(":projectId", query.projectId);
    }
    sqlQuery.bindValue(":limit", query.maxResults);

    if (!sqlQuery.exec()) {
        LOG_ERROR(QString("全文搜索失败: %1").arg(sqlQuery.lastError().text()));
        return;
    }

    while (sqlQuery.next()) {
        // 直接复用当前查询行构造完整报告（SELECT * 已含 content JSON），省去每结果一次 getById
        const qint64 reportId = sqlQuery.value("id").toLongLong();
        if (existingIds.contains(reportId)) continue;  // 元数据已匹配，跳过

        Report::Ptr report = ReportRepository::fromQueryRow(sqlQuery);
        if (!report) continue;

        SearchResultItem item;
        item.report = report;
        item.score = 0.0;
        item.matchedAt = QDateTime::currentDateTime();
        item.highlight = tr("内容匹配: %1").arg(query.keyword);
        existing.append(item);
        existingIds.insert(reportId);
    }
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
// 元数据搜索（标题、作者、标签、状态）
// ===========================================================================

QList<SearchResultItem> SearchService::metadataSearch(const SearchQuery& query)
{
    QList<SearchResultItem> results;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery sqlQuery(db);

    // 搜索范围：
    //   r.title   - 报告标题
    //   r.author  - 作者
    //   t.name    - 标签名称（通过 report_tags 关联）
    //   r.status  - 状态（整数，转为字符串比较）
    //
    // 使用 DISTINCT 避免一个报告有多个标签匹配时重复出现
    QString sql = R"(
        SELECT DISTINCT r.*
        FROM reports r
        LEFT JOIN report_tags rt ON rt.report_id = r.id
        LEFT JOIN tags t ON t.id = rt.tag_id
        WHERE (r.title LIKE :keyword
            OR r.author LIKE :keyword
            OR t.name LIKE :keyword
            OR CAST(r.status AS TEXT) LIKE :keyword)
    )";

    if (query.projectId > 0) {
        sql += " AND r.project_id = :projectId";
    }
    sql += " ORDER BY r.updated_at DESC LIMIT :limit;";

    sqlQuery.prepare(sql);
    sqlQuery.bindValue(":keyword", "%" + query.keyword + "%");
    if (query.projectId > 0) {
        sqlQuery.bindValue(":projectId", query.projectId);
    }
    sqlQuery.bindValue(":limit", query.maxResults);

    if (!sqlQuery.exec()) {
        LOG_ERROR(QString("元数据搜索失败: %1").arg(sqlQuery.lastError().text()));
        return results;
    }

    while (sqlQuery.next()) {
        // 直接复用当前查询行构造完整报告（SELECT r.* 已含 content JSON），省去每结果一次 getById
        Report::Ptr report = ReportRepository::fromQueryRow(sqlQuery);
        if (!report) continue;

        SearchResultItem item;
        item.report = report;
        item.score = 0.0;
        item.matchedAt = QDateTime::currentDateTime();
        results.append(item);
    }

    // 批量加载本次结果的全部标签（一次 SQL），供匹配描述使用
    QList<qint64> reportIds;
    for (const SearchResultItem& item : results) reportIds.append(item.report->id());
    const QHash<qint64, Tag::List> tagsMap = TagService::findReportTagsBatch(reportIds);

    for (SearchResultItem& item : results) {
        item.highlight = buildMatchDescription(item.report, query.keyword, tagsMap);
    }

    return results;
}

// ===========================================================================
// 生成匹配字段描述
// ===========================================================================

QString SearchService::buildMatchDescription(const Report::Ptr& report,
                                              const QString& keyword,
                                              const QHash<qint64, Tag::List>& tagsMap)
{
    if (!report) return QString();

    QStringList matchedFields;
    const QString kw = keyword.toLower();

    // 检查标题
    if (report->title().toLower().contains(kw)) {
        matchedFields.append(tr("标题"));
    }

    // 检查作者
    if (report->author().toLower().contains(kw)) {
        matchedFields.append(tr("作者"));
    }

    // 检查状态（统一走模型方法，避免重复 switch）
    const QString statusStr = report->statusDisplayName();
    if (statusStr.toLower().contains(kw)) {
        matchedFields.append(tr("状态"));
    }

    // 检查标签（使用调用方批量预载的缓存，避免逐结果查询）
    const Tag::List tags = tagsMap.value(report->id());
    for (const Tag::Ptr& tag : tags) {
        if (tag->name().toLower().contains(kw)) {
            matchedFields.append(tr("标签"));
            break;
        }
    }

    if (matchedFields.isEmpty()) {
        return tr("匹配元数据");
    }

    return tr("匹配字段: %1").arg(matchedFields.join("、"));
}

// ===========================================================================
// 结果补充
// ===========================================================================

void SearchService::enrichResults(QList<SearchResultItem>& results)
{
    // 批量查询项目名（一次 SQL），避免逐结果 getById
    QList<qint64> projectIds;
    for (const SearchResultItem& item : results) {
        if (item.report && item.report->projectId() > 0) {
            projectIds.append(item.report->projectId());
        }
    }
    const QHash<qint64, QString> names = ProjectService::findNamesBatch(projectIds);

    for (SearchResultItem& item : results) {
        if (item.report && item.report->projectId() > 0) {
            item.projectName = names.value(item.report->projectId());
        }
    }
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
// 保留的 FTS 相关方法（不再使用，但保留接口兼容）
// ===========================================================================

bool SearchService::isFtsAvailable() const
{
    return false;
}

bool SearchService::rebuildIndex()
{
    return true;
}
