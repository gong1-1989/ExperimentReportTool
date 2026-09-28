/**
 * @file FormulaBlockEditor.cpp
 * @brief 公式块编辑器实现文件
 */

#include "FormulaBlockEditor.h"
#include "ui/dialogs/FormulaEditorDialog.h"
#include "core/utils/Logger.h"
#include "core/utils/AppTheme.h"
#include "core/utils/AppDimensions.h"

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
        QString("QWidget { background: %1; border: 1px solid %2; "
                "border-radius: %3px; }"
                "QWidget:hover { border-color: %4; }")
            .arg(AppTheme::Color::BgGray)
            .arg(AppTheme::Color::Border)
            .arg(AppTheme::Radius::Medium)
            .arg(AppTheme::Color::Primary));

    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(AppTheme::Spacing::ExtraLarge, AppTheme::Spacing::Large,
                               AppTheme::Spacing::ExtraLarge, AppTheme::Spacing::Large);
    layout->setSpacing(AppTheme::Spacing::Small);

    // 公式显示区域
    m_formulaBrowser = new QTextBrowser(container);
    m_formulaBrowser->setFrameShape(QFrame::NoFrame);
    m_formulaBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_formulaBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_formulaBrowser->setStyleSheet(
        "QTextBrowser { background: transparent; border: none; padding: 0; }");
    m_formulaBrowser->setOpenExternalLinks(true);
    m_formulaBrowser->setMinimumHeight(AppDimensions::Widget::FormulaMinHeight);
    layout->addWidget(m_formulaBrowser, 1);

    // 编辑提示
    m_editHintLabel = new QLabel(tr("双击编辑公式"), container);
    m_editHintLabel->setStyleSheet(
        QString("QLabel { color: %1; font-size: %2px; background: transparent; border: none; }")
            .arg(AppTheme::Color::TextSecondary).arg(AppTheme::FontSize::ExtraSmall));
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
        m_formulaBrowser->setMinimumHeight(AppDimensions::Widget::FormulaMinHeight);
    }
}

QString FormulaBlockEditor::generateDisplayHtml()
{
    if (m_latex.isEmpty()) {
        return QString("<div style='color: %1; text-align: center; padding: %2px; "
                       "font-size: %3px;'>∑ 双击添加公式</div>")
            .arg(AppTheme::Color::TextPlaceholder)
            .arg(AppTheme::Spacing::Large)
            .arg(AppTheme::FontSize::Normal);
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
