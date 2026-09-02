/**
 * @file SearchService.h
 * @brief 搜索服务头文件
 *
 * 封装报告全文检索功能，提供统一的搜索接口。
 * 支持关键词搜索、项目范围筛选、结果高亮、搜索历史。
 */

#ifndef SEARCH_SERVICE_H
#define SEARCH_SERVICE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QDateTime>

#include "core/models/Report.h"
#include "data/repositories/ReportRepository.h"

/**
 * @brief 搜索结果项
 */
struct SearchResultItem {
    Report::Ptr report;       ///< 匹配的报告
    QString highlight;         ///< 高亮摘要
    double score;              ///< 匹配分数
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
    bool searchTitle;          ///< 是否搜索标题
    bool searchContent;        ///< 是否搜索正文
    bool searchTags;           ///< 是否搜索标签
    QDate dateFrom;            ///< 日期范围起始
    QDate dateTo;              ///< 日期范围结束

    SearchQuery()
        : projectId(-1)
        , maxResults(50)
        , searchTitle(true)
        , searchContent(true)
        , searchTags(true)
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
     * @brief 执行搜索
     * @param query 搜索条件
     * @return 搜索结果列表（按相关度排序）
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
     * @brief 检查 FTS5 全文索引是否可用
     * @return 可用返回 true
     */
    bool isFtsAvailable() const;

    /**
     * @brief 重建全文索引（当 FTS 表损坏或数据不一致时使用）
     * @return 成功返回 true
     */
    bool rebuildIndex();

signals:
    /// 搜索完成信号
    void searchFinished(const QList<SearchResultItem>& results);

    /// 搜索历史变化
    void historyChanged();

private:
    /**
     * @brief 使用 FTS5 全文索引搜索
     */
    QList<SearchResultItem> ftsSearch(const SearchQuery& query);

    /**
     * @brief 使用 LIKE 模糊搜索（降级方案）
     */
    QList<SearchResultItem> likeSearch(const SearchQuery& query);

    /**
     * @brief 为搜索结果补充项目名称等信息
     */
    void enrichResults(QList<SearchResultItem>& results);

    /**
     * @brief 生成搜索摘要（高亮匹配部分）
     * @param content 报告内容
     * @param keyword 关键词
     * @param contextLength 上下文长度
     * @return 高亮摘要
     */
    QString generateSnippet(const QString& content, const QString& keyword, int contextLength = 80);

    // -----------------------------------------------------------------------
    // 成员变量
    // -----------------------------------------------------------------------

    QStringList m_searchHistory;  ///< 搜索历史
    static const int MAX_HISTORY = 20;  ///< 最大历史记录数
};

#endif // SEARCH_SERVICE_H
