/**
 * @file UserService.h
 * @brief 用户业务逻辑服务层
 */

#ifndef USER_SERVICE_H
#define USER_SERVICE_H

#include "core/models/User.h"
#include <QMap>
#include <QList>

class UserService
{
public:
    static User::Ptr getById(qint64 id);
    /// 批量查询用户显示名（一次 SQL），返回 id → 显示名/用户名；不存在或无效 id 不包含在结果中
    static QMap<qint64, QString> batchDisplayNames(const QList<qint64>& userIds);
    static User::Ptr getByUsername(const QString& username);
    static User::List listAll();
    static User::Ptr authenticate(const QString& username, const QString& password);
    static bool changePassword(qint64 userId, const QString& newPassword);
    static bool resetPassword(qint64 userId);
    static void updateLastLogin(qint64 userId);
    static User::Ptr create(const QString& username, const QString& displayName,
                            const QString& password, UserRole role = UserRole::User);
    static bool save(const User::Ptr& user);
    static bool update(const User::Ptr& user);
    static bool remove(qint64 id);
    static bool exists(const QString& username);

private:
    UserService() = delete;
    ~UserService() = delete;
};

#endif // USER_SERVICE_H
