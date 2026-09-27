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
    static Project::Ptr create(const QString& name, qint64 parentId = 0);
    static bool update(const Project::Ptr& project);
    static bool remove(qint64 id);
    static Project::List children(qint64 parentId);

private:
    ProjectService() = delete;
    ~ProjectService() = delete;
};

#endif // PROJECT_SERVICE_H
