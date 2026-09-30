/**
 * @file User.h
 * @brief 用户实体头文件
 *
 * 用户系统支持多用户登录、权限管理、数据隔离。
 * 角色四级：超级管理员(super_admin) > 总管(manager) > 组长(leader) > 组员(member)。
 * 超级管理员唯一，拥有全部权限；总管可审批/归档/管理组长和组员；
 * 组长可审核/管理本组组员；组员仅能编写和管理自己的报告。
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
    SuperAdmin,  ///< 超级管理员（唯一，全部权限）
    Manager,     ///< 总管（审批/归档/管理组长和组员，不可删除用户）
    Leader,      ///< 组长（审核本组报告/管理本组组员）
    Member       ///< 组员（编写/管理自己的报告）
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

    bool isSuperAdmin() const { return m_role == UserRole::SuperAdmin; }
    bool isManager() const { return m_role == UserRole::Manager; }
    bool isLeader() const { return m_role == UserRole::Leader; }
    bool isMember() const { return m_role == UserRole::Member; }
    /// 兼容旧代码：管理员=超级管理员或总管
    bool isAdmin() const { return m_role == UserRole::SuperAdmin || m_role == UserRole::Manager; }
    /// 是否有审核权限（组长及以上）
    bool canReview() const { return m_role == UserRole::SuperAdmin || m_role == UserRole::Manager || m_role == UserRole::Leader; }
    /// 是否有审批权限（总管及以上）
    bool canApprove() const { return m_role == UserRole::SuperAdmin || m_role == UserRole::Manager; }

    bool isDisabled() const { return m_disabled; }
    void setDisabled(bool disabled) { m_disabled = disabled; }

    qint64 groupId() const { return m_groupId; }
    void setGroupId(qint64 id) { m_groupId = id; }

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
        switch (m_role) {
        case UserRole::SuperAdmin: return QStringLiteral("超级管理员");
        case UserRole::Manager:    return QStringLiteral("总管");
        case UserRole::Leader:     return QStringLiteral("组长");
        case UserRole::Member:     return QStringLiteral("组员");
        }
        return QStringLiteral("未知");
    }

    /// 角色字符串（数据库存储用）
    QString roleString() const {
        switch (m_role) {
        case UserRole::SuperAdmin: return QStringLiteral("super_admin");
        case UserRole::Manager:    return QStringLiteral("manager");
        case UserRole::Leader:     return QStringLiteral("leader");
        case UserRole::Member:     return QStringLiteral("member");
        }
        return QStringLiteral("member");
    }

    /// 从字符串解析角色
    static UserRole roleFromString(const QString& str) {
        if (str == "super_admin" || str == "admin") return UserRole::SuperAdmin;
        if (str == "manager") return UserRole::Manager;
        if (str == "leader") return UserRole::Leader;
        return UserRole::Member;  // "user" 及未知均视为组员
    }

    /// 密码哈希（使用 SHA256 + 盐）
    static QString hashPassword(const QString& password, const QString& salt = QString());

    /// 验证密码
    bool verifyPassword(const QString& password) const;

    /// 是否为旧版固定盐哈希（登录成功后据此触发自动迁移）
    bool usesLegacyHash() const;

    /// 创建用户实例
    static Ptr create() { return Ptr(new User()); }

private:
    qint64 m_id = -1;                    ///< 用户 ID
    QString m_username;                  ///< 登录用户名
    QString m_passwordHash;              ///< 密码哈希
    QString m_displayName;               ///< 显示名称
    UserRole m_role = UserRole::Member;  ///< 角色
    bool m_disabled = false;              ///< 账户是否禁用
    qint64 m_groupId = -1;                ///< 所属组 ID（-1=无组，超管/总管无组）
    bool m_mustChangePassword = false;   ///< 是否必须改密码（首次登录）
    QDateTime m_createdAt;               ///< 创建时间
    QDateTime m_lastLoginAt;             ///< 最后登录时间
};

#endif // USER_H
