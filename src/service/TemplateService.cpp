/**
 * @file TemplateService.cpp
 * @brief 模板业务逻辑服务层实现
 */

#include "TemplateService.h"
#include "core/utils/Logger.h"
#include "data/repositories/TemplateRepository.h"
#include <QtGlobal>

Template::Ptr TemplateService::getById(qint64 id)
{
    return TemplateRepository::findById(id);
}

Template::List TemplateService::listAll()
{
    return TemplateRepository::findAll();
}

Template::Ptr TemplateService::create(const QString& name, const QString& category)
{
    if (name.isEmpty()) return nullptr;

    Template::Ptr temp = Template::create();
    temp->setName(name);
    temp->setCategory(category);

    if (TemplateRepository::insert(temp)) {
        return temp;
    }
    LOG_ERROR(QString("创建模板失败: %1").arg(name));
    return nullptr;
}

bool TemplateService::save(const Template::Ptr& temp)
{
    Q_ASSERT(temp);
    if (!temp) return false;
    if (temp->id() > 0) {
        return TemplateRepository::update(temp);
    }
    return TemplateRepository::insert(temp);
}

bool TemplateService::update(const Template::Ptr& temp)
{
    if (!temp) return false;
    return TemplateRepository::update(temp);
}

bool TemplateService::remove(qint64 id)
{
    return TemplateRepository::remove(id);
}

bool TemplateService::exists(const QString& name)
{
    if (name.isEmpty()) return false;
    const Template::List all = TemplateRepository::findAll();
    for (const Template::Ptr& t : all) {
        if (t && t->name() == name) {
            return true;
        }
    }
    return false;
}
