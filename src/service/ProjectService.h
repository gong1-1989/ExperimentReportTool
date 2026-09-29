/**
 * @file ProjectService.h
 * @brief 项目业务逻辑服务层
 */

#ifndef PROJECT_SERVICE_H
#define PROJECT_SERVICE_H

#include "core/models/Project.h"

class ProjectService
{
public:
    static Project::Ptr getById(qint64 id);
    static Project::List listAll();
    /// 批量查询多个项目 id → 项目名（搜索结果/列表渲染用，一次 SQL）
    static QHash<qint64, QString> findNamesBatch(const QList<qint64>& projectIds);
    static Project::Ptr create(const QString& name, qint64 parentId = 0);
    static bool save(const Project::Ptr& project);
    static bool update(const Project::Ptr& project);
    static bool remove(qint64 id);
    static Project::List children(qint64 parentId);
    static int countChildren(qint64 parentId);
    static int count();
    static int countByParent(qint64 parentId);
    static bool existsByName(const QString& name, qint64 excludeId = -1);

private:
    ProjectService() = delete;
    ~ProjectService() = delete;
};

#endif // PROJECT_SERVICE_H
