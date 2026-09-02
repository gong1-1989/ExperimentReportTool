/**
 * @file TagRepository.h
 * @brief 标签数据访问层头文件
 *
 * 提供标签的增删改查，以及报告与标签的关联管理。
 */

#ifndef TAG_REPOSITORY_H
#define TAG_REPOSITORY_H

#include <QList>
#include <QString>
#include "core/models/Tag.h"

/**
 * @brief 标签仓储类
 *
 * 封装标签相关的数据库操作。
 */
class TagRepository
{
public:
    // -----------------------------------------------------------------------
    // 标签 CRUD
    // -----------------------------------------------------------------------

    /// 根据 ID 查找标签
    static Tag::Ptr findById(qint64 tagId);

    /// 根据名称查找标签
    static Tag::Ptr findByName(const QString& name);

    /// 获取所有标签（按使用次数排序）
    static Tag::List findAll();

    /// 搜索标签（按名称模糊匹配）
    static Tag::List search(const QString& keyword);

    /// 保存标签（新建或更新）
    static bool save(Tag::Ptr tag);

    /// 删除标签
    static bool remove(qint64 tagId);

    /// 检查标签名称是否已存在
    static bool exists(const QString& name, qint64 excludeId = -1);

    // -----------------------------------------------------------------------
    // 报告-标签关联
    // -----------------------------------------------------------------------

    /// 获取报告的所有标签
    static Tag::List findByReport(qint64 reportId);

    /// 获取拥有某标签的所有报告 ID
    static QList<qint64> findReportIdsByTag(qint64 tagId);

    /// 为报告添加标签
    static bool addToReport(qint64 reportId, qint64 tagId);

    /// 为报告移除标签
    static bool removeFromReport(qint64 reportId, qint64 tagId);

    /// 设置报告的标签（替换现有标签）
    static bool setReportTags(qint64 reportId, const QList<qint64>& tagIds);

    /// 设置报告的标签（按名称，自动创建不存在的标签）
    static bool setReportTagsByName(qint64 reportId, const QStringList& tagNames);

    /// 获取报告的标签名称列表
    static QStringList findReportTagNames(qint64 reportId);

    // -----------------------------------------------------------------------
    // 统计
    // -----------------------------------------------------------------------

    /// 获取标签总数
    static int count();

    /// 获取标签使用次数
    static int usageCount(qint64 tagId);

    /// 更新标签使用次数统计
    static void updateUsageCount(qint64 tagId);

private:
    /// 从 SQL 查询结果创建 Tag 对象
    static Tag::Ptr createFromQuery(const class QSqlQuery& query);
};

#endif // TAG_REPOSITORY_H
