/**
 * @file ChangePasswordDialog.h
 * @brief 修改密码对话框头文件
 */

#ifndef CHANGE_PASSWORD_DIALOG_H
#define CHANGE_PASSWORD_DIALOG_H

#include "BaseDialog.h"

namespace Ui {
class ChangePasswordDialog;
}

/**
 * @brief 修改密码对话框
 *
 * 用于首次登录强制改密码，或用户主动修改密码。
 */
class ChangePasswordDialog : public BaseDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
         * @param parent 父窗口
     */
    explicit ChangePasswordDialog(QWidget* parent = nullptr);
    ~ChangePasswordDialog() override;

    /// 获取新密码
    QString newPassword() const { return m_newPassword; }

    /// 获取原密码
    QString oldPassword() const;

private slots:
    void on_okButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::ChangePasswordDialog* ui;
    QString m_newPassword;  ///< 新密码

    /// 验证密码强度
    bool validatePassword(const QString& password, QString* errorMsg = nullptr) const;
};

#endif // CHANGE_PASSWORD_DIALOG_H
