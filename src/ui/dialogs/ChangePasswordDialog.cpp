/**
 * @file ChangePasswordDialog.cpp
 * @brief 修改密码对话框实现文件
 */

#include "ChangePasswordDialog.h"
#include "ui_ChangePasswordDialog.h"
#include "ui/UiHelper.h"
#include "core/utils/AppDimensions.h"

// UI 定义见 ChangePasswordDialog.ui（uic 生成 ui_ChangePasswordDialog.h）

ChangePasswordDialog::ChangePasswordDialog(QWidget* parent)
    : BaseDialog(parent)
    , ui(new Ui::ChangePasswordDialog)
{
    ui->setupUi(this);
    ui->titleLabel->setText(tr("修改密码"));

    // 按钮点击由 .ui 自动连接（on_okButton_clicked / on_cancelButton_clicked）
}

ChangePasswordDialog::~ChangePasswordDialog()
{
    delete ui;
}

QString ChangePasswordDialog::oldPassword() const
{
    return ui->oldPasswordEdit->text();
}

void ChangePasswordDialog::on_okButton_clicked()
{
    ui->errorLabel->clear();

    // 修改密码需验证原密码
    if (ui->oldPasswordEdit->text().isEmpty()) {
        ui->errorLabel->setText(tr("请输入原密码"));
        return;
    }
    // 原密码验证由调用方处理，这里只做新密码验证

    const QString newPwd = ui->newPasswordEdit->text();
    const QString confirmPwd = ui->confirmPasswordEdit->text();

    // 验证新密码
    QString errorMsg;
    if (!validatePassword(newPwd, &errorMsg)) {
        ui->errorLabel->setText(errorMsg);
        return;
    }

    if (newPwd != confirmPwd) {
        ui->errorLabel->setText(tr("两次输入的密码不一致"));
        return;
    }

    m_newPassword = newPwd;
    accept();
}

void ChangePasswordDialog::on_cancelButton_clicked()
{
    reject();
}

bool ChangePasswordDialog::validatePassword(const QString& password, QString* errorMsg) const
{
    if (password.length() < 6) {
        if (errorMsg) *errorMsg = tr("密码长度至少6位");
        return false;
    }

    bool hasLetter = false;
    bool hasDigit = false;
    for (const QChar& c : password) {
        if (c.isLetter()) hasLetter = true;
        if (c.isDigit()) hasDigit = true;
    }

    if (!hasLetter || !hasDigit) {
        if (errorMsg) *errorMsg = tr("密码必须包含字母和数字");
        return false;
    }

    return true;
}
