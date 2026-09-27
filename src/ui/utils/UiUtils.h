/**
 * @file UiUtils.h
 * @brief UI 工具类
 *
 * 提供通用的 UI 辅助方法，消除各对话框和组件中的重复代码。
 * 包括：
 * - 空状态 HTML 生成
 * - 列表项 HTML 生成
 * - 颜色工具方法
 */

#ifndef UI_UTILS_H
#define UI_UTILS_H

#include <QString>
#include <QColor>
#include <QSize>

/**
 * @brief UI 工具类
 *
 * 所有方法均为静态方法，无需实例化。
 */
class UiUtils
{
public:
    // ========================================================================
    // 空状态显示
    // ========================================================================

    /**
     * @brief 生成空状态的 HTML 内容
     * @param icon 图标字符（如 emoji 或 Unicode 符号）
     * @param text 提示文字
     * @param iconSize 图标大小（px），默认 48
     * @return HTML 字符串
     *
     * 使用示例：
     * @code
     *   label->setText(UiUtils::emptyStateHtml("📭", "暂无数据"));
     * @endcode
     */
    static QString emptyStateHtml(const QString& icon,
                                   const QString& text,
                                   int iconSize = 48);

    /**
     * @brief 生成带描述的空状态 HTML
     * @param icon 图标字符
     * @param title 标题文字
     * @param description 描述文字
     * @param iconSize 图标大小（px）
     * @return HTML 字符串
     */
    static QString emptyStateHtml(const QString& icon,
                                   const QString& title,
                                   const QString& description,
                                   int iconSize = 48);

    // ========================================================================
    // 列表项显示
    // ========================================================================

    /**
     * @brief 生成列表项的 HTML 内容（标题 + 副标题）
     * @param title 标题
     * @param subtitle 副标题（可选）
     * @param titleColor 标题颜色
     * @param subtitleColor 副标题颜色
     * @return HTML 字符串
     */
    static QString listItemHtml(const QString& title,
                                 const QString& subtitle = QString(),
                                 const QColor& titleColor = QColor("#303133"),
                                 const QColor& subtitleColor = QColor("#909399"));

    /**
     * @brief 生成带状态标签的列表项 HTML
     * @param title 标题
     * @param subtitle 副标题
     * @param statusText 状态文字
     * @param statusColor 状态颜色
     * @return HTML 字符串
     */
    static QString listItemWithStatusHtml(const QString& title,
                                           const QString& subtitle,
                                           const QString& statusText,
                                           const QColor& statusColor);

    // ========================================================================
    // 颜色工具
    // ========================================================================

    /**
     * @brief 获取颜色的浅色版本（用于背景）
     * @param color 基础颜色
     * @param factor 浅色因子（0-255，越大越浅），默认 180
     * @return 浅色 QColor
     */
    static QColor lightenColor(const QColor& color, int factor = 180);

    /**
     * @brief 判断颜色是否为浅色（用于决定文字颜色）
     * @param color 颜色
     * @return true 表示浅色，应使用深色文字
     */
    static bool isLightColor(const QColor& color);

    // ========================================================================
    // 尺寸工具
    // ========================================================================

    /**
     * @brief 列表项标准高度
     */
    static QSize listItemSize(int height = 60);

private:
    UiUtils() = delete;  ///< 禁止实例化
    ~UiUtils() = delete;
};

#endif // UI_UTILS_H
