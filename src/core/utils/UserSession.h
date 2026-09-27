/**
 * @file UserSession.h
 * @brief 当前用户会话管理
 *
 * 单例模式，保存当前登录用户信息，全局可访问。
 */

#ifndef USER_SESSION_H
#define USER_SESSION_H

#include "core/models/User.h"

/**
 * @brief 用户会话单例
 */
class UserSession
{
public:
    /// 获取单例实例
    static UserSession& instance();

    /// 当前登录用户
    User::Ptr currentUser() const { return m_currentUser; }

    /// 设置当前用户（登录成功后调用）
    void setCurrentUser(User::Ptr user) { m_currentUser = user; }

    /// 清除当前用户（登出）
    void clear() { m_currentUser.reset(); }

    /// 是否已登录
    bool isLoggedIn() const { return !m_currentUser.isNull(); }

    /// 当前用户 ID（未登录返回 -1）
    qint64 userId() const { return m_currentUser ? m_currentUser->id() : -1; }

    /// 当前用户名
    QString username() const { return m_currentUser ? m_currentUser->username() : QString(); }

    /// 当前用户显示名称
    QString displayName() const {
        return m_currentUser ? m_currentUser->displayNameOrUsername() : QString();
    }

    /// 是否为管理员
    bool isAdmin() const { return m_currentUser && m_currentUser->isAdmin(); }

private:
    UserSession() = default;
    ~UserSession() = default;
    UserSession(const UserSession&) = delete;
    UserSession& operator=(const UserSession&) = delete;

    User::Ptr m_currentUser;  ///< 当前登录用户
};

#endif // USER_SESSION_H
