/**
 * @file UserService.cpp
 * @brief 用户业务逻辑服务层实现
 */

#include "UserService.h"
#include "AuditService.h"
#include "core/utils/Logger.h"
#include "data/database/DatabaseManager.h"
#include "data/repositories/UserRepository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSet>
#include <QStringList>
#include <QtGlobal>

User::Ptr UserService::getById(qint64 id)
{
    return UserRepository::findById(id);
}

QMap<qint64, QString> UserService::batchDisplayNames(const QList<qint64>& userIds)
{
    QMap<qint64, QString> result;

    // 去重并过滤无效 id
    QList<qint64> uniqueIds;
    QSet<qint64> seen;
    for (qint64 id : userIds) {
        if (id > 0 && !seen.contains(id)) {
            seen.insert(id);
            uniqueIds.append(id);
        }
    }
    if (uniqueIds.isEmpty()) return result;

    // id 为纯整数，可直接拼接（避免 QSqlQuery 的 IN 绑定繁琐）；仅出现一次，无注入风险
    QStringList placeholders;
    for (qint64 id : uniqueIds) {
        placeholders << QString::number(id);
    }

    QSqlQuery query(DatabaseManager::instance().database());
    const QString sql = QString("SELECT id, display_name, username FROM users WHERE id IN (%1)")
                            .arg(placeholders.join(QStringLiteral(", ")));
    if (!query.exec(sql)) {
        LOG_WARNING(QString("批量查询用户显示名失败: %1").arg(query.lastError().text()));
        return result;
    }
    while (query.next()) {
        const qint64 id = query.value(0).toLongLong();
        const QString displayName = query.value(1).toString();
        const QString username = query.value(2).toString();
        result.insert(id, displayName.isEmpty() ? username : displayName);
    }
    return result;
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
        AuditService::log(QObject::tr("创建用户"), username);
        return user;
    }
    LOG_ERROR(QString("创建用户失败: %1").arg(username));
    return nullptr;
}

bool UserService::update(const User::Ptr& user)
{
    Q_ASSERT(user);
    if (!user) return false;
    const bool ok = UserRepository::save(user);
    if (ok) AuditService::log(QObject::tr("更新用户"), user->username());
    return ok;
}

bool UserService::save(const User::Ptr& user)
{
    if (!user) return false;
    return UserRepository::save(user);
}

bool UserService::remove(qint64 id)
{
    const QString name = UserRepository::findById(id)
                             ? UserRepository::findById(id)->username() : QString();
    const bool ok = UserRepository::remove(id);
    if (ok) AuditService::log(QObject::tr("删除用户"), name, id);
    return ok;
}

bool UserService::exists(const QString& username)
{
    return UserRepository::exists(username);
}
