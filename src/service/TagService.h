/**
 * @file TagService.h
 * @brief 标签业务逻辑服务层
 */

#ifndef TAG_SERVICE_H
#define TAG_SERVICE_H

#include "core/models/Tag.h"

class TagService
{
public:
    static Tag::Ptr getById(qint64 id);
    static Tag::List listAll();
    static Tag::Ptr create(const QString& name, const QString& color = QString());
    static bool update(const Tag::Ptr& tag);
    static bool remove(qint64 id);
    static bool exists(const QString& name);
    static bool exists(const QString& name, qint64 excludeId);
    static Tag::List search(const QString& keyword);
    static Tag::List findByReport(qint64 reportId);
    /// 批量查询多个报告 id → 标签名列表（一次 SQL，列表/表格渲染用）
    static QHash<qint64, QStringList> findReportTagNamesBatch(const QList<qint64>& reportIds);
    /// 批量查询多个报告 id → 完整标签对象列表（含颜色，搜索结果/列表渲染用）
    static QHash<qint64, Tag::List> findReportTagsBatch(const QList<qint64>& reportIds);
    static bool setReportTags(qint64 reportId, const QList<qint64>& tagIds);

private:
    TagService() = delete;
    ~TagService() = delete;
};

#endif // TAG_SERVICE_H
