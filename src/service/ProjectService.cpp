/**
 * @file ProjectService.cpp
 * @brief 项目业务逻辑服务层实现
 */

#include "ProjectService.h"
#include "core/utils/Logger.h"
#include "data/repositories/ProjectRepository.h"
#include <QtGlobal>

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
    LOG_ERROR(QString("创建项目失败: %1").arg(name));
    return nullptr;
}

bool ProjectService::save(const Project::Ptr& project)
{
    Q_ASSERT(project);
    if (!project) return false;
    if (project->id() > 0) {
        return ProjectRepository::update(project);
    }
    return ProjectRepository::insert(project);
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

int ProjectService::countChildren(qint64 parentId)
{
    return ProjectRepository::countChildren(parentId);
}

int ProjectService::count()
{
    return ProjectRepository::count();
}

int ProjectService::countByParent(qint64 parentId)
{
    return ProjectRepository::countChildren(parentId);
}

bool ProjectService::existsByName(const QString& name, qint64 excludeId)
{
    return ProjectRepository::existsByName(name, excludeId);
}
