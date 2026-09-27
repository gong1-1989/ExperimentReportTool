/**
 * @file ProjectDialog.h
 * @brief 项目编辑对话框头文件
 *
 * 用于新建和编辑项目的对话框，包含名称、类型、描述、状态、负责人等字段。
 * UI 界面使用 Qt Designer (.ui 文件) 可视化设计。
 */

#ifndef PROJECT_DIALOG_H
#define PROJECT_DIALOG_H

#include <QDialog>
#include "core/models/Project.h"

// 前向声明 UI 类（由 uic 工具从 .ui 文件自动生成）
namespace Ui {
class ProjectDialog;
}

/**
 * @brief 项目编辑对话框
 *
 * 使用方式：
 * @code
 *   // 新建项目
 *   ProjectDialog dlg(this);
 *   if (dlg.exec() == QDialog::Accepted) {
 *       Project::Ptr project = dlg.projectData();
 *       ProjectRepository::save(project);
 *   }
 *
 *   // 编辑项目
 *   ProjectDialog dlg(this);
 *   dlg.setProjectData(existingProject);
 *   if (dlg.exec() == QDialog::Accepted) {
 *       Project::Ptr project = dlg.projectData();
 *       ProjectRepository::save(project);
 *   }
 * @endcode
 */
class ProjectDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    explicit ProjectDialog(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ProjectDialog() override;

    /**
     * @brief 获取对话框中的项目数据
     * @return 项目对象智能指针，包含用户输入的所有字段
     */
    Project::Ptr projectData() const;

    /**
     * @brief 设置对话框中的项目数据（用于编辑模式，回填已有数据）
     * @param project 要编辑的项目对象
     */
    void setProjectData(const Project::Ptr& project);

    /**
     * @brief 设置父项目 ID（新建子项目时使用）
     * @param parentId 父项目 ID，-1 表示根项目
     */
    void setParentProjectId(qint64 parentId) { m_parentProjectId = parentId; }

private slots:
    /**
     * @brief 确定按钮点击槽函数
     *
     * 验证用户输入，验证通过后关闭对话框并返回 Accepted
     */
    void on_m_buttonBox_accepted();

private:
    /**
     * @brief 验证用户输入是否合法
     * @return 验证通过返回 true，否则显示错误提示并返回 false
     */
    bool validateInput();

    // -----------------------------------------------------------------------
    // 成员变量
    // -----------------------------------------------------------------------

    Ui::ProjectDialog* ui;             ///< UI 界面对象（从 .ui 文件自动生成）
    qint64 m_parentProjectId;           ///< 父项目 ID（-1 表示根项目）
    qint64 m_editingProjectId;          ///< 正在编辑的项目 ID（-1 表示新建模式）
};

#endif // PROJECT_DIALOG_H
