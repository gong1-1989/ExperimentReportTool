/**
 * @file SettingsDialog.cpp
 * @brief 设置对话框实现文件
 *
 * UI 布局由 SettingsDialog.ui 可视化设计，本文件只负责业务逻辑。
 * 配置读写通过 AppConfig 统一管理，支持 config.ini 配置文件。
 *
 * 配置项分为五个标签页：
 * - 常规：自动保存、最大版本数、默认导出格式、默认导出路径
 * - 外观：主题颜色（主色/成功/警告/危险）、全局字体、基础字号
 * - 编辑器：编辑器默认字体、字号
 * - 界面：主窗口尺寸、记住窗口大小、状态栏显示、消息时长
 * - 数据：数据库路径查看
 */

#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"
#include "core/utils/AppConfig.h"
#include "core/utils/AppTheme.h"
#include "core/utils/AppDimensions.h"

#include <QMessageBox>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QFont>
#include <QColorDialog>
#include <QFileDialog>

// ============================================================================
// 构造与析构
// ============================================================================

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    // 加载 .ui 文件中设计的布局
    ui->setupUi(this);

    // 槽函数通过 uic 自动连接（on_<objectName>_<signalName> 命名约定）

    // 加载当前设置
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

// ============================================================================
// 设置加载与保存
// ============================================================================

void SettingsDialog::loadSettings()
{
    AppConfig& config = AppConfig::instance();

    // ---- 常规设置 ----
    ui->autoSaveCheck->setChecked(config.autoSaveEnabled());
    // AppConfig 中自动保存间隔为毫秒，转换为分钟显示
    ui->autoSaveIntervalSpin->setValue(config.autoSaveInterval() / 60000);
    ui->maxVersionsSpin->setValue(config.maxVersions());

    const QString exportFormat = config.defaultExportFormat();
    const int idx = ui->exportFormatCombo->findData(exportFormat);
    ui->exportFormatCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    ui->exportPathEdit->setText(config.defaultExportPath());

    // ---- 外观设置 ----
    ui->primaryColorEdit->setText(config.primaryColor().name());
    ui->successColorEdit->setText(config.successColor().name());
    ui->warningColorEdit->setText(config.warningColor().name());
    ui->dangerColorEdit->setText(config.dangerColor().name());
    ui->globalFontCombo->setCurrentFont(QFont(config.fontFamily()));
    ui->baseFontSizeSpin->setValue(config.baseFontSize());

    // ---- 编辑器设置 ----
    ui->fontCombo->setCurrentFont(QFont(config.editorFontFamily()));
    ui->fontSizeSpin->setValue(config.editorFontSize());

    // ---- 界面设置 ----
    ui->windowWidthSpin->setValue(config.mainWindowWidth());
    ui->windowHeightSpin->setValue(config.mainWindowHeight());
    ui->rememberWindowSizeCheck->setChecked(config.rememberWindowSize());
    ui->showStatusBarCheck->setChecked(config.showStatusBar());
    ui->statusMessageDurationSpin->setValue(config.statusMessageDuration());

    // ---- 数据设置 ----
    ui->dbPathEdit->setText(databasePath());
}

void SettingsDialog::saveSettings()
{
    AppConfig& config = AppConfig::instance();

    // ---- 常规设置 ----
    config.setAutoSaveEnabled(ui->autoSaveCheck->isChecked());
    // 分钟转换为毫秒
    config.setAutoSaveInterval(ui->autoSaveIntervalSpin->value() * 60000);
    config.setMaxVersions(ui->maxVersionsSpin->value());
    config.setDefaultExportFormat(ui->exportFormatCombo->currentData().toString());
    config.setDefaultExportPath(ui->exportPathEdit->text().trimmed());

    // ---- 外观设置 ----
    config.setPrimaryColor(QColor(ui->primaryColorEdit->text().trimmed()));
    config.setSuccessColor(QColor(ui->successColorEdit->text().trimmed()));
    config.setWarningColor(QColor(ui->warningColorEdit->text().trimmed()));
    config.setDangerColor(QColor(ui->dangerColorEdit->text().trimmed()));
    config.setFontFamily(ui->globalFontCombo->currentFont().family());
    config.setBaseFontSize(ui->baseFontSizeSpin->value());

    // ---- 编辑器设置 ----
    config.setEditorFontFamily(ui->fontCombo->currentFont().family());
    config.setEditorFontSize(ui->fontSizeSpin->value());

    // ---- 界面设置 ----
    config.setMainWindowWidth(ui->windowWidthSpin->value());
    config.setMainWindowHeight(ui->windowHeightSpin->value());
    config.setRememberWindowSize(ui->rememberWindowSizeCheck->isChecked());
    config.setShowStatusBar(ui->showStatusBarCheck->isChecked());
    config.setStatusMessageDuration(ui->statusMessageDurationSpin->value());

    // 保存到 config.ini
    config.save();
}

void SettingsDialog::applySettings()
{
    saveSettings();
    // 这里可以发出信号通知主窗口应用新设置
    // 目前设置在下次新建报告或导出时生效
    // 外观设置需要重启应用后生效
}

// ============================================================================
// 槽函数
// ============================================================================

void SettingsDialog::on_okButton_clicked()
{
    saveSettings();
    accept();
}

void SettingsDialog::on_applyButton_clicked()
{
    applySettings();
    QMessageBox::information(this, tr("设置"),
        tr("设置已应用并保存。\n外观和窗口设置将在重启应用后生效。"));
}

void SettingsDialog::on_cancelButton_clicked()
{
    reject();
}

void SettingsDialog::on_resetButton_clicked()
{
    const QMessageBox::StandardButton ret = QMessageBox::question(
        this, tr("恢复默认"),
        tr("确定要将所有设置恢复为默认值吗？"),
        QMessageBox::Yes | QMessageBox::No);

    if (ret != QMessageBox::Yes) return;

    // ---- 常规设置 ----
    ui->autoSaveCheck->setChecked(true);
    ui->autoSaveIntervalSpin->setValue(AppDimensions::AutoSave::IntervalMs / 60000);
    ui->maxVersionsSpin->setValue(AppDimensions::AutoSave::MaxVersions);
    ui->exportFormatCombo->setCurrentIndex(0);
    ui->exportPathEdit->setText(QDir::homePath());

    // ---- 外观设置 ----
    ui->primaryColorEdit->setText(AppTheme::Color::Primary);
    ui->successColorEdit->setText(AppTheme::Color::Success);
    ui->warningColorEdit->setText(AppTheme::Color::Warning);
    ui->dangerColorEdit->setText(AppTheme::Color::Danger);
    ui->globalFontCombo->setCurrentFont(QFont("Microsoft YaHei"));
    ui->baseFontSizeSpin->setValue(AppTheme::FontSize::Small);

    // ---- 编辑器设置 ----
    ui->fontCombo->setCurrentFont(QFont("Microsoft YaHei"));
    ui->fontSizeSpin->setValue(AppTheme::FontSize::Normal);

    // ---- 界面设置 ----
    ui->windowWidthSpin->setValue(AppDimensions::Window::MainWidth);
    ui->windowHeightSpin->setValue(AppDimensions::Window::MainHeight);
    ui->rememberWindowSizeCheck->setChecked(true);
    ui->showStatusBarCheck->setChecked(true);
    ui->statusMessageDurationSpin->setValue(AppDimensions::Delay::StatusMessage);
}

// ============================================================================
// 颜色选择按钮槽函数
// ============================================================================

void SettingsDialog::on_primaryColorBtn_clicked()
{
    const QColor color = QColorDialog::getColor(
        QColor(ui->primaryColorEdit->text()), this, tr("选择主色调"));
    if (color.isValid()) {
        ui->primaryColorEdit->setText(color.name());
    }
}

void SettingsDialog::on_successColorBtn_clicked()
{
    const QColor color = QColorDialog::getColor(
        QColor(ui->successColorEdit->text()), this, tr("选择成功色"));
    if (color.isValid()) {
        ui->successColorEdit->setText(color.name());
    }
}

void SettingsDialog::on_warningColorBtn_clicked()
{
    const QColor color = QColorDialog::getColor(
        QColor(ui->warningColorEdit->text()), this, tr("选择警告色"));
    if (color.isValid()) {
        ui->warningColorEdit->setText(color.name());
    }
}

void SettingsDialog::on_dangerColorBtn_clicked()
{
    const QColor color = QColorDialog::getColor(
        QColor(ui->dangerColorEdit->text()), this, tr("选择危险色"));
    if (color.isValid()) {
        ui->dangerColorEdit->setText(color.name());
    }
}

// ============================================================================
// 导出路径浏览按钮槽函数
// ============================================================================

void SettingsDialog::on_exportPathBrowseBtn_clicked()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("选择默认导出路径"),
        ui->exportPathEdit->text().isEmpty() ? QDir::homePath() : ui->exportPathEdit->text());
    if (!dir.isEmpty()) {
        ui->exportPathEdit->setText(dir);
    }
}

// ============================================================================
// 静态便捷方法（改用 AppConfig）
// ============================================================================

bool SettingsDialog::autoSaveEnabled()
{
    return AppConfig::instance().autoSaveEnabled();
}

int SettingsDialog::autoSaveInterval()
{
    // 返回分钟数（保持原有接口兼容）
    return AppConfig::instance().autoSaveInterval() / 60000;
}

QString SettingsDialog::defaultExportFormat()
{
    return AppConfig::instance().defaultExportFormat();
}

QString SettingsDialog::defaultFontFamily()
{
    return AppConfig::instance().editorFontFamily();
}

int SettingsDialog::defaultFontSize()
{
    return AppConfig::instance().editorFontSize();
}

QString SettingsDialog::databasePath()
{
    // 数据库文件放在程序目录下的 data 子目录
    return QDir(QCoreApplication::applicationDirPath() + "/data")
        .filePath("experiment_reports.db");
}
