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
 */

#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QSettings>
#include <QString>

// 前向声明
class QCheckBox;
class QSpinBox;
class QComboBox;
class QFontComboBox;
class QLineEdit;
class QLabel;
class QTabWidget;
class QPushButton;

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
    void onAccept();

    /// 点击应用按钮
    void onApply();

    /// 点击恢复默认按钮
    void onResetDefaults();

private:
    // ========================================================================
    // UI 组件
    // ========================================================================
    QTabWidget* m_tabWidget;          ///< 选项卡控件

    // 常规设置
    QCheckBox* m_autoSaveCheck;       ///< 自动保存复选框
    QSpinBox* m_autoSaveIntervalSpin; ///< 自动保存间隔
    QComboBox* m_exportFormatCombo;   ///< 默认导出格式

    // 编辑器设置
    QFontComboBox* m_fontCombo;       ///< 默认字体
    QSpinBox* m_fontSizeSpin;         ///< 默认字号

    // 数据设置
    QLineEdit* m_dbPathEdit;          ///< 数据库路径（只读）

    // 按钮
    QPushButton* m_okButton;
    QPushButton* m_applyButton;
    QPushButton* m_cancelButton;
    QPushButton* m_resetButton;

    // ========================================================================
    // 内部方法
    // ========================================================================

    /// 初始化 UI
    void setupUi();

    /// 加载当前设置到控件
    void loadSettings();

    /// 保存控件中的设置
    void saveSettings();

    /// 应用设置（立即生效）
    void applySettings();
};

#endif // SETTINGS_DIALOG_H
