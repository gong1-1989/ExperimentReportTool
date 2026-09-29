/**
 * @file UiHelper.cpp
 * @brief 全局 UI 提示工具类实现
 *
 * 所有按钮文本均使用自定义中文按钮（addButton），
 * 不依赖 Qt 翻译文件（qtbase_zh_CN.qm），确保任何部署环境下按钮均为中文。
 */

#include "UiHelper.h"

#include <QMessageBox>
#include <QWidget>
#include <QPushButton>
#include <QAbstractButton>
#include <QHash>

void UiHelper::error(QWidget* parent, const QString& title, const QString& message)
{
    QMessageBox box(QMessageBox::Critical,
                    title.isEmpty() ? QObject::tr("错误") : title,
                    message,
                    QMessageBox::NoButton,
                    parent);
    box.addButton(QObject::tr("确定"), QMessageBox::AcceptRole);
    box.exec();
}

void UiHelper::info(QWidget* parent, const QString& title, const QString& message)
{
    QMessageBox box(QMessageBox::Information,
                    title.isEmpty() ? QObject::tr("提示") : title,
                    message,
                    QMessageBox::NoButton,
                    parent);
    box.addButton(QObject::tr("确定"), QMessageBox::AcceptRole);
    box.exec();
}

void UiHelper::warning(QWidget* parent, const QString& title, const QString& message)
{
    QMessageBox box(QMessageBox::Warning,
                    title.isEmpty() ? QObject::tr("警告") : title,
                    message,
                    QMessageBox::NoButton,
                    parent);
    box.addButton(QObject::tr("确定"), QMessageBox::AcceptRole);
    box.exec();
}

QMessageBox::StandardButton UiHelper::warning(QWidget* parent, const QString& title,
                                              const QString& message,
                                              QMessageBox::StandardButtons buttons,
                                              QMessageBox::StandardButton defaultButton)
{
    // 自定义中文按钮，避免依赖 Qt 翻译文件
    QMessageBox box(QMessageBox::Warning,
                    title.isEmpty() ? QObject::tr("警告") : title,
                    message,
                    QMessageBox::NoButton,
                    parent);

    // 按钮 → 标准值映射（用于返回用户点击的按钮）
    QHash<QPushButton*, QMessageBox::StandardButton> mapping;
    const auto addBtn = [&](QMessageBox::StandardButton value, const QString& text,
                            QMessageBox::ButtonRole role) {
        if (buttons & value) {
            QPushButton* btn = box.addButton(text, role);
            mapping.insert(btn, value);
            if (defaultButton == value) {
                box.setDefaultButton(btn);
            }
        }
    };

    addBtn(QMessageBox::Save,    QObject::tr("保存"), QMessageBox::AcceptRole);
    addBtn(QMessageBox::Discard, QObject::tr("放弃"), QMessageBox::DestructiveRole);
    addBtn(QMessageBox::Cancel,  QObject::tr("取消"), QMessageBox::RejectRole);
    addBtn(QMessageBox::Ok,      QObject::tr("确定"), QMessageBox::AcceptRole);
    addBtn(QMessageBox::Yes,     QObject::tr("是"),   QMessageBox::YesRole);
    addBtn(QMessageBox::No,      QObject::tr("否"),   QMessageBox::NoRole);

    box.exec();
    const QAbstractButton* clicked = box.clickedButton();
    const auto it = mapping.find(const_cast<QPushButton*>(static_cast<const QPushButton*>(clicked)));
    return it != mapping.end() ? it.value() : defaultButton;
}

bool UiHelper::confirm(QWidget* parent, const QString& title, const QString& message)
{
    // 自定义中文按钮，避免依赖 Qt 翻译文件
    QMessageBox box(QMessageBox::Question,
                    title.isEmpty() ? QObject::tr("确认") : title,
                    message,
                    QMessageBox::NoButton,
                    parent);
    QPushButton* yesBtn = box.addButton(QObject::tr("是"), QMessageBox::YesRole);
    box.addButton(QObject::tr("否"), QMessageBox::NoRole);
    box.setDefaultButton(yesBtn);

    box.exec();
    return box.clickedButton() == yesBtn;
}


void UiHelper::centerTableWidget(QTableWidget* table)
{
    if (!table) return;
    // 单元格内容水平+垂直居中（委托方式，QSS text-align 对表格不生效）
    table->setItemDelegate(new CenteredItemDelegate(table));
    // 表头内容居中
    if (QHeaderView* hh = table->horizontalHeader()) {
        hh->setDefaultAlignment(Qt::AlignCenter);
    }
}

void UiHelper::centerTableWidget(QTableView* table)
{
    if (!table) return;
    // 单元格内容水平+垂直居中（QTableView + Model 场景）
    table->setItemDelegate(new CenteredItemDelegate(table));
    if (QHeaderView* hh = table->horizontalHeader()) {
        hh->setDefaultAlignment(Qt::AlignCenter);
    }
}
