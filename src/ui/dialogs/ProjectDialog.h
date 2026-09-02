/**
 * @file ProjectDialog.h
 * @brief 项目编辑对话框头文件
 *
 * 用于新建和编辑项目的对话框，包含名称、类型、描述、状态、负责人等字段。
 */

#ifndef PROJECT_DIALOG_H
#define PROJECT_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>

#include "core/models/Project.h"

/**
 * @brief 项目编辑对话框
 */
class ProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProjectDialog(QWidget* parent = nullptr);

    /**
     * @brief 获取对话框中的项目数据
     * @return 项目对象指针
     */
    Project::Ptr projectData() const;

    /**
     * @brief 设置对话框中的项目数据（用于编辑模式）
     * @param project 项目对象
     */
    void setProjectData(const Project::Ptr& project);

    /**
     * @brief 设置父项目 ID（新建子项目时使用）
     * @param parentId 父项目 ID
     */
    void setParentProjectId(qint64 parentId) { m_parentProjectId = parentId; }

private slots:
    /// 验证输入并接受对话框
    void onAccept();

private:
    /// 初始化 UI
    void setupUi();

    /// 验证输入
    bool validateInput();

    // -----------------------------------------------------------------------
    // UI 控件
    // -----------------------------------------------------------------------

    QLineEdit* m_nameEdit;        ///< 项目名称
    QLineEdit* m_typeEdit;        ///< 项目类型
    QLineEdit* m_ownerEdit;       ///< 负责人
    QComboBox* m_statusCombo;     ///< 项目状态
    QTextEdit* m_descriptionEdit; ///< 项目描述
    QDialogButtonBox* m_buttonBox;///< 按钮组

    qint64 m_parentProjectId;     ///< 父项目 ID
    qint64 m_editingProjectId;    ///< 正在编辑的项目 ID（-1 表示新建）
};

#endif // PROJECT_DIALOG_H
