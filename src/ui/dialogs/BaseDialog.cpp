/**
 * @file BaseDialog.cpp
 * @brief 对话框基类实现文件
 */

#include "BaseDialog.h"

// ===========================================================================
// 构造与析构
// ===========================================================================

BaseDialog::BaseDialog(QWidget* parent, Qt::WindowFlags flags)
    : QDialog(parent, flags)
{
    // 默认去掉帮助按钮（保留在构造函数中，子类无需重复设置）
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

BaseDialog::~BaseDialog() = default;
