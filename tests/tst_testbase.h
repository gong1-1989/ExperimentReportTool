/**
 * @file tst_testbase.h
 * @brief 权限/工作流测试共享基类：临时数据库 + 测试数据工厂
 *
 * 依赖真实 SQLite（DatabaseManager::initialize 可指定路径），
 * 因此覆盖的是"查库逻辑"层（PermissionService / WorkflowService），
 * 而非纯模型逻辑。每个测试可执行文件独立进程，互不影响。
 */

#pragma once

#include <QTemporaryDir>
#include <QDir>

#include "core/models/User.h"
#include "core/models/Group.h"
#include "core/models/Project.h"
#include "core/models/Report.h"
#include "data/database/DatabaseManager.h"
#include "data/repositories/UserRepository.h"
#include "data/repositories/GroupRepository.h"
#include "data/repositories/ProjectRepository.h"
#include "service/ReportService.h"

class TestDbBase
{
protected:
    QTemporaryDir m_dir;
    qint64 m_projectId = -1;

    void initDb()
    {
        QVERIFY2(m_dir.isValid(), "无法创建临时目录");
        QDir().mkpath(m_dir.path());
        QVERIFY2(DatabaseManager::instance().initialize(m_dir.filePath("test.db")),
                 "数据库初始化失败");

        Project::Ptr p = Project::create();
        p->setName(QStringLiteral("测试项目"));
        QVERIFY2(ProjectRepository::insert(p), "插入测试项目失败");
        m_projectId = p->id();
    }

    void closeDb()
    {
        DatabaseManager::instance().close();
    }

    User::Ptr addUser(const QString& name, UserRole role,
                      qint64 groupId = -1, bool disabled = false)
    {
        User::Ptr u = User::create();
        u->setUsername(name);
        u->setPasswordHash(QStringLiteral("x"));
        u->setDisplayName(name);
        u->setRole(role);
        u->setGroupId(groupId);
        u->setDisabled(disabled);
        QVERIFY2(UserRepository::save(u), ("插入用户失败: " + name).toUtf8());
        return u;
    }

    Group::Ptr addGroup(const QString& name, qint64 leaderId)
    {
        Group::Ptr g = Group::create();
        g->setName(name);
        g->setLeaderId(leaderId);
        QVERIFY2(GroupRepository::insert(g), ("插入组失败: " + name).toUtf8());
        return g;
    }

    Report::Ptr addReport(const QString& title, qint64 createdBy, ReportStatus st)
    {
        Report::Ptr r = Report::create();
        r->setTitle(title);
        r->setProjectId(m_projectId);
        r->setCreatedBy(createdBy);
        r->setStatus(st);
        QVERIFY2(ReportService::save(r), ("插入报告失败: " + title).toUtf8());
        return r;
    }

    Report::Ptr fresh(qint64 id)
    {
        return ReportService::getById(id);
    }
};
