/**
 * @file UserManagerDialog.h
 * @brief 用户管理对话框头文件
 *
 * 管理员可在此对话框中增删改查用户、重置密码。
 */

#ifndef USER_MANAGER_DIALOG_H
#define USER_MANAGER_DIALOG_H

#include "BaseDialog.h"
#include <QTableWidget>
#include <QPushButton>
#include "core/models/User.h"

namespace Ui {
class UserManagerDialog;
}

/**
 * @brief 用户管理对话框
 */
class UserManagerDialog : public BaseDialog
{
    Q_OBJECT

public:
    explicit UserManagerDialog(QWidget* parent = nullptr);
    ~UserManagerDialog() override;

private slots:
    void on_btnAdd_clicked();
    void on_btnEdit_clicked();
    void on_btnDelete_clicked();
    void on_btnResetPassword_clicked();
    void on_btnRefresh_clicked();
    void on_btnClose_clicked();
    void on_userTable_itemSelectionChanged();

private:
    void loadUsers();
    User::Ptr currentUser() const;

    Ui::UserManagerDialog* ui;

    User::List m_users;
};

#endif // USER_MANAGER_DIALOG_H
