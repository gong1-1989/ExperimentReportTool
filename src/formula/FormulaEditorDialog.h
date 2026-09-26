/**
 * @file FormulaEditorDialog.h
 * @brief 公式编辑器对话框头文件
 *
 * 支持 LaTeX 公式输入和实时预览。
 * 预览使用 MathJax（通过 QTextBrowser + CDN），
 * 离线时显示 LaTeX 源码。
 */

#ifndef FORMULA_EDITOR_DIALOG_H
#define FORMULA_EDITOR_DIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QTextBrowser>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStringList>


// 前向声明 UI 类（由 uic 工具从 .ui 文件自动生成）
namespace Ui {
class FormulaEditorDialog;
}
/**
 * @brief 公式编辑器对话框
 *
 * 使用方式：
 * @code
 *   FormulaEditorDialog dialog("E = mc^2", this);
 *   if (dialog.exec() == QDialog::Accepted) {
 *       QString latex = dialog.formula();
 *   }
 * @endcode
 */
class FormulaEditorDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param initialLatex 初始 LaTeX 公式
     * @param parent 父窗口
     */
    explicit FormulaEditorDialog(const QString& initialLatex = QString(),
                                  QWidget* parent = nullptr);
    ~FormulaEditorDialog() override;

    /**
     * @brief 获取编辑后的 LaTeX 公式
     *
     * @note 此函数在 .cpp 中实现，因为需要访问 ui 对象的完整定义
     */
    QString formula() const;

    /**
     * @brief 获取公式显示模式（行内/块级）
     *
     * @note 此函数在 .cpp 中实现，因为需要访问 ui 对象的完整定义
     */
    bool isInline() const;

private slots:
    void onPreview();
    void onInsertTemplate(int index);
    void onTextChanged();

private:

    Ui::FormulaEditorDialog* ui;  ///< UI 界面对象（从 .ui 文件自动生成）
    void updatePreview();
    QString generatePreviewHtml(const QString& latex);

    // -----------------------------------------------------------------------
    // UI 控件
    // -----------------------------------------------------------------------




    // -----------------------------------------------------------------------
    // 常用公式模板
    // -----------------------------------------------------------------------

    static const QStringList s_templates;  ///< 公式模板列表
};

#endif // FORMULA_EDITOR_DIALOG_H
