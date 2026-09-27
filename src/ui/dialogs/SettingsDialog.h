/**
 * @file SettingsDialog.h
 * @brief 设置对话框头文件
 *
 * 提供应用程序设置界面，包括：
 * - 常规设置（自动保存、默认导出格式）
 * - 编辑器设置（默认字体、字号）
 * - 数据设置（数据库路径查看）
 *
 * 使用 QSettings 持久化保存设置。
 * UI 布局由 SettingsDialog.ui 可视化设计。
 */

#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QString>

class QLabel;

// UI 类前向声明（由 .ui 文件自动生成）
namespace Ui {
class SettingsDialog;
}

/**
 * @brief 设置对话框类
 *
 * 使用方式：
 * @code
 *   SettingsDialog dialog(this);
 *   if (dialog.exec() == QDialog::Accepted) {
 *       // 设置已保存
 *   }
 * @endcode
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit SettingsDialog(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~SettingsDialog() override;

    // ========================================================================
    // 静态便捷方法：读取设置
    // ========================================================================

    /// 是否启用自动保存
    static bool autoSaveEnabled();

    /// 自动保存间隔（分钟）
    static int autoSaveInterval();

    /// 默认导出格式（"pdf", "html", "word", "text"）
    static QString defaultExportFormat();

    /// 默认字体族
    static QString defaultFontFamily();

    /// 默认字号
    static int defaultFontSize();

    /// 数据库路径
    static QString databasePath();

private slots:
    /// 点击确定按钮
    void on_okButton_clicked();

    /// 点击应用按钮
    void on_applyButton_clicked();

    /// 点击取消按钮
    void on_cancelButton_clicked();

    /// 点击恢复默认按钮
    void on_resetButton_clicked();

    /// 点击主色调选择按钮
    void on_primaryColorBtn_clicked();

    /// 点击成功色选择按钮
    void on_successColorBtn_clicked();

    /// 点击警告色选择按钮
    void on_warningColorBtn_clicked();

    /// 点击危险色选择按钮
    void on_dangerColorBtn_clicked();

    /// 点击导出路径浏览按钮
    void on_exportPathBrowseBtn_clicked();

    /// 点击数据库路径浏览按钮
    void on_dbPathBrowseBtn_clicked();

private:
    // ========================================================================
    // 成员变量
    // ========================================================================
    Ui::SettingsDialog* ui;  ///< UI 界面对象（由 .ui 文件生成）

    // ========================================================================
    // 内部方法
    // ========================================================================

    /// 加载当前设置到控件
    void loadSettings();

    /// 保存控件中的设置
    void saveSettings();

    /// 应用设置（立即生效）
    void applySettings();

    /// 更新颜色预览方块
    void updateColorPreview(QLabel* previewLabel, const QString& colorStr);
};

#endif // SETTINGS_DIALOG_H
