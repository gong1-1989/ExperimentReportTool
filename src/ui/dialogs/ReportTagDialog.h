/**
 * @file ReportTagDialog.h
 * @brief 报告标签选择对话框头文件
 *
 * 为报告选择标签，支持新建标签。
 */

#ifndef REPORT_TAG_DIALOG_H
#define REPORT_TAG_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QList>

#include "core/models/Tag.h"


// 前向声明 UI 类（由 uic 工具从 .ui 文件自动生成）
namespace Ui {
class ReportTagDialog;
}
/**
 * @brief 报告标签选择对话框
 */
class ReportTagDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param reportId 报告 ID
     * @param parent 父窗口
     */
    explicit ReportTagDialog(qint64 reportId, QWidget* parent = nullptr);
    ~ReportTagDialog() override;

    /**
     * @brief 获取选中的标签 ID 列表
     */
    QList<qint64> selectedTagIds() const;

    /**
     * @brief 获取选中的标签名称列表
     */
    QStringList selectedTagNames() const;

private slots:
    void onNewTag();
    void onItemChanged(QListWidgetItem* item);
    void onSearchTextChanged(const QString& text);
    void onSelectAll();
    void onDeselectAll();

private:

    Ui::ReportTagDialog* ui;  ///< UI 界面对象（从 .ui 文件自动生成）
    void loadTags();
    void loadSelectedTags();
    void updateSelectedLabel();  ///< 更新已选标签数量显示

    // -----------------------------------------------------------------------
    // UI 控件
    // -----------------------------------------------------------------------


    // -----------------------------------------------------------------------
    // 数据
    // -----------------------------------------------------------------------

    qint64 m_reportId;                   ///< 报告 ID
    Tag::List m_allTags;                  ///< 所有标签
    QList<qint64> m_selectedIds;         ///< 已选标签 ID
};

#endif // REPORT_TAG_DIALOG_H
