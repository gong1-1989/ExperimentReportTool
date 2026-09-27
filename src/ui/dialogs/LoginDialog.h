/**
 * @file LoginDialog.h
 * @brief 用户登录对话框头文件
 *
 * 提供用户登录界面，验证用户名和密码后才允许进入主界面。
 * UI 布局由 LoginDialog.ui 可视化设计。
 */

#ifndef LOGIN_DIALOG_H
#define LOGIN_DIALOG_H

#include <QDialog>
#include <QString>

// UI 类前向声明（由 .ui 文件自动生成）
namespace Ui {
class LoginDialog;
}

/**
 * @brief 用户登录对话框类
 *
 * 使用方式：
 * @code
 *   LoginDialog login;
 *   if (login.exec() == QDialog::Accepted) {
 *       // 登录成功，显示主界面
 *   }
 * @endcode
 */
class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit LoginDialog(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~LoginDialog() override;

    /**
     * @brief 获取当前登录的用户名
     * @return 用户名
     */
    QString currentUsername() const { return m_username; }

private slots:
    /// 点击登录按钮
    void on_loginButton_clicked();

    /// 点击取消按钮
    void on_cancelButton_clicked();

    /// 用户名输入框按回车
    void on_usernameEdit_returnPressed();

    /// 密码输入框按回车
    void on_passwordEdit_returnPressed();

private:
    // ========================================================================
    // 成员变量
    // ========================================================================
    Ui::LoginDialog* ui;  ///< UI 界面对象（由 .ui 文件生成）
    QString m_username;   ///< 当前登录的用户名

    // ========================================================================
    // 内部方法
    // ========================================================================

    /**
     * @brief 验证用户名和密码
     * @param username 用户名
     * @param password 密码
     * @return 验证成功返回 true，否则返回 false
     */
    bool verifyCredentials(const QString& username, const QString& password);

    /**
     * @brief 显示错误信息
     * @param message 错误信息
     */
    void showError(const QString& message);

    /**
     * @brief 确保默认用户存在（首次运行时创建 admin 账号）
     */
    void ensureDefaultUser();

    /**
     * @brief 对密码进行哈希处理
     * @param password 明文密码
     * @return 哈希后的密码字符串
     */
    static QString hashPassword(const QString& password);
};

#endif // LOGIN_DIALOG_H
