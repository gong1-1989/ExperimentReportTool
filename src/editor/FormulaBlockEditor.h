/**
 * @file FormulaBlockEditor.h
 * @brief 公式块编辑器头文件
 *
 * 在报告中显示和编辑 LaTeX 公式块。
 * 双击可打开公式编辑器进行修改。
 */

#ifndef FORMULA_BLOCK_EDITOR_H
#define FORMULA_BLOCK_EDITOR_H

#include "editor/BlockEditor.h"
#include <QLabel>
#include <QTextBrowser>

/**
 * @brief 公式块编辑器
 *
 * 显示 LaTeX 公式，支持双击编辑。
 * 公式内容存储在 block.data["latex"] 中。
 */
class FormulaBlockEditor : public BlockEditor
{
    Q_OBJECT

public:
    explicit FormulaBlockEditor(const ContentBlock& block, QWidget* parent = nullptr);
    ~FormulaBlockEditor() override;

    QJsonObject blockData() const override;
    void setBlockData(const QJsonObject& data) override;
    BlockType blockType() const override { return BlockType::Formula; }
    bool isEmpty() const override { return m_latex.isEmpty(); }

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private slots:
    void onEditFormula();

private:
    void setupFormulaArea();
    void updateDisplay();
    QString generateDisplayHtml();

    // -----------------------------------------------------------------------
    // UI 控件
    // -----------------------------------------------------------------------

    QTextBrowser* m_formulaBrowser;  ///< 公式显示浏览器
    QLabel* m_editHintLabel;          ///< 编辑提示标签

    // -----------------------------------------------------------------------
    // 数据
    // -----------------------------------------------------------------------

    QString m_latex;       ///< LaTeX 公式
    bool m_inline;          ///< 是否行内公式
};

#endif // FORMULA_BLOCK_EDITOR_H
