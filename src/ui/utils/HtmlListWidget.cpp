/**
 * @file HtmlListWidget.cpp
 * @brief 通用 HTML 列表组件实现
 */

#include "HtmlListWidget.h"
#include "UiUtils.h"
#include "core/utils/AppTheme.h"

#include <QLabel>
#include <QListWidgetItem>

// ============================================================================
// 构造函数
// ============================================================================

HtmlListWidget::HtmlListWidget(QWidget* parent)
    : QListWidget(parent)
    , m_emptyShown(false)
{
    // 统一列表样式
    setStyleSheet(QString(
        "QListWidget { border: 1px solid %1; border-radius: 4px; background-color: %2; }"
        "QListWidget::item { border-bottom: 1px solid %3; padding: 0; }"
        "QListWidget::item:selected { background-color: %4; }"
    ).arg(AppTheme::Color::Border)
     .arg(AppTheme::Color::BgWhite)
     .arg(AppTheme::Color::BorderExtraLight)
     .arg(AppTheme::Color::PrimaryLight));

    setAlternatingRowColors(false);
    setSelectionMode(QAbstractItemView::SingleSelection);
}

// ============================================================================
// 添加 HTML 列表项
// ============================================================================

QListWidgetItem* HtmlListWidget::addHtmlItem(const QString& html,
                                              int height,
                                              const QVariant& data)
{
    // 如果当前显示空状态，先清除
    if (m_emptyShown) {
        clear();
        m_emptyShown = false;
    }

    QListWidgetItem* item = new QListWidgetItem(this);
    item->setSizeHint(QSize(0, height));
    item->setData(Qt::UserRole, data);

    QLabel* label = new QLabel(html, this);
    label->setWordWrap(true);
    label->setStyleSheet("background: transparent; border: none; padding: 4px 8px;");
    label->setTextFormat(Qt::RichText);

    addItem(item);
    setItemWidget(item, label);

    return item;
}

// ============================================================================
// 空状态
// ============================================================================

void HtmlListWidget::setEmptyText(const QString& text, const QString& icon)
{
    m_emptyText = text;
    m_emptyIcon = icon;
    updateEmptyState();
}

void HtmlListWidget::updateEmptyState()
{
    if (count() == 0 && !m_emptyText.isEmpty()) {
        // 显示空状态
        QListWidgetItem* item = new QListWidgetItem(this);
        item->setSizeHint(QSize(0, 120));
        item->setFlags(Qt::NoItemFlags);  // 不可选中

        QLabel* label = new QLabel(
            UiUtils::emptyStateHtml(m_emptyIcon, m_emptyText), this);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("background: transparent; border: none;");

        addItem(item);
        setItemWidget(item, label);
        m_emptyShown = true;
    }
}

// ============================================================================
// 获取选中项数据
// ============================================================================

QVariant HtmlListWidget::currentItemData() const
{
    QListWidgetItem* item = currentItem();
    if (!item || m_emptyShown) return QVariant();
    return item->data(Qt::UserRole);
}
