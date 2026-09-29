/**
 * @file BaseDialog.h
 * @brief 对话框基类
 *
 * 所有对话框的基类，提供通用功能：
 * - 自动去掉窗口帮助按钮（ContextHelpButtonHint）
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
     * @brief 构造函数（自动去掉窗口帮助按钮）
     * @param parent 父窗口
     * @param flags 窗口标志
     */
    explicit BaseDialog(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    ~BaseDialog() override;
};

#endif // BASE_DIALOG_H
