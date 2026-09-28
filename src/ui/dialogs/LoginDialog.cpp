/**
 * @file LoginDialog.cpp
 * @brief 用户登录对话框实现文件
 *
 * UI 布局由 LoginDialog.ui 可视化设计，本文件负责登录验证逻辑。
 * 用户信息存储在 SQLite 数据库的 users 表中。
 */

#include "LoginDialog.h"
#include "ui_LoginDialog.h"
#include "core/utils/AppDimensions.h"
#include "core/utils/UserSession.h"
#include "service/UserService.h"
#include "data/repositories/UserRepository.h"
#include "ui/dialogs/ChangePasswordDialog.h"

#include <QMessageBox>
#include "ui/UiHelper.h"
#include <QTimer>

// ============================================================================
// 构造与析构
// ============================================================================

LoginDialog::LoginDialog(QWidget* parent)
    : BaseDialog(parent)
    , ui(new Ui::LoginDialog)
    , m_username()
{
    ui->setupUi(this);

    // 去掉对话框标题栏的帮助按钮
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

// ============================================================================
// 登录验证
// ============================================================================

void LoginDialog::on_loginButton_clicked()
{
    const QString username = ui->usernameEdit->text().trimmed();
    const QString password = ui->passwordEdit->text();

    // 输入校验
    if (username.isEmpty()) {
        showError(tr("请输入用户名"));
        ui->usernameEdit->setFocus();
        return;
    }

    if (password.isEmpty()) {
        showError(tr("请输入密码"));
        ui->passwordEdit->setFocus();
        return;
    }

    // 从数据库验证用户
    User::Ptr user = UserService::authenticate(username, password);
    if (!user) {
        showError(tr("用户名或密码错误，请重试"));
        ui->passwordEdit->selectAll();
        ui->passwordEdit->setFocus();
        return;
    }

    // 登录成功
    m_username = username;

    // 更新最后登录时间
    UserService::updateLastLogin(user->id());

    // 保存到会话
    UserSession::instance().setCurrentUser(user);

    // 检查是否需要首次改密码
    if (user->mustChangePassword()) {
        ChangePasswordDialog dlg(true, this);
        if (dlg.exec() == QDialog::Accepted) {
            if (UserService::changePassword(user->id(), dlg.newPassword())) {
                user->setMustChangePassword(false);
                UiHelper::info(this, tr("成功"), tr("密码修改成功，请重新登录"));
                // 清空密码框，让用户重新登录
                ui->passwordEdit->clear();
                ui->passwordEdit->setFocus();
                return;
            } else {
                UiHelper::warning(this, tr("错误"), tr("密码修改失败，请重试"));
                return;
            }
        }
        // 用户取消改密码，不允许登录
        return;
    }

    accept();  // 登录成功，关闭对话框
}

void LoginDialog::on_cancelButton_clicked()
{
    reject();
}

void LoginDialog::on_usernameEdit_returnPressed()
{
    ui->passwordEdit->setFocus();
}

void LoginDialog::on_passwordEdit_returnPressed()
{
    on_loginButton_clicked();
}

// ============================================================================
// 辅助方法
// ============================================================================

void LoginDialog::showError(const QString& message)
{
    ui->errorLabel->setText(message);

    QTimer::singleShot(AppDimensions::Delay::ErrorClear, ui->errorLabel, [this]() {
        ui->errorLabel->clear();
    });
}
