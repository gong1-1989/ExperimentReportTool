/**
 * @file TemplateService.h
 * @brief 模板业务逻辑服务层
 */

#ifndef TEMPLATE_SERVICE_H
#define TEMPLATE_SERVICE_H

#include "core/models/Template.h"

class TemplateService
{
public:
    static Template::Ptr getById(qint64 id);
    static Template::List listAll();
    static Template::Ptr create(const QString& name, const QString& category = QString());
    static bool save(const Template::Ptr& temp);
    static bool update(const Template::Ptr& temp);
    static bool remove(qint64 id);
    static bool exists(const QString& name);

private:
    TemplateService() = delete;
    ~TemplateService() = delete;
};

#endif // TEMPLATE_SERVICE_H
