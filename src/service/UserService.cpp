/**
 * @file UserService.cpp
 * @brief 用户业务逻辑服务层实现
 */

#include "UserService.h"
#include "core/utils/Logger.h"
#include "data/repositories/UserRepository.h"
#include <QtGlobal>

User::Ptr UserService::getById(qint64 id)
{
    return UserRepository::findById(id);
}

User::Ptr UserService::getByUsername(const QString& username)
{
    return UserRepository::findByUsername(username);
}

User::List UserService::listAll()
{
    return UserRepository::findAll();
}

User::Ptr UserService::authenticate(const QString& username, const QString& password)
{
    User::Ptr user = UserRepository::authenticate(username, password);
    if (!user) {
        LOG_WARNING(QString("登录失败: %1").arg(username));
    }
    return user;
}

bool UserService::changePassword(qint64 userId, const QString& newPassword)
{
    const bool ok = UserRepository::changePassword(userId, newPassword);
    if (!ok) {
        LOG_ERROR(QString("修改密码失败: userId=%1").arg(userId));
    }
    return ok;
}

bool UserService::resetPassword(qint64 userId)
{
    const bool ok = UserRepository::resetPassword(userId);
    if (!ok) {
        LOG_ERROR(QString("重置密码失败: userId=%1").arg(userId));
    }
    return ok;
}

void UserService::updateLastLogin(qint64 userId)
{
    UserRepository::updateLastLogin(userId);
}

User::Ptr UserService::create(const QString& username, const QString& displayName,
                               const QString& password, UserRole role)
{
    if (username.isEmpty() || password.isEmpty()) return nullptr;
    if (exists(username)) return nullptr;

    User::Ptr user = User::create();
    user->setUsername(username);
    user->setDisplayName(displayName.isEmpty() ? username : displayName);
    user->setPasswordHash(User::hashPassword(password));
    user->setRole(role);

    if (UserRepository::save(user)) {
        return user;
    }
    LOG_ERROR(QString("创建用户失败: %1").arg(username));
    return nullptr;
}

bool UserService::update(const User::Ptr& user)
{
    Q_ASSERT(user);
    if (!user) return false;
    return UserRepository::save(user);
}

bool UserService::save(const User::Ptr& user)
{
    if (!user) return false;
    return UserRepository::save(user);
}

bool UserService::remove(qint64 id)
{
    return UserRepository::remove(id);
}

bool UserService::exists(const QString& username)
{
    return UserRepository::exists(username);
}
