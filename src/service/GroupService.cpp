/**
 * @file GroupService.cpp
 * @brief 组业务服务实现
 */

#include "GroupService.h"
#include "data/repositories/GroupRepository.h"
#include "data/repositories/UserRepository.h"
#include "core/utils/Logger.h"

Group::Ptr GroupService::createGroup(const QString& name, const QString& description, qint64 leaderId)
{
    if (name.trimmed().isEmpty()) return nullptr;
    if (GroupRepository::existsByName(name)) {
        LOG_WARNING(QString("组名已存在: %1").arg(name));
        return nullptr;
    }
    Group::Ptr group = Group::create();
    group->setName(name.trimmed());
    group->setDescription(description);
    group->setLeaderId(leaderId);
    if (!GroupRepository::insert(group)) return nullptr;
    // 如果指定了组长，同时设置该用户的组归属和角色
    if (leaderId > 0) {
        User::Ptr leader = UserRepository::findById(leaderId);
        if (leader) {
            leader->setGroupId(group->id());
            leader->setRole(UserRole::Leader);
            UserRepository::save(leader);
        }
    }
    return group;
}

bool GroupService::updateGroup(qint64 groupId, const QString& name, const QString& description, qint64 leaderId)
{
    Group::Ptr group = GroupRepository::findById(groupId);
    if (!group) return false;
    if (!name.trimmed().isEmpty()) group->setName(name.trimmed());
    group->setDescription(description);
    group->setLeaderId(leaderId);
    if (!GroupRepository::update(group)) return false;
    if (leaderId > 0) {
        User::Ptr leader = UserRepository::findById(leaderId);
        if (leader && leader->groupId() != groupId) {
            leader->setGroupId(groupId);
            leader->setRole(UserRole::Leader);
            UserRepository::save(leader);
        }
    }
    return true;
}

bool GroupService::deleteGroup(qint64 groupId)
{
    return GroupRepository::remove(groupId);
}

Group::List GroupService::getAllGroups()
{
    return GroupRepository::findAll();
}

Group::Ptr GroupService::getGroupById(qint64 groupId)
{
    return GroupRepository::findById(groupId);
}

User::List GroupService::getGroupMembers(qint64 groupId)
{
    User::List all = UserRepository::findAll();
    User::List result;
    for (const User::Ptr& u : all) {
        if (u->groupId() == groupId) result.append(u);
    }
    return result;
}

bool GroupService::addUserToGroup(qint64 userId, qint64 groupId)
{
    User::Ptr user = UserRepository::findById(userId);
    if (!user) return false;
    user->setGroupId(groupId);
    return UserRepository::save(user);
}

bool GroupService::removeUserFromGroup(qint64 userId)
{
    User::Ptr user = UserRepository::findById(userId);
    if (!user) return false;
    user->setGroupId(-1);
    return UserRepository::save(user);
}
