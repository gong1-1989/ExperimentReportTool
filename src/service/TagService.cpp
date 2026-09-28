/**
 * @file TagService.cpp
 * @brief 标签业务逻辑服务层实现
 */

#include "TagService.h"
#include "data/repositories/TagRepository.h"
#include "core/utils/AppTheme.h"
#include "core/utils/Logger.h"
#include <QtGlobal>

Tag::Ptr TagService::getById(qint64 id)
{
    return TagRepository::findById(id);
}

Tag::List TagService::listAll()
{
    return TagRepository::findAll();
}

Tag::Ptr TagService::create(const QString& name, const QString& color)
{
    if (name.isEmpty()) return nullptr;
    if (exists(name)) return nullptr;

    Tag::Ptr tag = Tag::create();
    tag->setName(name);
    tag->setColor(color.isEmpty() ? AppTheme::TagColors::Presets[0] : color);

    if (TagRepository::save(tag)) {
        return tag;
    }
    LOG_ERROR(QString("创建标签失败: %1").arg(name));
    return nullptr;
}

bool TagService::update(const Tag::Ptr& tag)
{
    Q_ASSERT(tag);
    if (!tag) return false;
    return TagRepository::save(tag);
}

bool TagService::remove(qint64 id)
{
    return TagRepository::remove(id);
}

bool TagService::exists(const QString& name)
{
    return TagRepository::exists(name);
}

bool TagService::exists(const QString& name, qint64 excludeId)
{
    return TagRepository::exists(name, excludeId);
}

Tag::List TagService::search(const QString& keyword)
{
    return TagRepository::search(keyword);
}

Tag::List TagService::findByReport(qint64 reportId)
{
    return TagRepository::findByReport(reportId);
}

bool TagService::setReportTags(qint64 reportId, const QList<qint64>& tagIds)
{
    return TagRepository::setReportTags(reportId, tagIds);
}
