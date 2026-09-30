/**
 * @file GroupManagerDialog.h
 * @brief 组管理对话框头文件
 *
 * 超级管理员/总管可在此创建、删除组。
 * 组长的任命在用户管理对话框中完成（将用户角色设为组长并选择所属组）。
 */

#ifndef GROUP_MANAGER_DIALOG_H
#define GROUP_MANAGER_DIALOG_H

#include "BaseDialog.h"
#include <QTableWidget>
#include <QPushButton>
#include "core/models/Group.h"
#include "core/models/User.h"

namespace Ui {
class GroupManagerDialog;
}

/**
 * @brief 组管理对话框
 */
class GroupManagerDialog : public BaseDialog
{
    Q_OBJECT

public:
    explicit GroupManagerDialog(QWidget* parent = nullptr);
    ~GroupManagerDialog() override;

private slots:
    void on_btnAdd_clicked();
    void on_btnDelete_clicked();
    void on_btnClose_clicked();
    void on_groupTable_itemSelectionChanged();

private:
    void loadGroups();
    Group::Ptr currentGroup() const;

    Ui::GroupManagerDialog* ui;

    Group::List m_groups;
};

#endif // GROUP_MANAGER_DIALOG_H
