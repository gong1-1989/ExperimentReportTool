/**
 * @file GroupService.h
 * @brief 组业务服务头文件
 *
 * 提供组的创建、编辑、删除、查询，以及组成员管理。
 * 权限控制在 UI 层（调用方）做，Service 层只做业务逻辑。
 */

#ifndef GROUP_SERVICE_H
#define GROUP_SERVICE_H

#include <QList>
#include "core/models/Group.h"
#include "core/models/User.h"

class GroupService
{
public:
    /// 创建组（返回创建的组，失败返回 nullptr）
    static Group::Ptr createGroup(const QString& name, const QString& description = QString(), qint64 leaderId = -1);
    /// 更新组信息
    static bool updateGroup(qint64 groupId, const QString& name, const QString& description, qint64 leaderId);
    /// 删除组（同时清除用户的组归属）
    static bool deleteGroup(qint64 groupId);
    /// 获取所有组
    static Group::List getAllGroups();
    /// 根据 ID 获取组
    static Group::Ptr getGroupById(qint64 groupId);
    /// 获取组的所有组员
    static User::List getGroupMembers(qint64 groupId);
    /// 将用户加入组
    static bool addUserToGroup(qint64 userId, qint64 groupId);
    /// 将用户移出组
    static bool removeUserFromGroup(qint64 userId);
};

#endif // GROUP_SERVICE_H
