/**
 * @file LoginDialog.cpp
 * @brief 用户登录对话框实现文件
 *
 * UI 布局由 LoginDialog.ui 可视化设计，本文件负责登录验证逻辑。
 * 用户信息使用 QSettings 持久化存储，默认账号 admin/admin123。
 */

#include "LoginDialog.h"
#include "ui_LoginDialog.h"
#include "core/utils/AppDimensions.h"

#include <QSettings>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QTimer>

// ============================================================================
// 构造与析构
// ============================================================================

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
    , m_username()
{
    // 加载 .ui 文件中设计的布局
    ui->setupUi(this);

    // 去掉对话框标题栏的帮助按钮
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // 槽函数通过 uic 自动连接（on_<objectName>_<signalName> 命名约定）

    // 确保默认用户存在
    ensureDefaultUser();
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

    // 验证凭据
    if (verifyCredentials(username, password)) {
        m_username = username;

        // 保存当前登录用户
        QSettings settings;
        settings.setValue("user/currentUsername", username);
        settings.sync();

        accept();  // 登录成功，关闭对话框
    } else {
        showError(tr("用户名或密码错误，请重试"));
        ui->passwordEdit->selectAll();
        ui->passwordEdit->setFocus();
    }
}

void LoginDialog::on_cancelButton_clicked()
{
    reject();  // 取消登录
}

void LoginDialog::on_usernameEdit_returnPressed()
{
    // 用户名输入完成，焦点移到密码框
    ui->passwordEdit->setFocus();
}

void LoginDialog::on_passwordEdit_returnPressed()
{
    // 密码输入完成，触发登录
    on_loginButton_clicked();
}

bool LoginDialog::verifyCredentials(const QString& username, const QString& password)
{
    QSettings settings;

    // 读取存储的密码哈希
    const QString storedHash = settings.value(
        QString("users/%1/password").arg(username), QString()).toString();

    if (storedHash.isEmpty()) {
        return false;  // 用户不存在
    }

    // 计算输入密码的哈希值
    const QString inputHash = hashPassword(password);

    // 比较哈希值
    return (storedHash == inputHash);
}

// ============================================================================
// 辅助方法
// ============================================================================

void LoginDialog::showError(const QString& message)
{
    ui->errorLabel->setText(message);

    // 3秒后自动清除错误信息
    QTimer::singleShot(AppDimensions::Delay::ErrorClear, ui->errorLabel, [this]() {
        ui->errorLabel->clear();
    });
}

void LoginDialog::ensureDefaultUser()
{
    QSettings settings;

    // 检查默认用户是否存在
    const QString adminHash = settings.value("users/admin/password", QString()).toString();
    if (adminHash.isEmpty()) {
        // 创建默认管理员账号
        settings.setValue("users/admin/password", hashPassword("admin123"));
        settings.setValue("users/admin/role", "admin");
        settings.setValue("users/admin/displayName", tr("管理员"));
        settings.sync();
    }
}

QString LoginDialog::hashPassword(const QString& password)
{
    // 使用 SHA-256 哈希密码（简单加盐）
    const QString salted = "ExperimentReportTool_" + password + "_salt";
    const QByteArray hash = QCryptographicHash::hash(
        salted.toUtf8(), QCryptographicHash::Sha256);
    return QString(hash.toHex());
}
