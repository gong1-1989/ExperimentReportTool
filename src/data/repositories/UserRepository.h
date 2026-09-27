/**
 * @file UserRepository.h
 * @brief 用户数据访问层头文件
 *
 * 提供用户的增删改查、登录验证、密码修改等操作。
 */

#ifndef USER_REPOSITORY_H
#define USER_REPOSITORY_H

#include <QList>
#include <QString>
#include <QSqlQuery>
#include "core/models/User.h"

/**
 * @brief 用户仓储类
 */
class UserRepository
{
public:
    // -----------------------------------------------------------------------
    // 用户 CRUD
    // -----------------------------------------------------------------------

    /// 根据 ID 查找用户
    static User::Ptr findById(qint64 userId);

    /// 根据用户名查找用户
    static User::Ptr findByUsername(const QString& username);

    /// 获取所有用户
    static User::List findAll();

    /// 保存用户（新建或更新）
    static bool save(User::Ptr user);

    /// 删除用户
    static bool remove(qint64 userId);

    /// 检查用户名是否已存在
    static bool exists(const QString& username, qint64 excludeId = -1);

    // -----------------------------------------------------------------------
    // 登录与密码
    // -----------------------------------------------------------------------

    /**
     * @brief 验证登录
     * @param username 用户名
     * @param password 明文密码
     * @return 验证成功返回用户对象，失败返回 nullptr
     */
    static User::Ptr authenticate(const QString& username, const QString& password);

    /**
     * @brief 修改密码
     * @param userId 用户 ID
     * @param newPassword 新密码（明文）
     * @return 是否成功
     */
    static bool changePassword(qint64 userId, const QString& newPassword);

    /**
     * @brief 更新最后登录时间
     * @param userId 用户 ID
     */
    static void updateLastLogin(qint64 userId);

    /**
     * @brief 重置密码为默认密码（123456），并设置必须改密码
     * @param userId 用户 ID
     */
    static bool resetPassword(qint64 userId);

    // -----------------------------------------------------------------------
    // 初始化
    // -----------------------------------------------------------------------

    /**
     * @brief 初始化默认用户（如果 users 表为空）
     *
     * 创建一个测试账号：
     * - 用户名：test
     * - 密码：123456
     * - 角色：普通用户
     * - 首次登录必须改密码
     *
     * 同时创建一个管理员账号：
     * - 用户名：admin
     * - 密码：admin123
     * - 角色：管理员
     */
    static void initializeDefaultUsers();

private:
    /// 从查询结果映射为 User 对象
    static User::Ptr mapToUser(const QSqlQuery& query);
};

#endif // USER_REPOSITORY_H
