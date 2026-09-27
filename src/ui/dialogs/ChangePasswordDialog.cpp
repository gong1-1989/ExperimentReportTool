/**
 * @file ChangePasswordDialog.cpp
 * @brief 修改密码对话框实现文件
 */

#include "ChangePasswordDialog.h"
#include "core/utils/AppTheme.h"
#include "core/utils/AppDimensions.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDialogButtonBox>

// 由于没有 .ui 文件，用代码创建 UI（简单对话框）
namespace Ui {
class ChangePasswordDialog {
public:
    QLabel* titleLabel;
    QLabel* oldPasswordLabel;
    QLineEdit* oldPasswordEdit;
    QLabel* newPasswordLabel;
    QLineEdit* newPasswordEdit;
    QLabel* confirmPasswordLabel;
    QLineEdit* confirmPasswordEdit;
    QLabel* errorLabel;
    QPushButton* okButton;
    QPushButton* cancelButton;

    void setupUi(QDialog* dialog) {
        dialog->setWindowTitle(QObject::tr("修改密码"));
        dialog->setMinimumWidth(360);

        QVBoxLayout* mainLayout = new QVBoxLayout(dialog);
        mainLayout->setSpacing(12);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        titleLabel = new QLabel(dialog);
        titleLabel->setStyleSheet(QString(
            "font-size: %1px; font-weight: bold; color: %2;")
            .arg(AppTheme::FontSize::Large)
            .arg(AppTheme::Color::TextPrimary));
        mainLayout->addWidget(titleLabel);

        // 旧密码（首次登录时不显示）
        oldPasswordLabel = new QLabel(QObject::tr("原密码:"), dialog);
        oldPasswordEdit = new QLineEdit(dialog);
        oldPasswordEdit->setEchoMode(QLineEdit::Password);
        oldPasswordEdit->setPlaceholderText(QObject::tr("请输入原密码"));
        mainLayout->addWidget(oldPasswordLabel);
        mainLayout->addWidget(oldPasswordEdit);

        // 新密码
        newPasswordLabel = new QLabel(QObject::tr("新密码:"), dialog);
        newPasswordEdit = new QLineEdit(dialog);
        newPasswordEdit->setEchoMode(QLineEdit::Password);
        newPasswordEdit->setPlaceholderText(QObject::tr("至少6位，包含字母和数字"));
        mainLayout->addWidget(newPasswordLabel);
        mainLayout->addWidget(newPasswordEdit);

        // 确认密码
        confirmPasswordLabel = new QLabel(QObject::tr("确认新密码:"), dialog);
        confirmPasswordEdit = new QLineEdit(dialog);
        confirmPasswordEdit->setEchoMode(QLineEdit::Password);
        confirmPasswordEdit->setPlaceholderText(QObject::tr("再次输入新密码"));
        mainLayout->addWidget(confirmPasswordLabel);
        mainLayout->addWidget(confirmPasswordEdit);

        // 错误提示
        errorLabel = new QLabel(dialog);
        errorLabel->setStyleSheet(QString("color: %1; font-size: %2px;")
            .arg(AppTheme::Color::Danger)
            .arg(AppTheme::FontSize::Small));
        errorLabel->setWordWrap(true);
        mainLayout->addWidget(errorLabel);

        mainLayout->addStretch();

        // 按钮
        QHBoxLayout* btnLayout = new QHBoxLayout();
        btnLayout->addStretch();
        okButton = new QPushButton(QObject::tr("确定"), dialog);
        okButton->setObjectName("okButton");
        cancelButton = new QPushButton(QObject::tr("取消"), dialog);
        cancelButton->setObjectName("cancelButton");
        btnLayout->addWidget(okButton);
        btnLayout->addWidget(cancelButton);
        mainLayout->addLayout(btnLayout);
    }
};
}

ChangePasswordDialog::ChangePasswordDialog(bool isFirstLogin, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ChangePasswordDialog)
    , m_isFirstLogin(isFirstLogin)
{
    ui->setupUi(this);

    // 去掉帮助按钮
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    if (m_isFirstLogin) {
        ui->titleLabel->setText(tr("首次登录，请修改密码"));
        ui->oldPasswordLabel->setVisible(false);
        ui->oldPasswordEdit->setVisible(false);
    } else {
        ui->titleLabel->setText(tr("修改密码"));
    }

    // 手动连接信号（因为没有 .ui 文件，uic 不会自动连接）
    connect(ui->okButton, &QPushButton::clicked,
            this, &ChangePasswordDialog::on_okButton_clicked);
    connect(ui->cancelButton, &QPushButton::clicked,
            this, &ChangePasswordDialog::on_cancelButton_clicked);
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

    // 非首次登录需要验证原密码
    if (!m_isFirstLogin) {
        if (ui->oldPasswordEdit->text().isEmpty()) {
            ui->errorLabel->setText(tr("请输入原密码"));
            return;
        }
        // 原密码验证由调用方处理，这里只做新密码验证
    }

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
    if (m_isFirstLogin) {
        // 首次登录强制改密码，取消则退出
        QMessageBox::warning(this, tr("提示"),
            tr("首次登录必须修改密码，否则无法使用系统。"));
        return;
    }
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
