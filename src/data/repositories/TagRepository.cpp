/**
 * @file TagRepository.cpp
 * @brief 标签数据访问层实现文件
 */

#include "TagRepository.h"
#include "data/database/DatabaseManager.h"
#include "core/utils/Logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

// ===========================================================================
// 标签 CRUD
// ===========================================================================

Tag::Ptr TagRepository::findById(qint64 tagId)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("SELECT * FROM tags WHERE id = :id;");
    query.bindValue(":id", tagId);

    if (!query.exec() || !query.next()) {
        return Tag::Ptr();
    }

    return createFromQuery(query);
}

Tag::Ptr TagRepository::findByName(const QString& name)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("SELECT * FROM tags WHERE name = :name;");
    query.bindValue(":name", name);

    if (!query.exec() || !query.next()) {
        return Tag::Ptr();
    }

    return createFromQuery(query);
}

Tag::List TagRepository::findAll()
{
    Tag::List tags;
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    // 查询标签及其使用次数
    query.exec(R"(
        SELECT t.*, COUNT(rt.report_id) as usage_count
        FROM tags t
        LEFT JOIN report_tags rt ON rt.tag_id = t.id
        GROUP BY t.id
        ORDER BY usage_count DESC, t.name ASC;
    )");

    while (query.next()) {
        Tag::Ptr tag = createFromQuery(query);
        tag->setUsageCount(query.value("usage_count").toInt());
        tags.append(tag);
    }

    return tags;
}

Tag::List TagRepository::search(const QString& keyword)
{
    Tag::List tags;
    if (keyword.isEmpty()) return findAll();

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare(R"(
        SELECT t.*, COUNT(rt.report_id) as usage_count
        FROM tags t
        LEFT JOIN report_tags rt ON rt.tag_id = t.id
        WHERE t.name LIKE :keyword
        GROUP BY t.id
        ORDER BY usage_count DESC, t.name ASC;
    )");
    query.bindValue(":keyword", "%" + keyword + "%");

    if (query.exec()) {
        while (query.next()) {
            Tag::Ptr tag = createFromQuery(query);
            tag->setUsageCount(query.value("usage_count").toInt());
            tags.append(tag);
        }
    }

    return tags;
}

bool TagRepository::save(Tag::Ptr tag)
{
    if (!tag) return false;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    if (tag->isNew()) {
        // 新建
        query.prepare(R"(
            INSERT INTO tags (name, color, description, created_at)
            VALUES (:name, :color, :description, :created_at);
        )");
        query.bindValue(":name", tag->name());
        query.bindValue(":color", tag->color());
        query.bindValue(":description", tag->description());
        query.bindValue(":created_at", tag->createdAt());

        if (!query.exec()) {
            LOG_ERROR(QString("创建标签失败: %1").arg(query.lastError().text()));
            return false;
        }

        tag->setId(query.lastInsertId().toLongLong());
    } else {
        // 更新
        query.prepare(R"(
            UPDATE tags SET name = :name, color = :color,
                   description = :description WHERE id = :id;
        )");
        query.bindValue(":name", tag->name());
        query.bindValue(":color", tag->color());
        query.bindValue(":description", tag->description());
        query.bindValue(":id", tag->id());

        if (!query.exec()) {
            LOG_ERROR(QString("更新标签失败: %1").arg(query.lastError().text()));
            return false;
        }
    }

    return true;
}

bool TagRepository::remove(qint64 tagId)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    // 先删除报告-标签关联
    query.prepare("DELETE FROM report_tags WHERE tag_id = :tagId;");
    query.bindValue(":tagId", tagId);
    query.exec();

    // 再删除标签
    query.prepare("DELETE FROM tags WHERE id = :id;");
    query.bindValue(":id", tagId);

    if (!query.exec()) {
        LOG_ERROR(QString("删除标签失败: %1").arg(query.lastError().text()));
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool TagRepository::exists(const QString& name, qint64 excludeId)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    if (excludeId > 0) {
        query.prepare("SELECT COUNT(*) FROM tags WHERE name = :name AND id != :id;");
        query.bindValue(":id", excludeId);
    } else {
        query.prepare("SELECT COUNT(*) FROM tags WHERE name = :name;");
    }
    query.bindValue(":name", name);

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

// ===========================================================================
// 报告-标签关联
// ===========================================================================

Tag::List TagRepository::findByReport(qint64 reportId)
{
    Tag::List tags;
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare(R"(
        SELECT t.* FROM tags t
        JOIN report_tags rt ON rt.tag_id = t.id
        WHERE rt.report_id = :reportId
        ORDER BY t.name ASC;
    )");
    query.bindValue(":reportId", reportId);

    if (query.exec()) {
        while (query.next()) {
            tags.append(createFromQuery(query));
        }
    }

    return tags;
}

QList<qint64> TagRepository::findReportIdsByTag(qint64 tagId)
{
    QList<qint64> reportIds;
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("SELECT report_id FROM report_tags WHERE tag_id = :tagId;");
    query.bindValue(":tagId", tagId);

    if (query.exec()) {
        while (query.next()) {
            reportIds.append(query.value("report_id").toLongLong());
        }
    }

    return reportIds;
}

bool TagRepository::addToReport(qint64 reportId, qint64 tagId)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    // 检查是否已存在
    query.prepare("SELECT COUNT(*) FROM report_tags WHERE report_id = :reportId AND tag_id = :tagId;");
    query.bindValue(":reportId", reportId);
    query.bindValue(":tagId", tagId);
    if (query.exec() && query.next() && query.value(0).toInt() > 0) {
        return true;  // 已存在
    }

    query.prepare("INSERT INTO report_tags (report_id, tag_id) VALUES (:reportId, :tagId);");
    query.bindValue(":reportId", reportId);
    query.bindValue(":tagId", tagId);

    if (!query.exec()) {
        LOG_ERROR(QString("添加报告标签失败: %1").arg(query.lastError().text()));
        return false;
    }

    updateUsageCount(tagId);
    return true;
}

bool TagRepository::removeFromReport(qint64 reportId, qint64 tagId)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("DELETE FROM report_tags WHERE report_id = :reportId AND tag_id = :tagId;");
    query.bindValue(":reportId", reportId);
    query.bindValue(":tagId", tagId);

    if (!query.exec()) {
        LOG_ERROR(QString("移除报告标签失败: %1").arg(query.lastError().text()));
        return false;
    }

    updateUsageCount(tagId);
    return true;
}

bool TagRepository::setReportTags(qint64 reportId, const QList<qint64>& tagIds)
{
    QSqlDatabase db = DatabaseManager::instance().database();

    // 开启事务
    db.transaction();

    // 清除现有标签
    QSqlQuery query(db);
    query.prepare("DELETE FROM report_tags WHERE report_id = :reportId;");
    query.bindValue(":reportId", reportId);
    if (!query.exec()) {
        db.rollback();
        return false;
    }

    // 添加新标签
    for (qint64 tagId : tagIds) {
        query.prepare("INSERT INTO report_tags (report_id, tag_id) VALUES (:reportId, :tagId);");
        query.bindValue(":reportId", reportId);
        query.bindValue(":tagId", tagId);
        if (!query.exec()) {
            db.rollback();
            return false;
        }
        updateUsageCount(tagId);
    }

    db.commit();
    return true;
}

bool TagRepository::setReportTagsByName(qint64 reportId, const QStringList& tagNames)
{
    QList<qint64> tagIds;

    for (const QString& name : tagNames) {
        const QString trimmed = name.trimmed();
        if (trimmed.isEmpty()) continue;

        Tag::Ptr tag = findByName(trimmed);
        if (!tag) {
            // 自动创建新标签
            tag = Tag::create(trimmed);
            if (!save(tag)) {
                continue;
            }
        }
        tagIds.append(tag->id());
    }

    return setReportTags(reportId, tagIds);
}

QStringList TagRepository::findReportTagNames(qint64 reportId)
{
    QStringList names;
    const Tag::List tags = findByReport(reportId);
    for (const Tag::Ptr& tag : tags) {
        names.append(tag->name());
    }
    return names;
}

// ===========================================================================
// 统计
// ===========================================================================

int TagRepository::count()
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.exec("SELECT COUNT(*) FROM tags;");
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int TagRepository::usageCount(qint64 tagId)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM report_tags WHERE tag_id = :tagId;");
    query.bindValue(":tagId", tagId);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

void TagRepository::updateUsageCount(qint64 tagId)
{
    // 使用次数是查询时动态计算的，不需要存储
    Q_UNUSED(tagId);
}

// ===========================================================================
// 辅助方法
// ===========================================================================

Tag::Ptr TagRepository::createFromQuery(const QSqlQuery& query)
{
    Tag::Ptr tag(new Tag());
    tag->setId(query.value("id").toLongLong());
    tag->setName(query.value("name").toString());
    tag->setColor(query.value("color").toString());
    tag->setDescription(query.value("description").toString());
    tag->setCreatedAt(query.value("created_at").toDateTime());
    return tag;
}
