/**
 * @file AppConfig.h
 * @brief 应用配置管理
 *
 * 从 config.ini 读取用户可配置项，配置缺失时返回 AppTheme/AppDimensions 中的默认值。
 * 单例模式，全局唯一实例。
 *
 * 设计原则：
 * - 用户可配置项放 .ini（主题色、字体、窗口大小、自动保存间隔等）
 * - 代码内部逻辑常量仍用 AppTheme/AppDimensions（状态颜色、间距、控件最小高度等）
 * - 配置缺失时自动回退到默认值，保证程序稳定运行
 *
 * 配置文件位置：程序运行目录下的 config.ini
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <QString>
#include <QColor>
#include <QSettings>

/**
 * @brief 应用配置管理类（单例）
 */
class AppConfig
{
public:
    /// 获取单例实例
    static AppConfig& instance();

    // =======================================================================
    // 主题配置 [Theme]
    // =======================================================================

    /// 主色调
    QColor primaryColor() const;

    /// 成功色
    QColor successColor() const;

    /// 警告色
    QColor warningColor() const;

    /// 危险色
    QColor dangerColor() const;

    /// 字体族
    QString fontFamily() const;

    /// 基础字体大小（px）
    int baseFontSize() const;

    // =======================================================================
    // 窗口配置 [Window]
    // =======================================================================

    /// 主窗口默认宽度
    int mainWindowWidth() const;

    /// 主窗口默认高度
    int mainWindowHeight() const;

    /// 是否记住窗口大小和位置
    bool rememberWindowSize() const;

    // =======================================================================
    // 编辑器配置 [Editor]
    // =======================================================================

    /// 编辑器默认字体大小（px）
    int editorFontSize() const;

    /// 编辑器默认字体族
    QString editorFontFamily() const;

    /// 自动保存是否启用
    bool autoSaveEnabled() const;

    /// 自动保存间隔（毫秒）
    int autoSaveInterval() const;

    /// 最大版本数
    int maxVersions() const;

    // =======================================================================
    // UI 配置 [UI]
    // =======================================================================

    /// 状态栏消息显示时长（毫秒）
    int statusMessageDuration() const;

    /// 是否显示状态栏
    bool showStatusBar() const;

    // =======================================================================
    // 导出配置 [Export]
    // =======================================================================

    /// 默认导出格式（pdf/word/html/text）
    QString defaultExportFormat() const;

    /// 默认导出路径
    QString defaultExportPath() const;

    // =======================================================================
    // 配置写入（用户修改后保存）
    // =======================================================================

    void setPrimaryColor(const QColor& color);
    void setSuccessColor(const QColor& color);
    void setWarningColor(const QColor& color);
    void setDangerColor(const QColor& color);
    void setFontFamily(const QString& family);
    void setBaseFontSize(int size);
    void setMainWindowWidth(int width);
    void setMainWindowHeight(int height);
    void setEditorFontSize(int size);
    void setEditorFontFamily(const QString& family);
    void setAutoSaveEnabled(bool enabled);
    void setAutoSaveInterval(int ms);
    void setMaxVersions(int count);
    void setRememberWindowSize(bool remember);
    void setStatusMessageDuration(int ms);
    void setShowStatusBar(bool show);
    void setDefaultExportFormat(const QString& format);
    void setDefaultExportPath(const QString& path);

    /// 保存所有配置到文件
    void save();

    /// 重置为默认值（清除所有用户配置）
    void resetToDefaults();

    /// 配置文件路径
    QString configFilePath() const;

private:
    AppConfig();
    ~AppConfig() = default;
    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;

    /// 读取配置值，缺失时返回默认值
    QVariant value(const QString& key, const QVariant& defaultValue) const;

    /// 写入配置值
    void setValue(const QString& key, const QVariant& value);

    QSettings m_settings;  ///< 配置文件读写器
};

#endif // APP_CONFIG_H
