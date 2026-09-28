/**
 * @file AppTheme.h
 * @brief 应用主题常量
 *
 * 集中管理所有颜色、字体、间距、圆角等样式常量，避免魔法数字和魔法文字。
 * 修改主题只需修改此文件，无需在代码中到处搜索替换。
 *
 * 设计原则：
 * - 颜色用语义命名（Primary/Success/Warning），不用颜色名（Blue/Green）
 * - 字体大小用相对命名（Small/Medium/Large），不用具体像素值
 * - 间距用阶梯命名（Tiny/Small/Medium/Normal/Large），统一 4px 基准
 * - 所有常量为 constexpr，编译期求值，零运行时开销
 */

#ifndef APP_THEME_H
#define APP_THEME_H

#include <QString>
#include <QColor>
#include "core/models/Report.h"

namespace AppTheme {

// ===========================================================================
// 颜色常量
// ===========================================================================
namespace Color {

    // --- 主色调 ---
    constexpr const char* Primary       = "#4A90D9";  ///< 主色（按钮、链接、选中态）
    constexpr const char* PrimaryHover  = "#357ABD";  ///< 主色悬停
    constexpr const char* PrimaryLight  = "#ECF5FF";  ///< 主色浅背景（选中行背景）

    // --- 状态色 ---
    constexpr const char* Success = "#67C23A";  ///< 成功（已保存、已审核、操作成功）
    constexpr const char* Warning = "#E6A23C";  ///< 警告（未保存、已提交、需注意）
    constexpr const char* Danger  = "#F56C6C";  ///< 危险（错误、删除、失败）
    constexpr const char* Info    = "#909399";  ///< 信息（提示、辅助说明）

    // --- 文本色（Element UI 规范）---
    constexpr const char* TextPrimary     = "#303133";  ///< 主要文字（标题、正文）
    constexpr const char* TextRegular     = "#606266";  ///< 常规文字（正文、列表）
    constexpr const char* TextSecondary   = "#909399";  ///< 次要文字（辅助信息、时间）
    constexpr const char* TextPlaceholder = "#C0C4CC";  ///< 占位文字（空状态、placeholder）

    // --- 边框/分割线 ---
    constexpr const char* Border           = "#DCDFE6";  ///< 常规边框
    constexpr const char* BorderLight      = "#E4E7ED";  ///< 浅边框
    constexpr const char* BorderLighter    = "#EBEEF5";  ///< 更浅边框
    constexpr const char* BorderExtraLight = "#F2F6FC";  ///< 极浅边框（分割线）

    // --- 背景色 ---
    constexpr const char* BgWhite = "#FFFFFF";  ///< 纯白（卡片、弹窗）
    constexpr const char* BgPage  = "#F5F7FA";  ///< 页面背景
    constexpr const char* BgGray  = "#FAFAFA";  ///< 浅灰背景（交替行）

    // --- 常用灰色 ---
    constexpr const char* Gray333 = "#333333";  ///< 深灰（标题）
    constexpr const char* Gray666 = "#666666";  ///< 中灰（正文）
    constexpr const char* Gray999 = "#999999";  ///< 浅灰（辅助）
    constexpr const char* GrayCCC = "#CCCCCC";  ///< 更浅灰（占位）
    constexpr const char* GrayDDD = "#DDDDDD";  ///< 分割线
    constexpr const char* GrayEEE = "#EEEEEE";  ///< 浅分割线
}

// ===========================================================================
// 字体大小（px）
// ===========================================================================
namespace FontSize {
    constexpr int ExtraSmall = 11;  ///< 极小（标签、徽章、辅助信息）
    constexpr int Small      = 12;  ///< 小（正文、列表、菜单）
    constexpr int Medium     = 13;  ///< 中（按钮、输入框）
    constexpr int Normal     = 14;  ///< 常规（标题、正文强调）
    constexpr int Large      = 16;  ///< 大（报告标题、对话框标题）
    constexpr int ExtraLarge = 18;  ///< 更大（页面标题）
    constexpr int Huge       = 24;  ///< 大图标文字
    constexpr int Massive    = 48;  ///< 空状态大图标
}

// ===========================================================================
// 标题字号（磅值，编辑器内标题专用；与 px 档位 FontSize 分离）
// ===========================================================================
namespace Heading {
    constexpr int H1 = 22;  ///< 一级标题
    constexpr int H2 = 18;  ///< 二级标题
    constexpr int H3 = 15;  ///< 三级标题
}

// ===========================================================================
// 间距（px，4px 基准阶梯）
// ===========================================================================
namespace Spacing {
    constexpr int Tiny       = 2;   ///< 极小（图标与文字间距）
    constexpr int Small      = 4;   ///< 小（内边距、元素间距）
    constexpr int Medium     = 6;   ///< 中（按钮内边距）
    constexpr int Normal     = 8;   ///< 常规（控件间距）
    constexpr int Large      = 12;  ///< 大（区块间距）
    constexpr int ExtraLarge = 16;  ///< 更大（页面边距）
    constexpr int Huge       = 24;  ///< 大（弹窗边距）
    constexpr int Massive    = 32;  ///< 极大（空状态内边距）
}

// ===========================================================================
// 图表系列色（数据可视化专用色板，与 UI 主题色分离）
// ===========================================================================
namespace Chart {
    constexpr const char* Series1 = "#4A90D9";  ///< 系列1 蓝色
    constexpr const char* Series2 = "#67C23A";  ///< 系列2 绿色
    constexpr const char* Series3 = "#E6A23C";  ///< 系列3 橙色
    constexpr const char* Series4 = "#F56C6C";  ///< 系列4 红色
    constexpr const char* Series5 = "#9B59B6";  ///< 系列5 紫色
    constexpr const char* Series6 = "#1ABC9C";  ///< 系列6 青色
    constexpr const char* Series7 = "#E91E63";  ///< 系列7 粉色
    constexpr const char* Series8 = "#FF9800";  ///< 系列8 深橙
}

// ===========================================================================
// 圆角（px）
// ===========================================================================
namespace Radius {
    constexpr int Small  = 2;    ///< 小圆角（输入框、按钮）
    constexpr int Medium = 4;    ///< 中圆角（卡片、弹窗）
    constexpr int Large  = 6;    ///< 大圆角（图片、容器）
    constexpr int Pill   = 999;  ///< 胶囊形（标签、徽章）
}

// ===========================================================================
// 报告状态 → 颜色映射
// ===========================================================================

/**
 * @brief 获取报告状态对应的颜色
 * @param status 报告状态
 * @return 颜色值
 */
inline QColor statusColor(ReportStatus status)
{
    switch (status) {
        case ReportStatus::Draft:     return QColor(Color::TextSecondary);
        case ReportStatus::Submitted: return QColor(Color::Warning);
        case ReportStatus::Reviewed:  return QColor(Color::Success);
        default:                      return QColor(Color::TextSecondary);
    }
}

/**
 * @brief 获取报告状态对应的中文名称
 * @param status 报告状态
 * @return 状态名称
 */
inline QString statusName(ReportStatus status)
{
    switch (status) {
        case ReportStatus::Draft:     return QStringLiteral("草稿");
        case ReportStatus::Submitted: return QStringLiteral("已提交");
        case ReportStatus::Reviewed:  return QStringLiteral("已审核");
        default:                      return QStringLiteral("未知");
    }
}

// ===========================================================================
// 预设标签颜色（12 种，Ant Design 色板）
// ===========================================================================
namespace TagColors {
    constexpr const char* Presets[] = {
        "#4A90D9",  // 蓝
        "#52c41a",  // 绿
        "#faad14",  // 黄
        "#f5222d",  // 红
        "#722ed1",  // 紫
        "#13c2c2",  // 青
        "#eb2f96",  // 粉
        "#fa8c16",  // 橙
        "#2f54eb",  // 深蓝
        "#a0d911",  // 黄绿
        "#8c8c8c",  // 灰
        "#000000"   // 黑
    };
    constexpr int Count = 12;

    /// 预设颜色名称
    inline QStringList presetNames() {
        return {
            QStringLiteral("蓝色"), QStringLiteral("绿色"), QStringLiteral("黄色"),
            QStringLiteral("红色"), QStringLiteral("紫色"), QStringLiteral("青色"),
            QStringLiteral("粉色"), QStringLiteral("橙色"), QStringLiteral("深蓝"),
            QStringLiteral("黄绿"), QStringLiteral("灰色"), QStringLiteral("黑色")
        };
    }
}

} // namespace AppTheme

#endif // APP_THEME_H
