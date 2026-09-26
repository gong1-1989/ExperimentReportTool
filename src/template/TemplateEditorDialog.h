/**
 * @file TemplateEditorDialog.h
 * @brief 模板编辑器对话框头文件
 *
 * 用于创建和编辑报告模板。
 * 左侧为块组件面板，右侧为模板预览/编辑区（复用 ReportEditor 组件）。
 */

#ifndef TEMPLATE_EDITOR_DIALOG_H
#define TEMPLATE_EDITOR_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QListWidget>
#include <QSplitter>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QList>

#include "core/models/Template.h"
#include "core/models/Report.h"


// 前向声明 UI 类（由 uic 工具从 .ui 文件自动生成）
namespace Ui {
class TemplateEditorDialog;
}
// 前向声明
class ReportEditor;

/**
 * @brief 模板编辑器对话框
 */
class TemplateEditorDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     * @param existingTemplate 要编辑的现有模板（为空则新建）
     */
    explicit TemplateEditorDialog(QWidget* parent = nullptr,
                                    const Template::Ptr& existingTemplate = nullptr);
    /// 析构函数
    ~TemplateEditorDialog() override;

    /**
     * @brief 获取编辑后的模板
     * @return 模板对象
     */
    Template::Ptr templateData() const;

private slots:
    /// 保存模板
    void onSave();
    /// 验证输入
    bool validateInput();

private:

    Ui::TemplateEditorDialog* ui;  ///< UI 界面对象（从 .ui 文件自动生成）
    /// 初始化 UI
    /// 加载模板到编辑器
    void loadTemplate();
    /// 从编辑器收集模板块
    QList<ContentBlock> collectBlocks() const;

    // -----------------------------------------------------------------------
    // UI 控件
    // -----------------------------------------------------------------------

    ReportEditor* m_editor;        ///< 模板块编辑器（复用 ReportEditor）

    // -----------------------------------------------------------------------
    // 数据
    // -----------------------------------------------------------------------

    Template::Ptr m_template;       ///< 正在编辑的模板
    bool m_isNewTemplate;           ///< 是否为新建模板
};

#endif // TEMPLATE_EDITOR_DIALOG_H
