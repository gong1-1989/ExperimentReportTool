/**
 * @file BaseDialog.cpp
 * @brief 对话框基类实现文件
 */

#include "BaseDialog.h"

#include "ui/UiHelper.h"
#include <QWidget>

// ===========================================================================
// 构造与析构
// ===========================================================================

BaseDialog::BaseDialog(QWidget* parent, Qt::WindowFlags flags)
    : QDialog(parent, flags)
{
    // 默认去掉帮助按钮
    removeHelpButton();
}

BaseDialog::~BaseDialog() = default;

// ===========================================================================
// 窗口设置
// ===========================================================================

void BaseDialog::setStandardSize(int width, int height)
{
    resize(width, height);
}

void BaseDialog::removeHelpButton()
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

// ===========================================================================
// 消息提示（统一委托 UiHelper，保持标题/样式一致）
// ===========================================================================

void BaseDialog::showError(const QString& title, const QString& message)
{
    UiHelper::error(this, title, message);
}

void BaseDialog::showInfo(const QString& title, const QString& message)
{
    UiHelper::info(this, title, message);
}

void BaseDialog::showWarning(const QString& title, const QString& message)
{
    UiHelper::warning(this, title, message);
}

bool BaseDialog::confirm(const QString& title, const QString& message)
{
    return UiHelper::confirm(this, title, message);
}
