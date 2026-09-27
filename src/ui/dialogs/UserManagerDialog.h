/**
 * @file UserManagerDialog.h
 * @brief 用户管理对话框头文件
 *
 * 管理员可在此对话框中增删改查用户、重置密码。
 */

#ifndef USER_MANAGER_DIALOG_H
#define USER_MANAGER_DIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include "core/models/User.h"

/**
 * @brief 用户管理对话框
 */
class UserManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UserManagerDialog(QWidget* parent = nullptr);
    ~UserManagerDialog() override;

private slots:
    void onAddUser();
    void onEditUser();
    void onDeleteUser();
    void onResetPassword();
    void onRefresh();
    void onItemSelectionChanged();

private:
    void setupUi();
    void loadUsers();
    User::Ptr currentUser() const;

    QTableWidget* m_userTable;
    QPushButton* m_btnAdd;
    QPushButton* m_btnEdit;
    QPushButton* m_btnDelete;
    QPushButton* m_btnResetPassword;
    QPushButton* m_btnRefresh;
    QPushButton* m_btnClose;

    User::List m_users;
};

#endif // USER_MANAGER_DIALOG_H
