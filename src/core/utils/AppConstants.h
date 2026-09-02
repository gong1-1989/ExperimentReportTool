/**
 * @file AppConstants.h
 * @brief 应用程序全局常量定义
 *
 * 集中管理应用名称、版本、组织名、默认值等常量，
 * 避免在代码中散落硬编码字符串。
 */

#ifndef APP_CONSTANTS_H
#define APP_CONSTANTS_H

#include <QString>

/**
 * @brief 应用程序全局常量命名空间
 *
 * 所有全局常量都放在此命名空间中，使用时通过 AppConstants::XXX 访问。
 */
namespace AppConstants {

// ---------------------------------------------------------------------------
// 应用基本信息
// ---------------------------------------------------------------------------

/// 组织名称（用于 QSettings 配置路径等）
inline const QString ORG_NAME = "ExperimentLab";

/// 应用程序名称
inline const QString APP_NAME = "ExperimentReportTool";

/// 应用程序显示名称（用于窗口标题等）
inline const QString APP_DISPLAY_NAME = "实验报告记录工具";

/// 应用程序版本号
inline const QString APP_VERSION = "1.0.0";

/// 配置文件中的数据库版本号（用于迁移判断）
inline const int DATABASE_VERSION = 1;

// ---------------------------------------------------------------------------
// 文件与目录相关常量
// ---------------------------------------------------------------------------

/// 数据库文件名
inline const QString DB_FILE_NAME = "experiment_reports.db";

/// 日志目录名（相对于数据目录）
inline const QString LOG_DIR_NAME = "logs";

/// 附件存储目录名（相对于数据目录）
inline const QString ATTACHMENT_DIR_NAME = "attachments";

/// 备份目录名（相对于数据目录）
inline const QString BACKUP_DIR_NAME = "backups";

/// 模板文件扩展名
inline const QString TEMPLATE_FILE_EXT = ".ertemplate";

/// 报告导出文件扩展名（项目打包）
inline const QString REPORT_PACKAGE_EXT = ".erpkg";

// ---------------------------------------------------------------------------
// 默认值常量
// ---------------------------------------------------------------------------

/// 自动保存间隔（毫秒）
inline const int AUTO_SAVE_INTERVAL_MS = 3000;

/// 最近打开文件列表最大数量
inline const int MAX_RECENT_FILES = 10;

/// 主窗口默认宽度
inline const int DEFAULT_WINDOW_WIDTH = 1280;

/// 主窗口默认高度
inline const int DEFAULT_WINDOW_HEIGHT = 800;

// ---------------------------------------------------------------------------
// 项目状态枚举的字符串表示
// ---------------------------------------------------------------------------

/// 项目状态：进行中
inline const QString PROJECT_STATUS_ACTIVE = "active";

/// 项目状态：已完成
inline const QString PROJECT_STATUS_COMPLETED = "completed";

/// 项目状态：已归档
inline const QString PROJECT_STATUS_ARCHIVED = "archived";

// ---------------------------------------------------------------------------
// 报告状态枚举的字符串表示
// ---------------------------------------------------------------------------

/// 报告状态：草稿
inline const QString REPORT_STATUS_DRAFT = "draft";

/// 报告状态：已提交
inline const QString REPORT_STATUS_SUBMITTED = "submitted";

/// 报告状态：已审核
inline const QString REPORT_STATUS_REVIEWED = "reviewed";

// ---------------------------------------------------------------------------
// 设置（QSettings）键名常量
// ---------------------------------------------------------------------------

namespace SettingsKeys {
    /// 主窗口几何信息（大小、位置）
    inline const QString MAIN_WINDOW_GEOMETRY = "ui/mainWindowGeometry";
    /// 主窗口状态（工具栏、 dock 等）
    inline const QString MAIN_WINDOW_STATE = "ui/mainWindowState";
    /// 最近打开的项目
    inline const QString RECENT_PROJECTS = "ui/recentProjects";
    /// 主题（light/dark/system）
    inline const QString THEME = "ui/theme";
    /// 字体大小
    inline const QString FONT_SIZE = "ui/fontSize";
    /// 是否启用自动保存
    inline const QString AUTO_SAVE_ENABLED = "editor/autoSaveEnabled";
    /// 自动保存间隔
    inline const QString AUTO_SAVE_INTERVAL = "editor/autoSaveInterval";
    /// 默认导出格式
    inline const QString DEFAULT_EXPORT_FORMAT = "export/defaultFormat";
    /// 数据库版本（迁移用）
    inline const QString DATABASE_VERSION = "database/version";
}

} // namespace AppConstants

#endif // APP_CONSTANTS_H
