/**
 * @file GroupManagerDialog.cpp
 * @brief 组管理对话框实现文件
 *
 * 权限：仅超级管理员/总管可管理组（PermissionService::canManageGroups）。
 * 删除组时：该组组长降为组员，随后组删除并清除成员归属。
 */

#include "GroupManagerDialog.h"
#include "ui_GroupManagerDialog.h"
#include "service/GroupService.h"
#include "service/UserService.h"
#include "service/PermissionService.h"
#include "core/utils/UserSession.h"
#include "core/utils/AppDimensions.h"

#include <QHeaderView>
#include "ui/UiHelper.h"
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QHash>

GroupManagerDialog::GroupManagerDialog(QWidget* parent)
    : BaseDialog(parent)
    , ui(new Ui::GroupManagerDialog)
{
    ui->setupUi(this);
    resize(AppDimensions::Window::DialogSmallWidth, AppDimensions::Window::DialogSmallHeight);

    ui->groupTable->setColumnCount(3);
    ui->groupTable->setHorizontalHeaderLabels({tr("组名"), tr("描述"), tr("组长")});
    ui->groupTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->groupTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->groupTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->groupTable->horizontalHeader()->setStretchLastSection(true);
    ui->groupTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    UiHelper::centerTableWidget(ui->groupTable);

    loadGroups();
}

GroupManagerDialog::~GroupManagerDialog()
{
    delete ui;
}

void GroupManagerDialog::loadGroups()
{
    m_groups = GroupService::getAllGroups();

    // 组长列：以用户表 role=Leader 且 group_id 匹配为准
    QHash<qint64, QString> leaderNames;
    for (const User::Ptr& u : UserService::listAll()) {
        if (u->isLeader() && u->groupId() > 0) {
            leaderNames.insert(u->groupId(), u->displayNameOrUsername());
        }
    }

    ui->groupTable->setRowCount(m_groups.size());
    for (int row = 0; row < m_groups.size(); ++row) {
        const Group::Ptr& g = m_groups[row];
        ui->groupTable->setItem(row, 0, new QTableWidgetItem(g->name()));
        ui->groupTable->setItem(row, 1, new QTableWidgetItem(g->description()));
        ui->groupTable->setItem(row, 2, new QTableWidgetItem(leaderNames.value(g->id(), tr("—"))));
    }
}

Group::Ptr GroupManagerDialog::currentGroup() const
{
    const int row = ui->groupTable->currentRow();
    if (row >= 0 && row < m_groups.size()) {
        return m_groups[row];
    }
    return nullptr;
}

void GroupManagerDialog::on_groupTable_itemSelectionChanged()
{
    ui->btnDelete->setEnabled(currentGroup() != nullptr);
}

void GroupManagerDialog::on_btnClose_clicked()
{
    accept();
}

void GroupManagerDialog::on_btnAdd_clicked()
{
    const User::Ptr current = UserSession::instance().currentUser();
    if (!PermissionService::canManageGroups(current)) {
        UiHelper::warning(this, tr("无权限"), tr("只有超级管理员和总管可以管理组"));
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(tr("新建组"));
    dlg.setMinimumWidth(300);

    QFormLayout* form = new QFormLayout(&dlg);
    QLineEdit* nameEdit = new QLineEdit(&dlg);
    QLineEdit* descEdit = new QLineEdit(&dlg);

    form->addRow(tr("组名:"), nameEdit);
    form->addRow(tr("描述:"), descEdit);

    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    const QString name = nameEdit->text().trimmed();
    if (name.isEmpty()) {
        UiHelper::warning(this, tr("错误"), tr("组名不能为空"));
        return;
    }

    for (const Group::Ptr& g : m_groups) {
        if (g->name() == name) {
            UiHelper::warning(this, tr("错误"), tr("组名已存在"));
            return;
        }
    }

    const QString desc = descEdit->text().trimmed();
    if (GroupService::createGroup(name, desc)) {
        UiHelper::info(this, tr("成功"), tr("组已创建"));
        loadGroups();
    } else {
        UiHelper::error(this, tr("错误"), tr("创建组失败"));
    }
}

void GroupManagerDialog::on_btnDelete_clicked()
{
    Group::Ptr group = currentGroup();
    if (!group) return;

    const User::Ptr current = UserSession::instance().currentUser();
    if (!PermissionService::canManageGroups(current)) {
        UiHelper::warning(this, tr("无权限"), tr("只有超级管理员和总管可以管理组"));
        return;
    }

    if (!UiHelper::confirm(this,
                           tr("确认删除"),
                           tr("确定要删除组「%1」吗？\n该组所有成员将变为无组。")
                               .arg(group->name()))) return;

    // 该组组长降为组员
    for (const User::Ptr& u : UserService::listAll()) {
        if (u->groupId() == group->id() && u->isLeader()) {
            u->setRole(UserRole::Member);
            UserService::save(u);
        }
    }

    if (GroupService::deleteGroup(group->id())) {
        UiHelper::info(this, tr("成功"), tr("组已删除"));
        loadGroups();
    } else {
        UiHelper::error(this, tr("错误"), tr("删除组失败"));
    }
}
