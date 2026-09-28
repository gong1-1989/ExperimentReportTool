/**
 * @file UserManagerDialog.cpp
 * @brief 用户管理对话框实现文件
 */

#include "UserManagerDialog.h"
#include "ui_UserManagerDialog.h"
#include "service/UserService.h"
#include "data/repositories/UserRepository.h"
#include "core/utils/UserSession.h"
#include "core/utils/AppDimensions.h"

#include <QHeaderView>
#include "ui/UiHelper.h"
#include <QComboBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QLabel>
#include <QFormLayout>

UserManagerDialog::UserManagerDialog(QWidget* parent)
    : BaseDialog(parent)
    , ui(new Ui::UserManagerDialog)
{
    ui->setupUi(this);
    resize(AppDimensions::Window::DialogSmallWidth, AppDimensions::Window::DialogSmallHeight);

    // 表格列配置（.ui 定义结构，行为属性在此细化）
    ui->userTable->setColumnCount(5);
    ui->userTable->setHorizontalHeaderLabels(
        {tr("用户名"), tr("显示名称"), tr("角色"), tr("最后登录"), tr("需改密码")});
    ui->userTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->userTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->userTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->userTable->horizontalHeader()->setStretchLastSection(true);
    ui->userTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    loadUsers();
}

UserManagerDialog::~UserManagerDialog()
{
    delete ui;
}

void UserManagerDialog::loadUsers()
{
    m_users = UserService::listAll();
    ui->userTable->setRowCount(m_users.size());

    for (int row = 0; row < m_users.size(); ++row) {
        const User::Ptr& user = m_users[row];
        ui->userTable->setItem(row, 0, new QTableWidgetItem(user->username()));
        ui->userTable->setItem(row, 1, new QTableWidgetItem(user->displayNameOrUsername()));
        ui->userTable->setItem(row, 2, new QTableWidgetItem(user->roleName()));
        ui->userTable->setItem(row, 3, new QTableWidgetItem(
            user->lastLoginAt().isValid()
                ? user->lastLoginAt().toString("yyyy-MM-dd HH:mm")
                : tr("从未登录")));
        ui->userTable->setItem(row, 4, new QTableWidgetItem(
            user->mustChangePassword() ? tr("是") : tr("否")));
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
    // 简单的新建用户对话框
    QDialog dlg(this);
    dlg.setWindowTitle(tr("新建用户"));
    dlg.setMinimumWidth(300);

    QFormLayout* form = new QFormLayout(&dlg);
    QLineEdit* usernameEdit = new QLineEdit(&dlg);
    QLineEdit* displayNameEdit = new QLineEdit(&dlg);
    QLineEdit* passwordEdit = new QLineEdit(&dlg);
    passwordEdit->setEchoMode(QLineEdit::Password);
    QComboBox* roleCombo = new QComboBox(&dlg);
    roleCombo->addItem(tr("普通用户"), QVariant::fromValue(static_cast<int>(UserRole::User)));
    roleCombo->addItem(tr("管理员"), QVariant::fromValue(static_cast<int>(UserRole::Admin)));

    form->addRow(tr("用户名:"), usernameEdit);
    form->addRow(tr("显示名称:"), displayNameEdit);
    form->addRow(tr("初始密码:"), passwordEdit);
    form->addRow(tr("角色:"), roleCombo);

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

    User::Ptr user = User::create();
    user->setUsername(username);
    user->setDisplayName(displayNameEdit->text().trimmed());
    user->setPasswordHash(User::hashPassword(password));
    user->setRole(static_cast<UserRole>(roleCombo->currentData().toInt()));
    user->setMustChangePassword(true);  // 新建用户首次登录必须改密码
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

    // 不能删除/修改自己
    if (user->id() == UserSession::instance().userId()) {
        UiHelper::info(this, tr("提示"), tr("不能修改当前登录用户"));
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(tr("编辑用户"));
    dlg.setMinimumWidth(300);

    QFormLayout* form = new QFormLayout(&dlg);
    QLineEdit* displayNameEdit = new QLineEdit(user->displayName(), &dlg);
    QComboBox* roleCombo = new QComboBox(&dlg);
    roleCombo->addItem(tr("普通用户"), QVariant::fromValue(static_cast<int>(UserRole::User)));
    roleCombo->addItem(tr("管理员"), QVariant::fromValue(static_cast<int>(UserRole::Admin)));
    roleCombo->setCurrentIndex(user->isAdmin() ? 1 : 0);

    form->addRow(tr("用户名:"), new QLabel(user->username(), &dlg));
    form->addRow(tr("显示名称:"), displayNameEdit);
    form->addRow(tr("角色:"), roleCombo);

    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    user->setDisplayName(displayNameEdit->text().trimmed());
    user->setRole(static_cast<UserRole>(roleCombo->currentData().toInt()));

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

    if (!UiHelper::confirm(this,
                           tr("确认删除"),
                           tr("确定要删除用户「%1」吗？\n该用户创建的数据不会被删除。")
                               .arg(user->username()))) return;

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

void UserManagerDialog::on_btnRefresh_clicked()
{
    loadUsers();
}
