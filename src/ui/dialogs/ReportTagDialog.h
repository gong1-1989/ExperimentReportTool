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

#include "core/models/Tag.h"

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
    void setupUi();
    void loadTags();
    void loadSelectedTags();
    void updateSelectedLabel();

    // -----------------------------------------------------------------------
    // UI 控件
    // -----------------------------------------------------------------------

    QLineEdit* m_searchEdit;           ///< 搜索框
    QListWidget* m_tagList;             ///< 标签列表（可勾选）
    QPushButton* m_newTagBtn;           ///< 新建标签按钮
    QPushButton* m_selectAllBtn;        ///< 全选按钮
    QPushButton* m_deselectAllBtn;      ///< 全不选按钮
    QPushButton* m_okBtn;                ///< 确定按钮
    QPushButton* m_cancelBtn;            ///< 取消按钮
    QLabel* m_selectedLabel;             ///< 已选标签显示

    // -----------------------------------------------------------------------
    // 数据
    // -----------------------------------------------------------------------

    qint64 m_reportId;                   ///< 报告 ID
    Tag::List m_allTags;                  ///< 所有标签
    QList<qint64> m_selectedIds;         ///< 已选标签 ID
};

#endif // REPORT_TAG_DIALOG_H
