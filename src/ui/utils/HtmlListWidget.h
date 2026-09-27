/**
 * @file HtmlListWidget.h
 * @brief 通用 HTML 列表组件
 *
 * 封装 QListWidget，支持直接添加 HTML 内容的列表项。
 * 消除各对话框中重复的"创建 QListWidgetItem + QLabel + setSizeHint"代码。
 *
 * 使用示例：
 * @code
 *   HtmlListWidget* list = new HtmlListWidget(this);
 *   list->addHtmlItem("<b>标题</b><br>副标题");
 *   list->addHtmlItem(html, 80);  // 指定高度
 *   list->setEmptyText("暂无数据");
 * @endcode
 */

#ifndef HTML_LIST_WIDGET_H
#define HTML_LIST_WIDGET_H

#include <QListWidget>
#include <QString>

class HtmlListWidget : public QListWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit HtmlListWidget(QWidget* parent = nullptr);

    /**
     * @brief 添加 HTML 列表项
     * @param html HTML 内容
     * @param height 列表项高度（px），默认 60
     * @param data 关联数据（可选）
     * @return 创建的 QListWidgetItem
     */
    QListWidgetItem* addHtmlItem(const QString& html,
                                  int height = 60,
                                  const QVariant& data = QVariant());

    /**
     * @brief 设置空状态显示文字
     * @param text 空状态提示文字
     * @param icon 图标（emoji 或 Unicode），默认 "📭"
     */
    void setEmptyText(const QString& text, const QString& icon = QStringLiteral("📭"));

    /**
     * @brief 刷新空状态显示（列表为空时显示提示）
     */
    void updateEmptyState();

    /**
     * @brief 获取当前选中项的关联数据
     * @return 数据，无选中时返回无效 QVariant
     */
    QVariant currentItemData() const;

private:
    QString m_emptyText;  ///< 空状态提示文字
    QString m_emptyIcon;  ///< 空状态图标
    bool m_emptyShown;    ///< 当前是否显示空状态
};

#endif // HTML_LIST_WIDGET_H
