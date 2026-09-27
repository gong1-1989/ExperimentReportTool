/**
 * @file ProjectService.cpp
 * @brief 项目业务逻辑服务层实现
 */

#include "ProjectService.h"
#include "data/repositories/ProjectRepository.h"

Project::Ptr ProjectService::getById(qint64 id)
{
    return ProjectRepository::findById(id);
}

Project::List ProjectService::listAll()
{
    return ProjectRepository::findAll();
}

Project::Ptr ProjectService::create(const QString& name, qint64 parentId)
{
    Project::Ptr project = Project::create();
    project->setName(name);
    project->setParentId(parentId);

    if (ProjectRepository::insert(project)) {
        return project;
    }
    return nullptr;
}

bool ProjectService::update(const Project::Ptr& project)
{
    if (!project) return false;
    return ProjectRepository::update(project);
}

bool ProjectService::remove(qint64 id)
{
    return ProjectRepository::remove(id);
}

Project::List ProjectService::children(qint64 parentId)
{
    return ProjectRepository::findChildren(parentId);
}
