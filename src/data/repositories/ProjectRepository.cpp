/**
 * @file ProjectRepository.cpp
 * @brief 项目仓储类实现文件
 */

#include "ProjectRepository.h"
#include "data/database/DatabaseManager.h"
#include "core/utils/Logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QStringList>

// ===========================================================================
// 内部工具：将查询结果映射为 Project 对象
// ===========================================================================

Project::Ptr ProjectRepository::mapToProject(const QSqlQuery& query)
{
    Project::Ptr project = Project::create();

    project->setId(query.value("id").toLongLong());
    project->setName(query.value("name").toString());
    project->setType(query.value("type").toString());
    project->setDescription(query.value("description").toString());
    project->setStatus(Project::statusFromString(query.value("status").toString()));
    project->setOwner(query.value("owner").toString());
    project->setParentId(query.value("parent_id").toLongLong());
    project->setCreatedAt(query.value("created_at").toDateTime());
    project->setUpdatedAt(query.value("updated_at").toDateTime());

    return project;
}

// ===========================================================================
// 查询操作
// ===========================================================================

Project::Ptr ProjectRepository::findById(qint64 id)
{
    if (id <= 0) return nullptr;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    query.prepare("SELECT * FROM projects WHERE id = :id;");
    query.bindValue(":id", id);

    if (!query.exec()) {
        LOG_ERROR(QString("findById 失败: %1").arg(query.lastError().text()));
        return nullptr;
    }

    if (query.next()) {
        return mapToProject(query);
    }

    return nullptr;
}

Project::List ProjectRepository::findAll(const ProjectQuery& query)
{
    Project::List result;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery sqlQuery(db);

    // 动态构建 SQL 查询
    QString sql = "SELECT * FROM projects WHERE 1=1";
    QVariantList bindValues;

    // 关键词筛选（名称或描述模糊匹配）
    if (!query.keyword.isEmpty()) {
        sql += " AND (name LIKE :keyword OR description LIKE :keyword)";
        bindValues << QString("%%1%").arg(query.keyword);
    }

    // 类型筛选
    if (!query.type.isEmpty()) {
        sql += " AND type = :type";
        bindValues << query.type;
    }

    // 状态筛选
    if (static_cast<int>(query.status) >= 0) {
        sql += " AND status = :status";
        Project tempProject;
        tempProject.setStatus(query.status);
        bindValues << tempProject.statusToString();
        // 注意：这里用了一个临时 Project 对象来转换状态，
        // 更优雅的方式是给 ProjectStatus 写一个独立的转换函数。
        // 但为了保持简单，这里复用了 Project::statusToString。
    }

    // 父项目筛选
    if (query.parentId == 0) {
        // parent_id = 0 表示仅根项目（parent_id <= 0）
        sql += " AND parent_id <= 0";
    } else if (query.parentId > 0) {
        sql += " AND parent_id = :parentId";
        bindValues << query.parentId;
    }
    // parentId = -1 表示不筛选

    // 负责人筛选
    if (!query.owner.isEmpty()) {
        sql += " AND owner = :owner";
        bindValues << query.owner;
    }

    // 时间范围筛选
    if (query.dateFrom.isValid()) {
        sql += " AND created_at >= :dateFrom";
        bindValues << query.dateFrom;
    }
    if (query.dateTo.isValid()) {
        sql += " AND created_at <= :dateTo";
        bindValues << query.dateTo;
    }

    // 排序
    const QString validSortColumns[] = {"name", "created_at", "updated_at", "id"};
    QString sortColumn = "updated_at";
    for (const QString& col : validSortColumns) {
        if (query.sortBy == col) {
            sortColumn = col;
            break;
        }
    }
    sql += QString(" ORDER BY %1 %2;")
               .arg(sortColumn)
               .arg(query.sortOrder == Qt::AscendingOrder ? "ASC" : "DESC");

    sqlQuery.prepare(sql);

    // 绑定参数（按顺序）
    int index = 0;
    for (const QVariant& value : bindValues) {
        sqlQuery.bindValue(index++, value);
    }

    if (!sqlQuery.exec()) {
        LOG_ERROR(QString("findAll 失败: %1").arg(sqlQuery.lastError().text()));
        return result;
    }

    while (sqlQuery.next()) {
        result.append(mapToProject(sqlQuery));
    }

    return result;
}

Project::List ProjectRepository::findRootProjects()
{
    ProjectQuery query;
    query.parentId = -1;  // 仅根项目
    query.sortBy = "name";
    query.sortOrder = Qt::AscendingOrder;
    return findAll(query);
}

Project::List ProjectRepository::findChildren(qint64 parentId)
{
    if (parentId <= 0) return findRootProjects();

    ProjectQuery query;
    query.parentId = parentId;
    query.sortBy = "name";
    query.sortOrder = Qt::AscendingOrder;
    return findAll(query);
}

// ===========================================================================
// 插入操作
// ===========================================================================

bool ProjectRepository::insert(Project::Ptr project)
{
    if (!project) return false;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO projects (name, type, description, status, owner, parent_id, created_at, updated_at)
        VALUES (:name, :type, :description, :status, :owner, :parent_id, :created_at, :updated_at);
    )");

    const QDateTime now = QDateTime::currentDateTime();
    query.bindValue(":name", project->name());
    query.bindValue(":type", project->type());
    query.bindValue(":description", project->description());
    query.bindValue(":status", project->statusToString());
    query.bindValue(":owner", project->owner());
    query.bindValue(":parent_id", project->parentId() > 0 ? project->parentId() : QVariant(QVariant::LongLong));
    query.bindValue(":created_at", now);
    query.bindValue(":updated_at", now);

    if (!query.exec()) {
        LOG_ERROR(QString("insert 失败: %1").arg(query.lastError().text()));
        return false;
    }

    // 获取数据库生成的自增 ID
    const qint64 newId = query.lastInsertId().toLongLong();
    project->setId(newId);
    project->setCreatedAt(now);
    project->setUpdatedAt(now);

    LOG_INFO(QString("项目已创建: id=%1, name='%2'").arg(newId).arg(project->name()));
    return true;
}

// ===========================================================================
// 更新操作
// ===========================================================================

bool ProjectRepository::update(const Project::Ptr& project)
{
    if (!project || !project->isPersisted()) return false;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE projects
        SET name = :name,
            type = :type,
            description = :description,
            status = :status,
            owner = :owner,
            parent_id = :parent_id,
            updated_at = :updated_at
        WHERE id = :id;
    )");

    query.bindValue(":name", project->name());
    query.bindValue(":type", project->type());
    query.bindValue(":description", project->description());
    query.bindValue(":status", project->statusToString());
    query.bindValue(":owner", project->owner());
    query.bindValue(":parent_id", project->parentId() > 0 ? project->parentId() : QVariant(QVariant::LongLong));
    query.bindValue(":updated_at", QDateTime::currentDateTime());
    query.bindValue(":id", project->id());

    if (!query.exec()) {
        LOG_ERROR(QString("update 失败: %1").arg(query.lastError().text()));
        return false;
    }

    project->setUpdatedAt(QDateTime::currentDateTime());
    return true;
}

// ===========================================================================
// 删除操作
// ===========================================================================

bool ProjectRepository::remove(qint64 id)
{
    if (id <= 0) return false;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    // 使用事务确保级联删除的原子性
    DatabaseManager::instance().transaction();

    query.prepare("DELETE FROM projects WHERE id = :id;");
    query.bindValue(":id", id);

    if (!query.exec()) {
        LOG_ERROR(QString("remove 失败: %1").arg(query.lastError().text()));
        DatabaseManager::instance().rollback();
        return false;
    }

    DatabaseManager::instance().commit();
    LOG_INFO(QString("项目已删除: id=%1").arg(id));
    return true;
}

// ===========================================================================
// 存在性检查
// ===========================================================================

bool ProjectRepository::existsByName(const QString& name, qint64 excludeId)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    QString sql = "SELECT COUNT(*) FROM projects WHERE name = :name";
    if (excludeId > 0) {
        sql += " AND id != :excludeId";
    }
    sql += ";";

    query.prepare(sql);
    query.bindValue(":name", name);
    if (excludeId > 0) {
        query.bindValue(":excludeId", excludeId);
    }

    if (!query.exec() || !query.next()) {
        return false;
    }

    return query.value(0).toInt() > 0;
}

// ===========================================================================
// 统计操作
// ===========================================================================

int ProjectRepository::count()
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    query.exec("SELECT COUNT(*) FROM projects;");
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int ProjectRepository::countByStatus(ProjectStatus status)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    query.prepare("SELECT COUNT(*) FROM projects WHERE status = :status;");
    Project tempProject;
    tempProject.setStatus(status);
    query.bindValue(":status", tempProject.statusToString());

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int ProjectRepository::countChildren(qint64 parentId)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    if (parentId <= 0) {
        query.exec("SELECT COUNT(*) FROM projects WHERE parent_id <= 0;");
    } else {
        query.prepare("SELECT COUNT(*) FROM projects WHERE parent_id = :parentId;");
        query.bindValue(":parentId", parentId);
        query.exec();
    }

    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

// ===========================================================================
// 树状结构操作
// ===========================================================================

QString ProjectRepository::getPath(qint64 id)
{
    QStringList pathParts;
    qint64 currentId = id;

    // 向上遍历父项目链，最多 20 层（防止循环引用）
    for (int depth = 0; depth < 20 && currentId > 0; ++depth) {
        Project::Ptr project = findById(currentId);
        if (!project) break;

        pathParts.prepend(project->name());
        currentId = project->parentId();
    }

    return pathParts.join(" / ");
}

QList<qint64> ProjectRepository::getAllDescendantIds(qint64 rootId)
{
    QList<qint64> result;
    result.append(rootId);

    // 使用栈进行深度优先遍历（避免递归栈溢出）
    QList<qint64> stack;
    stack.append(rootId);

    while (!stack.isEmpty()) {
        const qint64 currentId = stack.takeLast();
        const Project::List children = findChildren(currentId);

        for (const Project::Ptr& child : children) {
            if (!result.contains(child->id())) {
                result.append(child->id());
                stack.append(child->id());
            }
        }
    }

    return result;
}
