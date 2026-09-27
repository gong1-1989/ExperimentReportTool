/**
 * @file TagService.cpp
 * @brief 标签业务逻辑服务层实现
 */

#include "TagService.h"
#include "data/repositories/TagRepository.h"
#include "core/utils/AppTheme.h"

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
    return nullptr;
}

bool TagService::update(const Tag::Ptr& tag)
{
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
