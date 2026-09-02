/**
 * @file TemplateRepository.h
 * @brief 模板仓储类头文件
 */

#ifndef TEMPLATE_REPOSITORY_H
#define TEMPLATE_REPOSITORY_H

#include "core/models/Template.h"
#include <QSqlQuery>

class TemplateRepository
{
public:
    static Template::Ptr findById(qint64 id);
    static Template::List findAll();
    static Template::List findByCategory(const QString& category);
    static Template::List findBuiltin();
    static Template::List findCustom();
    static QStringList allCategories();

    static bool insert(Template::Ptr temp);
    static bool update(const Template::Ptr& temp);
    static bool remove(qint64 id);

    static int count();

private:
    static Template::Ptr mapToTemplate(const QSqlQuery& query);
};

#endif // TEMPLATE_REPOSITORY_H
