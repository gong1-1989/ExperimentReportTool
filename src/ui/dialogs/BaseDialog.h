/**
 * @file BaseDialog.h
 * @brief 对话框基类
 *
 * 所有对话框的基类，提供通用功能：
 * - 自动去掉窗口帮助按钮（ContextHelpButtonHint）
 * - 统一的对话框尺寸设置
 * - 统一的错误/信息/警告提示
 * - 统一的确认对话框
 *
 * 使用方式：
 * @code
 *   class MyDialog : public BaseDialog {
 *       // ...
 *   };
 * @endcode
 */

#ifndef BASE_DIALOG_H
#define BASE_DIALOG_H

#include <QDialog>
#include <QString>

class QWidget;

/**
 * @brief 对话框基类
 *
 * 封装对话框的通用操作，减少重复代码。
 * 子类只需关注自身的业务逻辑。
 */
class BaseDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     * @param flags 窗口标志（默认去掉帮助按钮）
     */
    explicit BaseDialog(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    ~BaseDialog() override;

    /**
     * @brief 设置对话框为标准尺寸
     * @param width 宽度
     * @param height 高度
     */
    void setStandardSize(int width, int height);

    /**
     * @brief 显示错误提示
     * @param message 错误信息
     * @param title 标题（默认"错误"）
     */
    void showError(const QString& title, const QString& message = QString());

    /**
     * @brief 显示信息提示
     * @param message 信息内容
     * @param title 标题（默认"提示"）
     */
    void showInfo(const QString& title, const QString& message = QString());

    /**
     * @brief 显示警告提示
     * @param message 警告信息
     * @param title 标题（默认"警告"）
     */
    void showWarning(const QString& title, const QString& message = QString());

    /**
     * @brief 显示确认对话框
     * @param message 确认信息
     * @param title 标题（默认"确认"）
     * @return 用户点击"是"返回 true，否则返回 false
     */
    bool confirm(const QString& title, const QString& message = QString());

protected:
    /**
     * @brief 去掉窗口帮助按钮
     *
     * 在构造函数中自动调用，子类无需重复设置。
     */
    void removeHelpButton();
};

#endif // BASE_DIALOG_H
