/**
 * @file UserManagerDialog.cpp
 * @brief 用户管理对话框实现文件
 */

#include "UserManagerDialog.h"
#include "data/repositories/UserRepository.h"
#include "core/utils/UserSession.h"
#include "core/utils/AppTheme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QLabel>
#include <QFormLayout>

UserManagerDialog::UserManagerDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("用户管理"));
    resize(600, 400);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setupUi();
    loadUsers();
}

UserManagerDialog::~UserManagerDialog() = default;

void UserManagerDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 用户表格
    m_userTable = new QTableWidget(this);
    m_userTable->setColumnCount(5);
    m_userTable->setHorizontalHeaderLabels(
        {tr("用户名"), tr("显示名称"), tr("角色"), tr("最后登录"), tr("需改密码")});
    m_userTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_userTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_userTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_userTable->horizontalHeader()->setStretchLastSection(true);
    m_userTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLayout->addWidget(m_userTable);

    connect(m_userTable, &QTableWidget::itemSelectionChanged,
            this, &UserManagerDialog::onItemSelectionChanged);

    // 按钮栏
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_btnAdd = new QPushButton(tr("新建用户"), this);
    m_btnEdit = new QPushButton(tr("编辑"), this);
    m_btnDelete = new QPushButton(tr("删除"), this);
    m_btnResetPassword = new QPushButton(tr("重置密码"), this);
    m_btnRefresh = new QPushButton(tr("刷新"), this);
    m_btnClose = new QPushButton(tr("关闭"), this);

    btnLayout->addWidget(m_btnAdd);
    btnLayout->addWidget(m_btnEdit);
    btnLayout->addWidget(m_btnDelete);
    btnLayout->addWidget(m_btnResetPassword);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnRefresh);
    btnLayout->addWidget(m_btnClose);

    mainLayout->addLayout(btnLayout);

    connect(m_btnAdd, &QPushButton::clicked, this, &UserManagerDialog::onAddUser);
    connect(m_btnEdit, &QPushButton::clicked, this, &UserManagerDialog::onEditUser);
    connect(m_btnDelete, &QPushButton::clicked, this, &UserManagerDialog::onDeleteUser);
    connect(m_btnResetPassword, &QPushButton::clicked, this, &UserManagerDialog::onResetPassword);
    connect(m_btnRefresh, &QPushButton::clicked, this, &UserManagerDialog::onRefresh);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);

    onItemSelectionChanged();
}

void UserManagerDialog::loadUsers()
{
    m_users = UserRepository::findAll();
    m_userTable->setRowCount(m_users.size());

    for (int row = 0; row < m_users.size(); ++row) {
        const User::Ptr& user = m_users[row];
        m_userTable->setItem(row, 0, new QTableWidgetItem(user->username()));
        m_userTable->setItem(row, 1, new QTableWidgetItem(user->displayNameOrUsername()));
        m_userTable->setItem(row, 2, new QTableWidgetItem(user->roleName()));
        m_userTable->setItem(row, 3, new QTableWidgetItem(
            user->lastLoginAt().isValid()
                ? user->lastLoginAt().toString("yyyy-MM-dd HH:mm")
                : tr("从未登录")));
        m_userTable->setItem(row, 4, new QTableWidgetItem(
            user->mustChangePassword() ? tr("是") : tr("否")));
    }
}

User::Ptr UserManagerDialog::currentUser() const
{
    const int row = m_userTable->currentRow();
    if (row >= 0 && row < m_users.size()) {
        return m_users[row];
    }
    return nullptr;
}

void UserManagerDialog::onItemSelectionChanged()
{
    const bool hasSelection = currentUser() != nullptr;
    m_btnEdit->setEnabled(hasSelection);
    m_btnDelete->setEnabled(hasSelection);
    m_btnResetPassword->setEnabled(hasSelection);
}

void UserManagerDialog::onAddUser()
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
        QMessageBox::warning(this, tr("错误"), tr("用户名不能为空"));
        return;
    }

    if (UserRepository::exists(username)) {
        QMessageBox::warning(this, tr("错误"), tr("用户名已存在"));
        return;
    }

    const QString password = passwordEdit->text();
    if (password.length() < 6) {
        QMessageBox::warning(this, tr("错误"), tr("密码至少6位"));
        return;
    }

    User::Ptr user = User::create();
    user->setUsername(username);
    user->setDisplayName(displayNameEdit->text().trimmed());
    user->setPasswordHash(User::hashPassword(password));
    user->setRole(static_cast<UserRole>(roleCombo->currentData().toInt()));
    user->setMustChangePassword(true);  // 新建用户首次登录必须改密码
    user->setCreatedAt(QDateTime::currentDateTime());

    if (UserRepository::save(user)) {
        QMessageBox::information(this, tr("成功"), tr("用户已创建"));
        loadUsers();
    } else {
        QMessageBox::critical(this, tr("错误"), tr("创建用户失败"));
    }
}

void UserManagerDialog::onEditUser()
{
    User::Ptr user = currentUser();
    if (!user) return;

    // 不能删除/修改自己
    if (user->id() == UserSession::instance().userId()) {
        QMessageBox::information(this, tr("提示"), tr("不能修改当前登录用户"));
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

    if (UserRepository::save(user)) {
        QMessageBox::information(this, tr("成功"), tr("用户已更新"));
        loadUsers();
    } else {
        QMessageBox::critical(this, tr("错误"), tr("更新用户失败"));
    }
}

void UserManagerDialog::onDeleteUser()
{
    User::Ptr user = currentUser();
    if (!user) return;

    if (user->id() == UserSession::instance().userId()) {
        QMessageBox::warning(this, tr("错误"), tr("不能删除当前登录用户"));
        return;
    }

    const auto ret = QMessageBox::warning(
        this, tr("确认删除"),
        tr("确定要删除用户「%1」吗？\n该用户创建的数据不会被删除。")
            .arg(user->username()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret != QMessageBox::Yes) return;

    if (UserRepository::remove(user->id())) {
        QMessageBox::information(this, tr("成功"), tr("用户已删除"));
        loadUsers();
    } else {
        QMessageBox::critical(this, tr("错误"), tr("删除用户失败"));
    }
}

void UserManagerDialog::onResetPassword()
{
    User::Ptr user = currentUser();
    if (!user) return;

    const auto ret = QMessageBox::question(
        this, tr("重置密码"),
        tr("确定要将用户「%1」的密码重置为 123456 吗？\n重置后该用户首次登录必须修改密码。")
            .arg(user->username()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret != QMessageBox::Yes) return;

    if (UserRepository::resetPassword(user->id())) {
        QMessageBox::information(this, tr("成功"), tr("密码已重置为 123456"));
        loadUsers();
    } else {
        QMessageBox::critical(this, tr("错误"), tr("重置密码失败"));
    }
}

void UserManagerDialog::onRefresh()
{
    loadUsers();
}
