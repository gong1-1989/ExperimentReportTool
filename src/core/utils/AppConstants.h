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
inline const int DATABASE_VERSION = 4;

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

/// 主窗口最小宽度
inline const int MIN_WINDOW_WIDTH = 800;

/// 主窗口最小高度
inline const int MIN_WINDOW_HEIGHT = 600;

// ---------------------------------------------------------------------------
// UI 对话框尺寸常量
// ---------------------------------------------------------------------------

namespace DialogSize {
    /// 项目对话框最小宽度
    inline const int PROJECT_MIN_WIDTH = 450;
    /// 项目对话框最小高度
    inline const int PROJECT_MIN_HEIGHT = 380;
    /// 图表配置对话框最小宽度
    inline const int CHART_CONFIG_MIN_WIDTH = 600;
    /// 图表配置对话框最小高度
    inline const int CHART_CONFIG_MIN_HEIGHT = 700;
    /// 数据导入对话框最小宽度
    inline const int DATA_IMPORT_MIN_WIDTH = 700;
    /// 数据导入对话框最小高度
    inline const int DATA_IMPORT_MIN_HEIGHT = 600;
    /// 数据表编辑器对话框最小宽度
    inline const int DATA_TABLE_MIN_WIDTH = 900;
    /// 数据表编辑器对话框最小高度
    inline const int DATA_TABLE_MIN_HEIGHT = 700;
    /// 公式编辑器对话框最小宽度
    inline const int FORMULA_MIN_WIDTH = 800;
    /// 公式编辑器对话框最小高度
    inline const int FORMULA_MIN_HEIGHT = 600;
    /// 搜索结果对话框最小宽度
    inline const int SEARCH_MIN_WIDTH = 900;
    /// 搜索结果对话框最小高度
    inline const int SEARCH_MIN_HEIGHT = 650;
    /// 版本历史对话框最小宽度
    inline const int VERSION_MIN_WIDTH = 850;
    /// 版本历史对话框最小高度
    inline const int VERSION_MIN_HEIGHT = 600;
    /// 标签管理对话框最小宽度
    inline const int TAG_MANAGER_MIN_WIDTH = 750;
    /// 标签管理对话框最小高度
    inline const int TAG_MANAGER_MIN_HEIGHT = 550;
    /// 附件管理对话框最小宽度
    inline const int ATTACHMENT_MIN_WIDTH = 700;
    /// 附件管理对话框最小高度
    inline const int ATTACHMENT_MIN_HEIGHT = 500;
}

// ---------------------------------------------------------------------------
// 主题颜色常量
// ---------------------------------------------------------------------------

namespace ThemeColors {
    /// 成功状态颜色（绿色）
    inline const QString SUCCESS = "#67C23A";
    /// 警告状态颜色（橙色）
    inline const QString WARNING = "#E6A23C";
    /// 错误状态颜色（红色）
    inline const QString ERROR = "#F56C6C";
    /// 信息状态颜色（蓝色）
    inline const QString INFO = "#409EFF";
    /// 主要文字颜色
    inline const QString TEXT_PRIMARY = "#303133";
    /// 常规文字颜色
    inline const QString TEXT_REGULAR = "#606266";
    /// 次要文字颜色
    inline const QString TEXT_SECONDARY = "#909399";
    /// 占位文字颜色
    inline const QString TEXT_PLACEHOLDER = "#C0C4CC";
    /// 边框颜色
    inline const QString BORDER = "#DCDFE6";
    /// 背景颜色（浅灰）
    inline const QString BACKGROUND_LIGHT = "#F5F7FA";
    /// 背景颜色（白色）
    inline const QString BACKGROUND_WHITE = "#FFFFFF";
}

// ---------------------------------------------------------------------------
// 超时时间常量（毫秒）
// ---------------------------------------------------------------------------

namespace Timeouts {
    /// 自动保存延迟（防抖）
    inline const int AUTO_SAVE_DEBOUNCE_MS = 1000;
    /// 搜索输入防抖延迟
    inline const int SEARCH_DEBOUNCE_MS = 300;
    /// 状态消息显示时间
    inline const int STATUS_MESSAGE_MS = 3000;
    /// 工具提示显示时间
    inline const int TOOLTIP_TIMEOUT_MS = 5000;
    /// 文件操作超时
    inline const int FILE_OPERATION_MS = 30000;
}

// ---------------------------------------------------------------------------
// 数据限制常量
// ---------------------------------------------------------------------------

namespace DataLimits {
    /// 项目名称最大长度
    inline const int PROJECT_NAME_MAX_LENGTH = 100;
    /// 报告标题最大长度
    inline const int REPORT_TITLE_MAX_LENGTH = 200;
    /// 标签名称最大长度
    inline const int TAG_NAME_MAX_LENGTH = 50;
    /// 模板名称最大长度
    inline const int TEMPLATE_NAME_MAX_LENGTH = 100;
    /// 搜索历史最大数量
    inline const int SEARCH_HISTORY_MAX = 20;
    /// 附件最大大小（MB）
    inline const int ATTACHMENT_MAX_SIZE_MB = 50;
    /// 单次导入最大行数
    inline const int IMPORT_MAX_ROWS = 100000;
}

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
