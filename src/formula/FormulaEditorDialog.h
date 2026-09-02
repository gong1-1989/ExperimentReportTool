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
     */
    QString formula() const { return m_latexEdit->toPlainText(); }

    /**
     * @brief 获取公式显示模式（行内/块级）
     */
    bool isInline() const { return m_inlineCombo->currentIndex() == 0; }

private slots:
    void onPreview();
    void onInsertTemplate(int index);
    void onTextChanged();

private:
    void setupUi();
    void updatePreview();
    QString generatePreviewHtml(const QString& latex);

    // -----------------------------------------------------------------------
    // UI 控件
    // -----------------------------------------------------------------------

    QTextEdit* m_latexEdit;           ///< LaTeX 输入框
    QTextBrowser* m_previewBrowser;   ///< 预览浏览器
    QSplitter* m_splitter;             ///< 分割器

    QComboBox* m_templateCombo;       ///< 公式模板选择
    QComboBox* m_inlineCombo;         ///< 行内/块级选择
    QPushButton* m_previewBtn;         ///< 预览按钮
    QPushButton* m_okBtn;              ///< 确定按钮
    QPushButton* m_cancelBtn;          ///< 取消按钮

    QLabel* m_statusLabel;             ///< 状态标签

    // -----------------------------------------------------------------------
    // 常用公式模板
    // -----------------------------------------------------------------------

    static const QStringList s_templates;  ///< 公式模板列表
};

#endif // FORMULA_EDITOR_DIALOG_H
