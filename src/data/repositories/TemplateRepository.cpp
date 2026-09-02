/**
 * @file TemplateRepository.cpp
 * @brief 模板仓储类实现文件
 */

#include "TemplateRepository.h"
#include "data/database/DatabaseManager.h"
#include "core/utils/Logger.h"

#include <QSqlQuery>
#include <QSqlError>

Template::Ptr TemplateRepository::mapToTemplate(const QSqlQuery& query)
{
    Template::Ptr temp = Template::create();
    temp->setId(query.value("id").toLongLong());
    temp->setName(query.value("name").toString());
    temp->setCategory(query.value("category").toString());
    temp->setDescription(query.value("description").toString());
    temp->setBuiltin(query.value("is_builtin").toBool());
    temp->setCreatedAt(query.value("created_at").toDateTime());
    temp->setUpdatedAt(query.value("updated_at").toDateTime());
    temp->structureFromJson(query.value("structure").toString());
    return temp;
}

Template::Ptr TemplateRepository::findById(qint64 id)
{
    if (id <= 0) return nullptr;
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("SELECT * FROM templates WHERE id = :id;");
    query.bindValue(":id", id);
    if (query.exec() && query.next()) {
        return mapToTemplate(query);
    }
    return nullptr;
}

Template::List TemplateRepository::findAll()
{
    Template::List result;
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.exec("SELECT * FROM templates ORDER BY is_builtin DESC, category, name;");
    while (query.next()) {
        result.append(mapToTemplate(query));
    }
    return result;
}

Template::List TemplateRepository::findByCategory(const QString& category)
{
    Template::List result;
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("SELECT * FROM templates WHERE category = :category ORDER BY name;");
    query.bindValue(":category", category);
    if (query.exec()) {
        while (query.next()) {
            result.append(mapToTemplate(query));
        }
    }
    return result;
}

Template::List TemplateRepository::findBuiltin()
{
    Template::List result;
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.exec("SELECT * FROM templates WHERE is_builtin = 1 ORDER BY category, name;");
    while (query.next()) {
        result.append(mapToTemplate(query));
    }
    return result;
}

Template::List TemplateRepository::findCustom()
{
    Template::List result;
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.exec("SELECT * FROM templates WHERE is_builtin = 0 ORDER BY category, name;");
    while (query.next()) {
        result.append(mapToTemplate(query));
    }
    return result;
}

QStringList TemplateRepository::allCategories()
{
    QStringList categories;
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.exec("SELECT DISTINCT category FROM templates ORDER BY category;");
    while (query.next()) {
        categories.append(query.value(0).toString());
    }
    return categories;
}

bool TemplateRepository::insert(Template::Ptr temp)
{
    if (!temp) return false;
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO templates (name, category, description, structure, is_builtin, created_at, updated_at)
        VALUES (:name, :category, :description, :structure, :is_builtin, :created_at, :updated_at);
    )");

    const QDateTime now = QDateTime::currentDateTime();
    query.bindValue(":name", temp->name());
    query.bindValue(":category", temp->category());
    query.bindValue(":description", temp->description());
    query.bindValue(":structure", temp->structureToJson());
    query.bindValue(":is_builtin", temp->isBuiltin() ? 1 : 0);
    query.bindValue(":created_at", now);
    query.bindValue(":updated_at", now);

    if (!query.exec()) {
        LOG_ERROR(QString("insert 失败: %1").arg(query.lastError().text()));
        return false;
    }

    temp->setId(query.lastInsertId().toLongLong());
    temp->setCreatedAt(now);
    temp->setUpdatedAt(now);
    return true;
}

bool TemplateRepository::update(const Template::Ptr& temp)
{
    if (!temp || !temp->isPersisted()) return false;
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE templates
        SET name = :name, category = :category, description = :description,
            structure = :structure, updated_at = :updated_at
        WHERE id = :id;
    )");

    query.bindValue(":name", temp->name());
    query.bindValue(":category", temp->category());
    query.bindValue(":description", temp->description());
    query.bindValue(":structure", temp->structureToJson());
    query.bindValue(":updated_at", QDateTime::currentDateTime());
    query.bindValue(":id", temp->id());

    if (!query.exec()) {
        LOG_ERROR(QString("update 失败: %1").arg(query.lastError().text()));
        return false;
    }
    temp->setUpdatedAt(QDateTime::currentDateTime());
    return true;
}

bool TemplateRepository::remove(qint64 id)
{
    if (id <= 0) return false;

    // 内置模板不可删除
    Template::Ptr temp = findById(id);
    if (temp && temp->isBuiltin()) {
        LOG_WARNING("内置模板不可删除");
        return false;
    }

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("DELETE FROM templates WHERE id = :id;");
    query.bindValue(":id", id);
    return query.exec();
}

int TemplateRepository::count()
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.exec("SELECT COUNT(*) FROM templates;");
    return query.next() ? query.value(0).toInt() : 0;
}
