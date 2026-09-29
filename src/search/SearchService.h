/**
 * @file SearchService.h
 * @brief 搜索服务头文件
 *
 * 封装报告元数据检索功能，搜索范围：标题、作者、标签、状态（不搜索报告内容）。
 * 支持关键词搜索、项目范围筛选、搜索历史。
 */

#ifndef SEARCH_SERVICE_H
#define SEARCH_SERVICE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QDateTime>

#include "core/models/Report.h"
#include "core/models/Tag.h"
#include "data/repositories/ReportRepository.h"

/**
 * @brief 搜索结果项
 */
struct SearchResultItem {
    Report::Ptr report;       ///< 匹配的报告
    QString highlight;         ///< 匹配字段描述（如"匹配字段: 标题、标签"）
    double score;              ///< 匹配分数（保留字段，当前未使用）
    QString projectName;       ///< 所属项目名称
    QDateTime matchedAt;       ///< 匹配时间

    SearchResultItem() : score(0.0) {}
};

/**
 * @brief 搜索查询条件
 */
struct SearchQuery {
    QString keyword;           ///< 搜索关键词
    qint64 projectId;          ///< 限定项目（-1 表示所有）
    int maxResults;            ///< 最大结果数
    QDate dateFrom;            ///< 日期范围起始（保留字段）
    QDate dateTo;              ///< 日期范围结束（保留字段）

    SearchQuery()
        : projectId(-1)
        , maxResults(50)
    {}
};

/**
 * @brief 搜索服务
 *
 * 使用方式：
 * @code
 *   SearchService service;
 *   SearchQuery query;
 *   query.keyword = "牛顿";
 *   QList<SearchResultItem> results = service.search(query);
 * @endcode
 */
class SearchService : public QObject
{
    Q_OBJECT

public:
    explicit SearchService(QObject* parent = nullptr);
    ~SearchService() override;

    /**
     * @brief 执行搜索（搜索范围：标题、作者、标签、状态）
     * @param query 搜索条件
     * @return 搜索结果列表（按更新时间倒序）
     */
    QList<SearchResultItem> search(const SearchQuery& query);

    /**
     * @brief 便捷搜索方法
     * @param keyword 关键词
     * @param projectId 限定项目（-1 表示所有）
     * @param maxResults 最大结果数
     * @return 搜索结果列表
     */
    QList<SearchResultItem> search(const QString& keyword,
                                     qint64 projectId = -1,
                                     int maxResults = 50);

    /**
     * @brief 获取搜索历史
     * @return 搜索关键词列表（最近的在前）
     */
    QStringList searchHistory() const { return m_searchHistory; }

    /**
     * @brief 添加搜索历史
     * @param keyword 关键词
     */
    void addToHistory(const QString& keyword);

    /**
     * @brief 清空搜索历史
     */
    void clearHistory();

    /**
     * @brief 检查全文索引是否可用（已禁用，始终返回 false）
     */
    bool isFtsAvailable() const;

    /**
     * @brief 重建全文索引（已禁用，仅保留接口兼容）
     */
    bool rebuildIndex();

signals:
    /// 搜索完成信号
    void searchFinished(const QList<SearchResultItem>& results);

    /// 搜索历史变化
    void historyChanged();

private:
    /**
     * @brief 元数据搜索（标题、作者、标签、状态）
     */
    QList<SearchResultItem> metadataSearch(const SearchQuery& query);
    void contentSearch(const SearchQuery& query, QList<SearchResultItem>& existing);

    /**
     * @brief 生成匹配字段描述
     * @param report 报告
     * @param keyword 关键词
     * @return 匹配字段描述，如"匹配字段: 标题、标签"
     */
    QString buildMatchDescription(const Report::Ptr& report, const QString& keyword,
                                  const QHash<qint64, Tag::List>& tagsMap);

    /**
     * @brief 为搜索结果补充项目名称等信息
     */
    void enrichResults(QList<SearchResultItem>& results);

    // -----------------------------------------------------------------------
    // 成员变量
    // -----------------------------------------------------------------------

    QStringList m_searchHistory;  ///< 搜索历史
    static const int MAX_HISTORY = 20;  ///< 最大历史记录数
};

#endif // SEARCH_SERVICE_H
