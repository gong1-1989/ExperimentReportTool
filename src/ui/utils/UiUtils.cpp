/**
 * @file UiUtils.cpp
 * @brief UI 工具类实现
 */

#include "UiUtils.h"
#include "core/utils/AppTheme.h"

// ============================================================================
// 空状态显示
// ============================================================================

QString UiUtils::emptyStateHtml(const QString& icon,
                                 const QString& text,
                                 int iconSize)
{
    return QString(
        "<div style='text-align: center; padding: 40px 20px;'>"
        "<div style='font-size: %1px; margin-bottom: 12px;'>%2</div>"
        "<div style='color: %3; font-size: %4px;'>%5</div>"
        "</div>"
    ).arg(iconSize)
     .arg(icon)
     .arg(AppTheme::Color::TextPlaceholder)
     .arg(AppTheme::FontSize::Normal)
     .arg(text.toHtmlEscaped());
}

QString UiUtils::emptyStateHtml(const QString& icon,
                                 const QString& title,
                                 const QString& description,
                                 int iconSize)
{
    return QString(
        "<div style='text-align: center; padding: 40px 20px;'>"
        "<div style='font-size: %1px; margin-bottom: 12px;'>%2</div>"
        "<div style='color: %3; font-size: %4px; font-weight: bold; margin-bottom: 8px;'>%5</div>"
        "<div style='color: %6; font-size: %7px;'>%8</div>"
        "</div>"
    ).arg(iconSize)
     .arg(icon)
     .arg(AppTheme::Color::TextRegular)
     .arg(AppTheme::FontSize::Large)
     .arg(title.toHtmlEscaped())
     .arg(AppTheme::Color::TextPlaceholder)
     .arg(AppTheme::FontSize::Small)
     .arg(description.toHtmlEscaped());
}

// ============================================================================
// 列表项显示
// ============================================================================

QString UiUtils::listItemHtml(const QString& title,
                               const QString& subtitle,
                               const QColor& titleColor,
                               const QColor& subtitleColor)
{
    if (subtitle.isEmpty()) {
        return QString(
            "<div style='padding: 8px 12px;'>"
            "<div style='color: %1; font-size: %2px; font-weight: bold;'>%3</div>"
            "</div>"
        ).arg(titleColor.name())
         .arg(AppTheme::FontSize::Normal)
         .arg(title.toHtmlEscaped());
    }

    return QString(
        "<div style='padding: 8px 12px;'>"
        "<div style='color: %1; font-size: %2px; font-weight: bold; margin-bottom: 4px;'>%3</div>"
        "<div style='color: %4; font-size: %5px;'>%6</div>"
        "</div>"
    ).arg(titleColor.name())
     .arg(AppTheme::FontSize::Normal)
     .arg(title.toHtmlEscaped())
     .arg(subtitleColor.name())
     .arg(AppTheme::FontSize::Small)
     .arg(subtitle.toHtmlEscaped());
}

QString UiUtils::listItemWithStatusHtml(const QString& title,
                                         const QString& subtitle,
                                         const QString& statusText,
                                         const QColor& statusColor)
{
    const QColor statusBg = lightenColor(statusColor, 200);

    return QString(
        "<div style='padding: 8px 12px;'>"
        "<div style='display: flex; justify-content: space-between; align-items: center; margin-bottom: 4px;'>"
        "<span style='color: %1; font-size: %2px; font-weight: bold;'>%3</span>"
        "<span style='background-color: %4; color: %5; font-size: %6px; "
        "padding: 2px 8px; border-radius: 999px;'>%7</span>"
        "</div>"
        "<div style='color: %8; font-size: %9px;'>%10</div>"
        "</div>"
    ).arg(AppTheme::Color::TextPrimary)
     .arg(AppTheme::FontSize::Normal)
     .arg(title.toHtmlEscaped())
     .arg(statusBg.name())
     .arg(statusColor.name())
     .arg(AppTheme::FontSize::ExtraSmall)
     .arg(statusText.toHtmlEscaped())
     .arg(AppTheme::Color::TextSecondary)
     .arg(AppTheme::FontSize::Small)
     .arg(subtitle.toHtmlEscaped());
}

// ============================================================================
// 颜色工具
// ============================================================================

QColor UiUtils::lightenColor(const QColor& color, int factor)
{
    return color.lighter(factor);
}

bool UiUtils::isLightColor(const QColor& color)
{
    // 计算相对亮度（YIQ 公式）
    const int yiq = (color.red() * 299 + color.green() * 587 + color.blue() * 114) / 1000;
    return yiq > 128;
}

// ============================================================================
// 尺寸工具
// ============================================================================

QSize UiUtils::listItemSize(int height)
{
    return QSize(0, height);
}
