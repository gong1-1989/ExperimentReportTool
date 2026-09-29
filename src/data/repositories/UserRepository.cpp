/**
 * @file UserRepository.cpp
 * @brief 用户数据访问层实现文件
 */

#include "UserRepository.h"
#include "data/database/DatabaseManager.h"
#include "core/utils/Logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QVariant>

// ============================================================================
// 用户 CRUD
// ============================================================================

User::Ptr UserRepository::findById(qint64 userId)
{
    QSqlQuery query(BaseRepository::db());
    query.prepare("SELECT * FROM users WHERE id = :id");
    query.bindValue(":id", userId);

    if (!BaseRepository::execChecked(query, "查询用户")) return nullptr;

    if (query.next()) {
        return mapToUser(query);
    }
    return nullptr;
}

User::Ptr UserRepository::findByUsername(const QString& username)
{
    QSqlQuery query(BaseRepository::db());
    query.prepare("SELECT * FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (!BaseRepository::execChecked(query, "查询用户")) return nullptr;

    if (query.next()) {
        return mapToUser(query);
    }
    return nullptr;
}

User::List UserRepository::findAll()
{
    User::List users;
    QSqlQuery query(BaseRepository::db());
    query.exec("SELECT * FROM users ORDER BY username");

    while (query.next()) {
        users.append(mapToUser(query));
    }
    return users;
}

bool UserRepository::save(User::Ptr user)
{
    if (!user) return false;

    QSqlQuery query(BaseRepository::db());

    if (user->isNew()) {
        // 新建
        query.prepare(
            "INSERT INTO users (username, password_hash, display_name, role, "
            "must_change_password, created_at, last_login_at) "
            "VALUES (:username, :password_hash, :display_name, :role, "
            ":must_change_password, :created_at, :last_login_at)");
        query.bindValue(":created_at", user->createdAt().isValid()
            ? user->createdAt() : QDateTime::currentDateTime());
    } else {
        // 更新
        query.prepare(
            "UPDATE users SET username = :username, password_hash = :password_hash, "
            "display_name = :display_name, role = :role, "
            "must_change_password = :must_change_password, last_login_at = :last_login_at "
            "WHERE id = :id");
        query.bindValue(":id", user->id());
    }

    query.bindValue(":username", user->username());
    query.bindValue(":password_hash", user->passwordHash());
    query.bindValue(":display_name", user->displayName());
    query.bindValue(":role", user->isAdmin() ? "admin" : "user");
    query.bindValue(":must_change_password", user->mustChangePassword());
    query.bindValue(":last_login_at", user->lastLoginAt());

    if (!BaseRepository::execChecked(query, "保存用户")) return false;

    if (user->isNew()) {
        user->setId(query.lastInsertId().toLongLong());
    }
    return true;
}

bool UserRepository::remove(qint64 userId)
{
    QSqlQuery query(BaseRepository::db());
    query.prepare("DELETE FROM users WHERE id = :id");
    query.bindValue(":id", userId);

    if (!BaseRepository::execChecked(query, "删除用户")) return false;
    return true;
}

bool UserRepository::exists(const QString& username, qint64 excludeId)
{
    QSqlQuery query(BaseRepository::db());
    if (excludeId > 0) {
        query.prepare("SELECT COUNT(*) FROM users WHERE username = :username AND id != :id");
        query.bindValue(":id", excludeId);
    } else {
        query.prepare("SELECT COUNT(*) FROM users WHERE username = :username");
    }
    query.bindValue(":username", username);

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

// ============================================================================
// 登录与密码
// ============================================================================

User::Ptr UserRepository::authenticate(const QString& username, const QString& password)
{
    User::Ptr user = findByUsername(username);
    if (!user) return nullptr;

    if (user->verifyPassword(password)) {
        return user;
    }
    return nullptr;
}

bool UserRepository::changePassword(qint64 userId, const QString& newPassword)
{
    User::Ptr user = findById(userId);
    if (!user) return false;

    user->setPasswordHash(User::hashPassword(newPassword));
    user->setMustChangePassword(false);
    return save(user);
}

void UserRepository::updateLastLogin(qint64 userId)
{
    QSqlQuery query(BaseRepository::db());
    query.prepare("UPDATE users SET last_login_at = :time WHERE id = :id");
    query.bindValue(":time", QDateTime::currentDateTime());
    query.bindValue(":id", userId);
    if (!query.exec()) {
        LOG_WARNING(QString("更新最后登录时间失败: id=%1, %2").arg(userId).arg(query.lastError().text()));
    }
}

bool UserRepository::resetPassword(qint64 userId)
{
    User::Ptr user = findById(userId);
    if (!user) return false;

    user->setPasswordHash(User::hashPassword("123456"));
    user->setMustChangePassword(true);
    return save(user);
}

// ============================================================================
// 初始化默认用户
// ============================================================================

void UserRepository::initializeDefaultUsers()
{
    // 检查是否已有用户
    QSqlQuery query(BaseRepository::db());
    query.exec("SELECT COUNT(*) FROM users");
    if (query.next() && query.value(0).toInt() > 0) {
        LOG_DEBUG("用户表已有数据，跳过默认用户初始化");
        return;
    }

    // 创建管理员账号
    User::Ptr admin = User::create();
    admin->setUsername("admin");
    admin->setPasswordHash(User::hashPassword("admin123"));
    admin->setDisplayName("系统管理员");
    admin->setRole(UserRole::Admin);
    admin->setMustChangePassword(false);
    admin->setCreatedAt(QDateTime::currentDateTime());
    if (save(admin)) {
        LOG_DEBUG("默认管理员账号已创建: admin（首次登录需改密码）");
    }

    // 创建测试账号
    User::Ptr test = User::create();
    test->setUsername("test");
    test->setPasswordHash(User::hashPassword("123456"));
    test->setDisplayName("测试用户");
    test->setRole(UserRole::User);
    test->setMustChangePassword(true);  // 首次登录必须改密码
    test->setCreatedAt(QDateTime::currentDateTime());
    if (save(test)) {
        LOG_DEBUG("默认测试账号已创建: test");
    }
}

// ============================================================================
// 内部方法
// ============================================================================

User::Ptr UserRepository::mapToUser(const QSqlQuery& query)
{
    User::Ptr user = User::create();
    user->setId(query.value("id").toLongLong());
    user->setUsername(query.value("username").toString());
    user->setPasswordHash(query.value("password_hash").toString());
    user->setDisplayName(query.value("display_name").toString());

    const QString roleStr = query.value("role").toString();
    user->setRole(roleStr == "admin" ? UserRole::Admin : UserRole::User);

    user->setMustChangePassword(query.value("must_change_password").toBool());
    user->setCreatedAt(query.value("created_at").toDateTime());
    user->setLastLoginAt(query.value("last_login_at").toDateTime());

    return user;
}
