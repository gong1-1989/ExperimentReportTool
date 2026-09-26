/**
 * @file TagManagerDialog.h
 * @brief 标签管理对话框头文件
 *
 * 管理所有标签：查看、新建、编辑、删除。
 * 显示标签名称、颜色、使用次数。
 */

#ifndef TAG_MANAGER_DIALOG_H
#define TAG_MANAGER_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QColor>
#include <QComboBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

#include "core/models/Tag.h"


// 前向声明 UI 类（由 uic 工具从 .ui 文件自动生成）
namespace Ui {
class TagManagerDialog;
}
/**
 * @brief 标签管理对话框
 */
class TagManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TagManagerDialog(QWidget* parent = nullptr);
    ~TagManagerDialog() override;

private slots:
    void onTagSelected(QListWidgetItem* item);
    void onNewTag();
    void onEditTag();
    void onDeleteTag();
    void onSaveTag();
    void onCancelEdit();
    void onSearchTextChanged(const QString& text);
    void onColorSelected(int index);

private:

    Ui::TagManagerDialog* ui;  ///< UI 界面对象（从 .ui 文件自动生成）
    void loadTags(const QString& filter = QString());
    void updateTagList();
    void clearEditForm();
    void setEditMode(bool editing);
    QString colorSwatchHtml(const QString& color, int size = 16);

    // -----------------------------------------------------------------------
    // UI 控件
    // -----------------------------------------------------------------------


    // 编辑表单

    // -----------------------------------------------------------------------
    // 数据
    // -----------------------------------------------------------------------

    Tag::List m_tags;                    ///< 当前标签列表
    Tag::Ptr m_currentTag;               ///< 当前选中/编辑的标签
    bool m_editing;                       ///< 是否处于编辑模式
};

#endif // TAG_MANAGER_DIALOG_H
