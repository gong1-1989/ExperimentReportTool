/**
 * @file ReportRepository.h
 * @brief 报告仓储类头文件
 *
 * 负责 reports 表的所有数据操作，包括 CRUD、全文检索、版本管理等。
 */

#ifndef REPORT_REPOSITORY_H
#define REPORT_REPOSITORY_H

#include <QList>
#include <QString>
#include <QDate>
#include <QSqlQuery>

#include "core/models/Report.h"

/**
 * @brief 报告查询条件结构体
 */
struct ReportQuery {
    QString keyword;        ///< 全文检索关键词
    qint64 projectId;       ///< 所属项目 ID（-1 表示所有）
    qint64 templateId;      ///< 模板 ID（-1 表示所有）
    ReportStatus status;    ///< 状态（-1 表示所有）
    QString author;         ///< 作者
    QDate dateFrom;         ///< 实验日期起始
    QDate dateTo;           ///< 实验日期截止
    QString sortBy;         ///< 排序字段
    Qt::SortOrder sortOrder; ///< 排序方向
    int limit;              ///< 最大返回数量（0 表示不限）
    int offset;             ///< 偏移量（分页用）

    ReportQuery()
        : projectId(-1)
        , templateId(-1)
        , status(static_cast<ReportStatus>(-1))
        , sortBy("updated_at")
        , sortOrder(Qt::DescendingOrder)
        , limit(0)
        , offset(0)
    {}
};

/**
 * @brief 搜索结果结构体
 */
struct SearchResult {
    Report::Ptr report;     ///< 匹配的报告
    QString highlight;      ///< 高亮摘要（匹配上下文）
    double score;           ///< 匹配分数

    SearchResult() : score(0.0) {}
};

/**
 * @brief 报告仓储类
 */
class ReportRepository
{
public:
    // -----------------------------------------------------------------------
    // CRUD
    // -----------------------------------------------------------------------

    static Report::Ptr findById(qint64 id);
    static Report::List findAll(const ReportQuery& query = ReportQuery());
    static Report::List findByProject(qint64 projectId);
    static bool insert(Report::Ptr report);
    static bool update(const Report::Ptr& report);
    static bool remove(qint64 id);

    // -----------------------------------------------------------------------
    // 全文检索
    // -----------------------------------------------------------------------

    /**
     * @brief 全文搜索报告
     * @param keyword 搜索关键词
     * @param projectId 限定项目（-1 表示所有）
     * @param limit 最大结果数
     * @return 搜索结果列表（按相关度排序）
     *
     * 优先使用 FTS5 全文索引，如果不可用则降级为 LIKE 模糊查询。
     */
    static QList<SearchResult> search(const QString& keyword,
                                       qint64 projectId = -1,
                                       int limit = 50);

    // -----------------------------------------------------------------------
    // 版本管理
    // -----------------------------------------------------------------------

    /**
     * @brief 保存报告版本快照
     * @param reportId 报告 ID
     * @param snapshotName 版本名称/备注
     * @return 版本 ID，失败返回 -1
     */
    static qint64 saveVersion(qint64 reportId, const QString& snapshotName = "");

    /**
     * @brief 获取报告的所有版本
     * @param reportId 报告 ID
     * @return 版本列表（按时间倒序）
     */
    static QList<QPair<qint64, QString>> getVersions(qint64 reportId);

    /**
     * @brief 获取指定版本的内容
     * @param versionId 版本 ID
     * @return 内容 JSON 字符串
     */
    static QString getVersionContent(qint64 versionId);

    /**
     * @brief 恢复报告到指定版本
     * @param reportId 报告 ID
     * @param versionId 版本 ID
     * @return 成功返回 true
     */
    static bool restoreVersion(qint64 reportId, qint64 versionId);

    /**
     * @brief 删除版本
     * @param versionId 版本 ID
     * @return 成功返回 true
     */
    static bool deleteVersion(qint64 versionId);

    // -----------------------------------------------------------------------
    // 统计
    // -----------------------------------------------------------------------

    static int count();
    static int countByProject(qint64 projectId);
    static int countByStatus(ReportStatus status);

private:
    static Report::Ptr mapToReport(const QSqlQuery& query);
    static bool ftsAvailable();
};

#endif // REPORT_REPOSITORY_H
