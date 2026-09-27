/**
 * @file FormulaEditorDialog.cpp
 * @brief 公式编辑器对话框实现文件
 */

#include "FormulaEditorDialog.h"
#include "ui_FormulaEditorDialog.h"  // 由 uic 工具从 .ui 文件自动生成
#include "core/utils/Logger.h"
#include "core/utils/AppTheme.h"
#include "core/utils/AppDimensions.h"

#include <QMessageBox>
#include <QTimer>
#include <QApplication>
#include <QClipboard>

// ===========================================================================
// 常用公式模板
// ===========================================================================

const QStringList FormulaEditorDialog::s_templates = {
    "选择模板...",
    "分数: \\frac{a}{b}",
    "平方根: \\sqrt{x}",
    "n次根: \\sqrt[n]{x}",
    "上标: x^{2}",
    "下标: x_{i}",
    "求和: \\sum_{i=1}^{n} x_i",
    "积分: \\int_{a}^{b} f(x)dx",
    "极限: \\lim_{x \\to \\infty}",
    "导数: \\frac{dy}{dx}",
    "偏导: \\frac{\\partial f}{\\partial x}",
    "矩阵: \\begin{pmatrix} a & b \\\\ c & d \\end{pmatrix}",
    "方程组: \\begin{cases} x + y = 1 \\\\ x - y = 0 \\end{cases}",
    "希腊字母: \\alpha \\beta \\gamma \\pi \\theta",
    "箭头: \\rightarrow \\leftarrow \\Rightarrow",
    "不等式: \\leq \\geq \\neq \\approx",
    "质能方程: E = mc^2",
    "牛顿第二定律: F = ma",
    "欧姆定律: V = IR",
};

// ===========================================================================
// 构造与析构
// ===========================================================================

FormulaEditorDialog::FormulaEditorDialog(const QString& initialLatex, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::FormulaEditorDialog)
{
    ui->setupUi(this);

    if (!initialLatex.isEmpty()) {
        ui->m_latexEdit->setPlainText(initialLatex);
    }

    setWindowTitle(tr("公式编辑器"));
    resize(AppDimensions::Window::DialogMediumWidth, AppDimensions::Window::DialogSmallHeight);

    // 延迟更新预览
    QTimer::singleShot(AppDimensions::Delay::Fast, this, &FormulaEditorDialog::updatePreview);
}

FormulaEditorDialog::~FormulaEditorDialog()
{
    delete ui;
}

// ===========================================================================
// 公共方法
// ===========================================================================

QString FormulaEditorDialog::formula() const
{
    // 通过 ui 指针访问 .ui 文件中定义的 LaTeX 编辑框
    // 此函数不能在头文件中内联实现，因为 Ui::FormulaEditorDialog 在头文件中只有前向声明
    return ui->m_latexEdit->toPlainText();
}

bool FormulaEditorDialog::isInline() const
{
    // 通过 ui 指针访问 .ui 文件中定义的模式下拉框
    // 索引 0 表示行内模式，索引 1 表示块级模式
    return ui->m_inlineCombo->currentIndex() == 0;
}

// ===========================================================================
// 预览更新
// ===========================================================================

void FormulaEditorDialog::on_m_previewBtn_clicked()
{
    updatePreview();
}

void FormulaEditorDialog::on_m_latexEdit_textChanged()
{
    // 防抖：500ms 后更新预览
    QTimer::singleShot(AppDimensions::Delay::Medium, this, &FormulaEditorDialog::updatePreview);
}

void FormulaEditorDialog::updatePreview()
{
    const QString latex = ui->m_latexEdit->toPlainText().trimmed();
    ui->m_previewBrowser->setHtml(generatePreviewHtml(latex));
}

QString FormulaEditorDialog::generatePreviewHtml(const QString& latex)
{
    if (latex.isEmpty()) {
        return QString("<div style='color: %1; text-align: center; margin-top: 50px;'>"
                       "<p style='font-size: %2px;'>∑</p>"
                       "<p>输入 LaTeX 公式后在此预览</p>"
                       "</div>")
            .arg(AppTheme::Color::TextSecondary)
            .arg(AppTheme::FontSize::Massive);
    }

    // 转义 HTML 特殊字符
    const QString escaped = latex.toHtmlEscaped();

    // 使用 MathJax CDN 渲染
    // 行内公式用 $...$，块级公式用 $$...$$
    const QString formulaTag = isInline()
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
        "body { font-family: 'Microsoft YaHei', sans-serif; padding: 20px; }\n"
        ".formula-container { text-align: center; padding: 30px; background: %1; "
        "border-radius: %2px; min-height: 100px; display: flex; align-items: center; "
        "justify-content: center; }\n"
        ".formula { font-size: 20px; }\n"
        ".source { margin-top: 16px; padding: 12px; background: %3; border-radius: %4px; "
        "font-family: Consolas, monospace; font-size: %5px; color: %6; word-break: break-all; }\n"
        ".source-label { font-size: %7px; color: %8; margin-bottom: 4px; }\n"
        "</style>\n"
        "</head>\n<body>\n"
        "<div class='formula-container'>\n"
        "<div class='formula'>%9</div>\n"
        "</div>\n"
        "<div class='source'>\n"
        "<div class='source-label'>LaTeX 源码:</div>\n"
        "%10\n"
        "</div>\n"
        "</body>\n</html>"
    ).arg(AppTheme::Color::BgGray)
     .arg(AppTheme::Radius::Large)
     .arg(AppTheme::Color::BgGray)
     .arg(AppTheme::Radius::Small)
     .arg(AppTheme::FontSize::Small)
     .arg(AppTheme::Color::Gray666)
     .arg(AppTheme::FontSize::ExtraSmall)
     .arg(AppTheme::Color::TextSecondary)
     .arg(formulaTag, escaped);
}

// ===========================================================================
// 模板插入
// ===========================================================================

void FormulaEditorDialog::on_m_templateCombo_currentIndexChanged(int index)
{
    if (index <= 0) return;  // 第一项是"选择模板..."

    const QString templateText = s_templates.at(index);
    // 提取模板中的 LaTeX 部分（格式: "描述: latex"）
    const int colonPos = templateText.indexOf(':');
    if (colonPos > 0) {
        const QString latex = templateText.mid(colonPos + 1).trimmed();
        ui->m_latexEdit->setPlainText(latex);
    }

    // 重置选择
    ui->m_templateCombo->setCurrentIndex(0);
}

void FormulaEditorDialog::on_m_inlineCombo_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    updatePreview();
}

void FormulaEditorDialog::on_m_okBtn_clicked()
{
    accept();
}

void FormulaEditorDialog::on_m_cancelBtn_clicked()
{
    reject();
}
