/**
 * @file UiHelper.h
 * @brief 全局 UI 提示工具类
 *
 * 统一所有错误/信息/警告/确认提示的入口：
 * - MainWindow、widgets、对话框等所有界面代码统一走 UiHelper
 * - 后续统一标题、样式、换肤、国际化时只需修改本类一处
 *
 * 使用方式：
 * @code
 *   UiHelper::warning(this, tr("保存失败"), tr("数据表保存到数据库失败"));
 *   if (UiHelper::confirm(this, tr("确定删除该报告吗？"))) { ... }
 * @endcode
 */

#ifndef UI_HELPER_H
#define UI_HELPER_H

#include <QString>
#include <QMessageBox>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QTableWidget>
#include <QTableView>
#include <QHeaderView>

class QWidget;

/**
 * @brief 表格单元格居中委托
 *
 * QSS 的 text-align 对表格单元格不生效，因此通过自定义委托
 * 将单元格内容统一水平+垂直居中。
 */
class CenteredItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

protected:
    void initStyleOption(QStyleOptionViewItem* option,
                         const QModelIndex& index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        option->displayAlignment = Qt::AlignCenter;
    }
};

/**
 * @brief UI 提示工具类（全部为静态方法）
 */
class UiHelper
{
public:
    /**
     * @brief 显示错误提示（QMessageBox::Critical）
     * @param parent 父窗口
     * @param title 标题（默认"错误"）
     * @param message 信息内容
     */
    static void error(QWidget* parent, const QString& title, const QString& message = QString());

    /**
     * @brief 显示信息提示（QMessageBox::Information）
     * @param parent 父窗口
     * @param title 标题（默认"提示"）
     * @param message 信息内容
     */
    static void info(QWidget* parent, const QString& title, const QString& message = QString());

    /**
     * @brief 显示警告提示（QMessageBox::Warning）
     * @param parent 父窗口
     * @param title 标题（默认"警告"）
     * @param message 信息内容
     */
    static void warning(QWidget* parent, const QString& title, const QString& message = QString());

    /**
     * @brief 显示带自定义按钮的警告对话框（QMessageBox::Warning）
     *
     * 用于需要"保存/放弃/取消"等多按钮选择的三态确认场景。
     * @param parent 父窗口
     * @param title 标题
     * @param message 提示信息
     * @param buttons 按钮组合（如 QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel）
     * @param defaultButton 默认按钮
     * @return 用户点击的按钮
     */
    static QMessageBox::StandardButton warning(QWidget* parent, const QString& title,
                                               const QString& message,
                                               QMessageBox::StandardButtons buttons,
                                               QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

    /**
     * @brief 显示确认对话框（QMessageBox::Question）
     * @param parent 父窗口
     * @param title 标题（默认"确认"）
     * @param message 信息内容
     * @param buttons 按钮集合（默认仅确定）
     * @param defaultButton 默认按钮（默认取消）
     */
    static bool confirm(QWidget* parent, const QString& title, const QString& message = QString());

    /**
     * @brief 使表格单元格与表头内容居中显示
     * @param table 目标表格控件
     */
    static void centerTableWidget(QTableWidget* table);
    static void centerTableWidget(QTableView* table);
};

#endif // UI_HELPER_H
