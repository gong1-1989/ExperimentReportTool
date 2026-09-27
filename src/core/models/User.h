/**
 * @file User.h
 * @brief 用户实体头文件
 *
 * 用户系统支持多用户登录、权限管理、数据隔离。
 * 角色分为管理员(admin)和普通用户(user)。
 * 普通用户可以查看所有人的数据，但只能修改自己创建的数据。
 */

#ifndef USER_H
#define USER_H

#include <QString>
#include <QDateTime>
#include <QSharedPointer>
#include <QList>

/**
 * @brief 用户角色枚举
 */
enum class UserRole {
    Admin,   ///< 管理员（全部权限 + 用户管理）
    User     ///< 普通用户（只能改自己的数据）
};

/**
 * @brief 用户实体类
 */
class User
{
public:
    using Ptr = QSharedPointer<User>;
    using List = QList<Ptr>;

    User();
    ~User();

    // -----------------------------------------------------------------------
    // 属性访问
    // -----------------------------------------------------------------------

    qint64 id() const { return m_id; }
    void setId(qint64 id) { m_id = id; }

    QString username() const { return m_username; }
    void setUsername(const QString& username) { m_username = username; }

    QString passwordHash() const { return m_passwordHash; }
    void setPasswordHash(const QString& hash) { m_passwordHash = hash; }

    QString displayName() const { return m_displayName; }
    void setDisplayName(const QString& name) { m_displayName = name; }

    UserRole role() const { return m_role; }
    void setRole(UserRole role) { m_role = role; }

    bool isAdmin() const { return m_role == UserRole::Admin; }

    bool mustChangePassword() const { return m_mustChangePassword; }
    void setMustChangePassword(bool must) { m_mustChangePassword = must; }

    QDateTime createdAt() const { return m_createdAt; }
    void setCreatedAt(const QDateTime& dt) { m_createdAt = dt; }

    QDateTime lastLoginAt() const { return m_lastLoginAt; }
    void setLastLoginAt(const QDateTime& dt) { m_lastLoginAt = dt; }

    // -----------------------------------------------------------------------
    // 工具方法
    // -----------------------------------------------------------------------

    /// 是否为新用户（未保存到数据库）
    bool isNew() const { return m_id <= 0; }

    /// 显示名称（优先 displayName，否则 username）
    QString displayNameOrUsername() const {
        return m_displayName.isEmpty() ? m_username : m_displayName;
    }

    /// 角色名称
    QString roleName() const {
        return m_role == UserRole::Admin ? QStringLiteral("管理员") : QStringLiteral("普通用户");
    }

    /// 密码哈希（使用 SHA256 + 盐）
    static QString hashPassword(const QString& password, const QString& salt = QString());

    /// 验证密码
    bool verifyPassword(const QString& password) const;

    /// 创建用户实例
    static Ptr create() { return Ptr(new User()); }

private:
    qint64 m_id = -1;                    ///< 用户 ID
    QString m_username;                  ///< 登录用户名
    QString m_passwordHash;              ///< 密码哈希
    QString m_displayName;               ///< 显示名称
    UserRole m_role = UserRole::User;    ///< 角色
    bool m_mustChangePassword = false;   ///< 是否必须改密码（首次登录）
    QDateTime m_createdAt;               ///< 创建时间
    QDateTime m_lastLoginAt;             ///< 最后登录时间
};

#endif // USER_H
