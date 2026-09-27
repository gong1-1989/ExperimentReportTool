/**
 * @file AppConfig.cpp
 * @brief 应用配置管理实现
 */

#include "AppConfig.h"
#include "AppTheme.h"
#include "AppDimensions.h"
#include "Logger.h"

#include <QCoreApplication>
#include <QDir>

// ===========================================================================
// 单例实现
// ===========================================================================

AppConfig& AppConfig::instance()
{
    static AppConfig s_instance;
    return s_instance;
}

AppConfig::AppConfig()
    : m_settings(QCoreApplication::applicationDirPath() + "/config.ini",
                 QSettings::IniFormat)
{
    LOG_INFO(QString("配置文件路径: %1").arg(configFilePath()));
}

// ===========================================================================
// 内部辅助方法
// ===========================================================================

QVariant AppConfig::value(const QString& key, const QVariant& defaultValue) const
{
    return m_settings.value(key, defaultValue);
}

void AppConfig::setValue(const QString& key, const QVariant& val)
{
    m_settings.setValue(key, val);
}

QString AppConfig::configFilePath() const
{
    return m_settings.fileName();
}

// ===========================================================================
// 主题配置 [Theme]
// ===========================================================================

QColor AppConfig::primaryColor() const
{
    return QColor(value("Theme/primaryColor", AppTheme::Color::Primary).toString());
}

QColor AppConfig::successColor() const
{
    return QColor(value("Theme/successColor", AppTheme::Color::Success).toString());
}

QColor AppConfig::warningColor() const
{
    return QColor(value("Theme/warningColor", AppTheme::Color::Warning).toString());
}

QColor AppConfig::dangerColor() const
{
    return QColor(value("Theme/dangerColor", AppTheme::Color::Danger).toString());
}

QString AppConfig::fontFamily() const
{
    return value("Theme/fontFamily", "Microsoft YaHei").toString();
}

int AppConfig::baseFontSize() const
{
    return value("Theme/baseFontSize", AppTheme::FontSize::Small).toInt();
}

// ===========================================================================
// 窗口配置 [Window]
// ===========================================================================

int AppConfig::mainWindowWidth() const
{
    return value("Window/mainWidth", AppDimensions::Window::MainWidth).toInt();
}

int AppConfig::mainWindowHeight() const
{
    return value("Window/mainHeight", AppDimensions::Window::MainHeight).toInt();
}

bool AppConfig::rememberWindowSize() const
{
    return value("Window/rememberWindowSize", true).toBool();
}

// ===========================================================================
// 编辑器配置 [Editor]
// ===========================================================================

int AppConfig::editorFontSize() const
{
    return value("Editor/fontSize", AppTheme::FontSize::Normal).toInt();
}

QString AppConfig::editorFontFamily() const
{
    return value("Editor/fontFamily", "Microsoft YaHei").toString();
}

bool AppConfig::autoSaveEnabled() const
{
    return value("Editor/autoSaveEnabled", true).toBool();
}

int AppConfig::autoSaveInterval() const
{
    return value("Editor/autoSaveInterval", AppDimensions::AutoSave::IntervalMs).toInt();
}

int AppConfig::maxVersions() const
{
    return value("Editor/maxVersions", AppDimensions::AutoSave::MaxVersions).toInt();
}

// ===========================================================================
// UI 配置 [UI]
// ===========================================================================

int AppConfig::statusMessageDuration() const
{
    return value("UI/statusMessageDuration", AppDimensions::Delay::StatusMessage).toInt();
}

bool AppConfig::showStatusBar() const
{
    return value("UI/showStatusBar", true).toBool();
}

// ===========================================================================
// 导出配置 [Export]
// ===========================================================================

QString AppConfig::defaultExportFormat() const
{
    return value("Export/defaultFormat", "pdf").toString();
}

QString AppConfig::defaultExportPath() const
{
    return value("Export/defaultPath", QDir::homePath()).toString();
}

// ===========================================================================
// 数据库配置 [Database]
// ===========================================================================

QString AppConfig::databasePath() const
{
    return value("Database/path", "").toString();
}

void AppConfig::setDatabasePath(const QString& path)
{
    setValue("Database/path", path);
}

// ===========================================================================
// 配置写入
// ===========================================================================

void AppConfig::setPrimaryColor(const QColor& color)
{
    setValue("Theme/primaryColor", color.name());
}

void AppConfig::setFontFamily(const QString& family)
{
    setValue("Theme/fontFamily", family);
}

void AppConfig::setBaseFontSize(int size)
{
    setValue("Theme/baseFontSize", size);
}

void AppConfig::setEditorFontSize(int size)
{
    setValue("Editor/fontSize", size);
}

void AppConfig::setEditorFontFamily(const QString& family)
{
    setValue("Editor/fontFamily", family);
}

void AppConfig::setAutoSaveEnabled(bool enabled)
{
    setValue("Editor/autoSaveEnabled", enabled);
}

void AppConfig::setAutoSaveInterval(int ms)
{
    setValue("Editor/autoSaveInterval", ms);
}

void AppConfig::setRememberWindowSize(bool remember)
{
    setValue("Window/rememberWindowSize", remember);
}

void AppConfig::setDefaultExportFormat(const QString& format)
{
    setValue("Export/defaultFormat", format);
}

void AppConfig::setSuccessColor(const QColor& color)
{
    setValue("Theme/successColor", color.name());
}

void AppConfig::setWarningColor(const QColor& color)
{
    setValue("Theme/warningColor", color.name());
}

void AppConfig::setDangerColor(const QColor& color)
{
    setValue("Theme/dangerColor", color.name());
}

void AppConfig::setMainWindowWidth(int width)
{
    setValue("Window/mainWidth", width);
}

void AppConfig::setMainWindowHeight(int height)
{
    setValue("Window/mainHeight", height);
}

void AppConfig::setMaxVersions(int count)
{
    setValue("Editor/maxVersions", count);
}

void AppConfig::setStatusMessageDuration(int ms)
{
    setValue("UI/statusMessageDuration", ms);
}

void AppConfig::setShowStatusBar(bool show)
{
    setValue("UI/showStatusBar", show);
}

void AppConfig::setDefaultExportPath(const QString& path)
{
    setValue("Export/defaultPath", path);
}

void AppConfig::save()
{
    m_settings.sync();
    LOG_INFO("配置已保存到文件");
}

void AppConfig::resetToDefaults()
{
    m_settings.clear();
    m_settings.sync();
    LOG_INFO("配置已重置为默认值");
}
