/**
 * @file FormulaBlockEditor.cpp
 * @brief 公式块编辑器实现文件
 */

#include "FormulaBlockEditor.h"
#include "formula/FormulaEditorDialog.h"
#include "core/utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>

// ===========================================================================
// 构造与析构
// ===========================================================================

FormulaBlockEditor::FormulaBlockEditor(const ContentBlock& block, QWidget* parent)
    : BlockEditor(block, parent)
    , m_formulaBrowser(nullptr)
    , m_editHintLabel(nullptr)
    , m_latex("")
    , m_inline(false)
{
    setupEditor();
    setupFormulaArea();

    if (!block.data.isEmpty()) {
        setBlockData(block.data);
    }
}

FormulaBlockEditor::~FormulaBlockEditor()
{
}

// ===========================================================================
// UI 初始化
// ===========================================================================

void FormulaBlockEditor::setupFormulaArea()
{
    // 公式容器
    QWidget* container = new QWidget(this);
    container->setStyleSheet(
        "QWidget { background: #fafbfc; border: 1px solid #e1e4e8; "
        "border-radius: 6px; }"
        "QWidget:hover { border-color: #4A90D9; }");

    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(4);

    // 公式显示区域
    m_formulaBrowser = new QTextBrowser(container);
    m_formulaBrowser->setFrameShape(QFrame::NoFrame);
    m_formulaBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_formulaBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_formulaBrowser->setStyleSheet(
        "QTextBrowser { background: transparent; border: none; padding: 0; }");
    m_formulaBrowser->setOpenExternalLinks(true);
    m_formulaBrowser->setMinimumHeight(60);
    layout->addWidget(m_formulaBrowser, 1);

    // 编辑提示
    m_editHintLabel = new QLabel(tr("双击编辑公式"), container);
    m_editHintLabel->setStyleSheet(
        "QLabel { color: #999; font-size: 11px; background: transparent; border: none; }");
    m_editHintLabel->setAlignment(Qt::AlignRight);
    layout->addWidget(m_editHintLabel);

    contentContainer()->addWidget(container);

    updateDisplay();
}

// ===========================================================================
// 数据存取
// ===========================================================================

QJsonObject FormulaBlockEditor::blockData() const
{
    QJsonObject data;
    data["latex"] = m_latex;
    data["inline"] = m_inline;
    return data;
}

void FormulaBlockEditor::setBlockData(const QJsonObject& data)
{
    m_latex = data.value("latex").toString("");
    m_inline = data.value("inline").toBool(false);
    updateDisplay();
}

// ===========================================================================
// 显示更新
// ===========================================================================

void FormulaBlockEditor::updateDisplay()
{
    m_formulaBrowser->setHtml(generateDisplayHtml());

    // 调整高度
    if (m_latex.isEmpty()) {
        m_formulaBrowser->setMinimumHeight(50);
    } else {
        m_formulaBrowser->setMinimumHeight(60);
    }
}

QString FormulaBlockEditor::generateDisplayHtml()
{
    if (m_latex.isEmpty()) {
        return "<div style='color: #bbb; text-align: center; padding: 10px; "
               "font-size: 14px;'>∑ 双击添加公式</div>";
    }

    const QString escaped = m_latex.toHtmlEscaped();
    const QString formulaTag = m_inline
        ? QString("\\(%1\\)").arg(escaped)
        : QString("\\[%1\\]").arg(escaped);

    return QString(
        "<!DOCTYPE html>\n"
        "<html>\n<head>\n"
        "<meta charset='utf-8'>\n"
        "<script>\n"
        "window.MathJax = {\n"
        "  tex: { inlineMath: [['\\\\(', '\\\\)']], displayMath: [['\\\\[', '\\\\]']] },\n"
        "  svg: { fontCache: 'global' }\n"
        "};\n"
        "</script>\n"
        "<script async src='https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-svg.js'></script>\n"
        "<style>\n"
        "body { margin: 0; padding: 8px; font-family: 'Microsoft YaHei', sans-serif; }\n"
        ".formula { text-align: center; font-size: 18px; padding: 8px 0; }\n"
        ".source { display: none; }\n"
        "</style>\n"
        "</head>\n<body>\n"
        "<div class='formula'>%1</div>\n"
        "</body>\n</html>"
    ).arg(formulaTag);
}

// ===========================================================================
// 事件处理
// ===========================================================================

void FormulaBlockEditor::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        onEditFormula();
        return;
    }
    BlockEditor::mouseDoubleClickEvent(event);
}

// ===========================================================================
// 编辑公式
// ===========================================================================

void FormulaBlockEditor::onEditFormula()
{
    FormulaEditorDialog dialog(m_latex, this);
    if (!m_inline) {
        dialog.setModal(true);
    }

    if (dialog.exec() == QDialog::Accepted) {
        m_latex = dialog.formula();
        m_inline = dialog.isInline();
        updateDisplay();
        notifyContentChanged();
    }
}
