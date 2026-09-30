/**
 * @file UserManagerDialog.cpp
 * @brief 用户管理对话框实现文件
 *
 * v9.20.0 起支持四级角色（超级管理员/总管/组长/组员）与组归属管理，
 * 并按当前登录用户权限过滤可执行操作：
 *   - 超级管理员：可管理所有用户（含删除）
 *   - 总管：可管理组长与组员（可设置任意组）
 *   - 组长：仅可管理本组组员（组固定为本组）
 */

#include "UserManagerDialog.h"
#include "GroupManagerDialog.h"
#include "ui_UserManagerDialog.h"
#include "service/UserService.h"
#include "service/GroupService.h"
#include "service/PermissionService.h"
#include "data/repositories/UserRepository.h"
#include "core/utils/UserSession.h"
#include "core/utils/AppDimensions.h"
#include "core/models/Group.h"

#include <QHeaderView>
#include "ui/UiHelper.h"
#include <QComboBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QLabel>
#include <QFormLayout>
#include <QHash>

namespace {

/// 当前用户可创建/授予的角色（从低到高）
QList<UserRole> allowedRolesFor(const User::Ptr& current)
{
    QList<UserRole> roles;
    if (!current) return roles;
    if (current->isSuperAdmin()) {
        roles << UserRole::Member << UserRole::Leader << UserRole::Manager << UserRole::SuperAdmin;
    } else if (current->isManager()) {
        roles << UserRole::Member << UserRole::Leader;
    } else if (current->isLeader()) {
        roles << UserRole::Member;
    }
    return roles;
}

QString roleDisplayName(UserRole role)
{
    switch (role) {
    case UserRole::SuperAdmin: return QObject::tr("超级管理员");
    case UserRole::Manager:    return QObject::tr("总管");
    case UserRole::Leader:     return QObject::tr("组长");
    case UserRole::Member:     return QObject::tr("组员");
    }
    return QString();
}

/// 当前用户可见的组列表（组长只能看到本组）
QList<Group::Ptr> visibleGroups(const User::Ptr& current)
{
    QList<Group::Ptr> groups = GroupService::getAllGroups();
    if (current && current->isLeader() && current->groupId() > 0) {
        QList<Group::Ptr> mine;
        for (const Group::Ptr& g : groups) {
            if (g->id() == current->groupId()) mine << g;
        }
        return mine;
    }
    return groups;
}

/// 角色下拉与组下拉联动：组长/组员必须选组，总管/超管不选组
void connectRoleGroupCombo(QComboBox* roleCombo, QComboBox* groupCombo)
{
    const auto updateGroupEnabled = [roleCombo, groupCombo]() {
        const UserRole role = static_cast<UserRole>(roleCombo->currentData().toInt());
        const bool needGroup = (role == UserRole::Leader || role == UserRole::Member);
        groupCombo->setEnabled(needGroup);
        if (!needGroup) {
            groupCombo->setCurrentIndex(0);  // （无组）
        }
    };
    QObject::connect(roleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     groupCombo, [updateGroupEnabled](int) { updateGroupEnabled(); });
    updateGroupEnabled();
}

} // namespace

UserManagerDialog::UserManagerDialog(QWidget* parent)
    : BaseDialog(parent)
    , ui(new Ui::UserManagerDialog)
{
    ui->setupUi(this);
    resize(AppDimensions::Window::DialogSmallWidth, AppDimensions::Window::DialogSmallHeight);

    // 表格列配置（.ui 定义结构，行为属性在此细化）
    ui->userTable->setColumnCount(5);
    ui->userTable->setHorizontalHeaderLabels(
        {tr("用户名"), tr("显示名称"), tr("角色"), tr("所属组"), tr("最后登录")});
    ui->userTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->userTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->userTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->userTable->horizontalHeader()->setStretchLastSection(true);
    ui->userTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // 表格内容与表头居中
    UiHelper::centerTableWidget(ui->userTable);

    // 组管理入口仅 超管/总管 可见（组长无组管理权限）
    const User::Ptr current = UserSession::instance().currentUser();
    ui->btnManageGroup->setVisible(current && (current->isSuperAdmin() || current->isManager()));

    loadUsers();
}

UserManagerDialog::~UserManagerDialog()
{
    delete ui;
}

void UserManagerDialog::loadUsers()
{
    m_users = UserService::listAll();

    // 权限过滤：组长只能看到本组组员（总管/超管看到全部）
    const User::Ptr current = UserSession::instance().currentUser();
    if (current && current->isLeader()) {
        User::List filtered;
        for (const User::Ptr& u : m_users) {
            if (u->isMember() && u->groupId() > 0 && u->groupId() == current->groupId()) {
                filtered.append(u);
            }
        }
        m_users = filtered;
    }

    QHash<qint64, QString> groupNames;
    for (const Group::Ptr& g : GroupService::getAllGroups()) {
        groupNames.insert(g->id(), g->name());
    }

    ui->userTable->setRowCount(m_users.size());

    for (int row = 0; row < m_users.size(); ++row) {
        const User::Ptr& user = m_users[row];
        ui->userTable->setItem(row, 0, new QTableWidgetItem(user->username()));
        ui->userTable->setItem(row, 1, new QTableWidgetItem(user->displayNameOrUsername()));
        ui->userTable->setItem(row, 2, new QTableWidgetItem(user->roleName()));
        const QString groupText =
            user->groupId() > 0
                ? groupNames.value(user->groupId(), QString::number(user->groupId()))
                : tr("—");
        ui->userTable->setItem(row, 3, new QTableWidgetItem(groupText));
        ui->userTable->setItem(row, 4, new QTableWidgetItem(
            user->lastLoginAt().isValid()
                ? user->lastLoginAt().toString("yyyy-MM-dd HH:mm")
                : tr("从未登录")));
    }
}

User::Ptr UserManagerDialog::currentUser() const
{
    const int row = ui->userTable->currentRow();
    if (row >= 0 && row < m_users.size()) {
        return m_users[row];
    }
    return nullptr;
}

void UserManagerDialog::on_userTable_itemSelectionChanged()
{
    const bool hasSelection = currentUser() != nullptr;
    ui->btnEdit->setEnabled(hasSelection);
    ui->btnDelete->setEnabled(hasSelection);
    ui->btnResetPassword->setEnabled(hasSelection);
}

void UserManagerDialog::on_btnClose_clicked()
{
    accept();
}

void UserManagerDialog::on_btnAdd_clicked()
{
    const User::Ptr current = UserSession::instance().currentUser();
    const QList<UserRole> allowed = allowedRolesFor(current);
    const QList<Group::Ptr> groups = visibleGroups(current);
    if (allowed.isEmpty()) {
        UiHelper::warning(this, tr("无权限"), tr("您没有权限创建用户"));
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(tr("新建用户"));
    dlg.setMinimumWidth(320);

    QFormLayout* form = new QFormLayout(&dlg);
    QLineEdit* usernameEdit = new QLineEdit(&dlg);
    QLineEdit* displayNameEdit = new QLineEdit(&dlg);
    QLineEdit* passwordEdit = new QLineEdit(&dlg);
    // 初始密码明文显示（默认 123456，创建前可手动修改，便于告知新用户）
    passwordEdit->setText(QStringLiteral("123456"));
    passwordEdit->setPlaceholderText(tr("默认 123456，可修改"));
    QComboBox* roleCombo = new QComboBox(&dlg);
    QComboBox* groupCombo = new QComboBox(&dlg);

    for (UserRole r : allowed) {
        roleCombo->addItem(roleDisplayName(r), QVariant::fromValue(static_cast<int>(r)));
    }
    groupCombo->addItem(tr("（无组）"), QVariant::fromValue(-1));
    for (const Group::Ptr& g : groups) {
        groupCombo->addItem(g->name(), QVariant::fromValue(g->id()));
    }
    connectRoleGroupCombo(roleCombo, groupCombo);

    form->addRow(tr("用户名:"), usernameEdit);
    form->addRow(tr("显示名称:"), displayNameEdit);
    form->addRow(tr("初始密码:"), passwordEdit);
    form->addRow(tr("角色:"), roleCombo);
    form->addRow(tr("所属组:"), groupCombo);

    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    const QString username = usernameEdit->text().trimmed();
    if (username.isEmpty()) {
        UiHelper::warning(this, tr("错误"), tr("用户名不能为空"));
        return;
    }

    if (UserService::exists(username)) {
        UiHelper::warning(this, tr("错误"), tr("用户名已存在"));
        return;
    }

    const QString password = passwordEdit->text();
    if (password.length() < 6) {
        UiHelper::warning(this, tr("错误"), tr("密码至少6位"));
        return;
    }

    const UserRole role = static_cast<UserRole>(roleCombo->currentData().toInt());
    const qint64 groupId = groupCombo->currentData().toLongLong();
    if ((role == UserRole::Leader || role == UserRole::Member) && groupId <= 0) {
        UiHelper::warning(this, tr("错误"), tr("组长/组员必须选择所属组"));
        return;
    }

    User::Ptr user = User::create();
    user->setUsername(username);
    user->setDisplayName(displayNameEdit->text().trimmed());
    user->setPasswordHash(User::hashPassword(password));
    user->setRole(role);
    user->setGroupId(groupId);
    user->setMustChangePassword(false);  // v9.20.0 起新建用户不强制首次改密码
    user->setCreatedAt(QDateTime::currentDateTime());

    if (UserService::save(user)) {
        UiHelper::info(this, tr("成功"), tr("用户已创建"));
        loadUsers();
    } else {
        UiHelper::error(this, tr("错误"), tr("创建用户失败"));
    }
}

void UserManagerDialog::on_btnEdit_clicked()
{
    User::Ptr user = currentUser();
    if (!user) return;

    // 不能修改自己
    if (user->id() == UserSession::instance().userId()) {
        UiHelper::info(this, tr("提示"), tr("不能修改当前登录用户"));
        return;
    }

    const User::Ptr current = UserSession::instance().currentUser();
    if (!PermissionService::canManageUser(current, user)) {
        UiHelper::warning(this, tr("无权限"), tr("您没有权限修改该用户"));
        return;
    }

    const QList<UserRole> allowed = allowedRolesFor(current);
    const QList<Group::Ptr> groups = visibleGroups(current);
    if (allowed.isEmpty()) {
        UiHelper::warning(this, tr("无权限"), tr("您没有权限修改该用户"));
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(tr("编辑用户"));
    dlg.setMinimumWidth(320);

    QFormLayout* form = new QFormLayout(&dlg);
    QLineEdit* displayNameEdit = new QLineEdit(user->displayName(), &dlg);
    QComboBox* roleCombo = new QComboBox(&dlg);
    QComboBox* groupCombo = new QComboBox(&dlg);

    int roleIndex = 0;
    for (int i = 0; i < allowed.size(); ++i) {
        roleCombo->addItem(roleDisplayName(allowed[i]), QVariant::fromValue(static_cast<int>(allowed[i])));
        if (allowed[i] == user->role()) roleIndex = i;
    }
    roleCombo->setCurrentIndex(roleIndex);

    groupCombo->addItem(tr("（无组）"), QVariant::fromValue(-1));
    int groupIndex = 0;
    for (int i = 0; i < groups.size(); ++i) {
        groupCombo->addItem(groups[i]->name(), QVariant::fromValue(groups[i]->id()));
        if (groups[i]->id() == user->groupId()) groupIndex = i + 1;
    }
    groupCombo->setCurrentIndex(groupIndex);
    connectRoleGroupCombo(roleCombo, groupCombo);

    form->addRow(tr("用户名:"), new QLabel(user->username(), &dlg));
    form->addRow(tr("显示名称:"), displayNameEdit);
    form->addRow(tr("角色:"), roleCombo);
    form->addRow(tr("所属组:"), groupCombo);

    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    const UserRole role = static_cast<UserRole>(roleCombo->currentData().toInt());
    const qint64 groupId = groupCombo->currentData().toLongLong();
    if ((role == UserRole::Leader || role == UserRole::Member) && groupId <= 0) {
        UiHelper::warning(this, tr("错误"), tr("组长/组员必须选择所属组"));
        return;
    }

    user->setDisplayName(displayNameEdit->text().trimmed());
    user->setRole(role);
    user->setGroupId(groupId);

    if (UserService::save(user)) {
        UiHelper::info(this, tr("成功"), tr("用户已更新"));
        loadUsers();
    } else {
        UiHelper::error(this, tr("错误"), tr("更新用户失败"));
    }
}

void UserManagerDialog::on_btnDelete_clicked()
{
    User::Ptr user = currentUser();
    if (!user) return;

    if (user->id() == UserSession::instance().userId()) {
        UiHelper::warning(this, tr("错误"), tr("不能删除当前登录用户"));
        return;
    }

    const User::Ptr current = UserSession::instance().currentUser();
    if (!PermissionService::canDeleteUser(current)) {
        UiHelper::warning(this, tr("无权限"), tr("只有超级管理员可以删除用户"));
        return;
    }

    if (!UiHelper::confirm(this,
                           tr("确认删除"),
                           tr("确定要删除用户「%1」吗？\n该用户创建的数据不会被删除。")
                               .arg(user->username()))) return;

    // 若被删除用户是组长，先清除其组组长身份
    if (user->isLeader()) {
        for (const Group::Ptr& g : GroupService::getAllGroups()) {
            if (g->leaderId() == user->id()) {
                GroupService::updateGroup(g->id(), g->name(), g->description(), -1);
            }
        }
    }

    if (UserService::remove(user->id())) {
        UiHelper::info(this, tr("成功"), tr("用户已删除"));
        loadUsers();
    } else {
        UiHelper::error(this, tr("错误"), tr("删除用户失败"));
    }
}

void UserManagerDialog::on_btnResetPassword_clicked()
{
    User::Ptr user = currentUser();
    if (!user) return;

    const User::Ptr current = UserSession::instance().currentUser();
    if (!PermissionService::canManageUser(current, user)) {
        UiHelper::warning(this, tr("无权限"), tr("您没有权限重置该用户的密码"));
        return;
    }

    if (!UiHelper::confirm(this,
                           tr("重置密码"),
                           tr("确定要将用户「%1」的密码重置为 123456 吗？\n重置后该用户首次登录必须修改密码。")
                               .arg(user->username()))) return;

    if (UserService::resetPassword(user->id())) {
        UiHelper::info(this, tr("成功"), tr("密码已重置为 123456"));
        loadUsers();
    } else {
        UiHelper::error(this, tr("错误"), tr("重置密码失败"));
    }
}

void UserManagerDialog::on_btnManageGroup_clicked()
{
    GroupManagerDialog dialog(this);
    dialog.exec();
    loadUsers();  // 组归属可能变化，刷新列表
}

void UserManagerDialog::on_btnRefresh_clicked()
{
    loadUsers();
}
