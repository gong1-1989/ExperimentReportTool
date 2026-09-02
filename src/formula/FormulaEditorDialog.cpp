/**
 * @file FormulaEditorDialog.cpp
 * @brief 公式编辑器对话框实现文件
 */

#include "FormulaEditorDialog.h"
#include "core/utils/Logger.h"

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
    , m_latexEdit(nullptr)
    , m_previewBrowser(nullptr)
    , m_splitter(nullptr)
    , m_templateCombo(nullptr)
    , m_inlineCombo(nullptr)
    , m_previewBtn(nullptr)
    , m_okBtn(nullptr)
    , m_cancelBtn(nullptr)
    , m_statusLabel(nullptr)
{
    setupUi();

    if (!initialLatex.isEmpty()) {
        m_latexEdit->setPlainText(initialLatex);
    }

    setWindowTitle(tr("公式编辑器"));
    resize(800, 500);

    // 延迟更新预览
    QTimer::singleShot(100, this, &FormulaEditorDialog::updatePreview);
}

FormulaEditorDialog::~FormulaEditorDialog()
{
}

// ===========================================================================
// UI 初始化
// ===========================================================================

void FormulaEditorDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // -----------------------------------------------------------------------
    // 工具栏
    // -----------------------------------------------------------------------
    QHBoxLayout* toolbar = new QHBoxLayout();
    toolbar->setSpacing(8);

    toolbar->addWidget(new QLabel(tr("模板:"), this));
    m_templateCombo = new QComboBox(this);
    m_templateCombo->addItems(s_templates);
    m_templateCombo->setMinimumWidth(200);
    toolbar->addWidget(m_templateCombo);

    toolbar->addSpacing(20);
    toolbar->addWidget(new QLabel(tr("模式:"), this));
    m_inlineCombo = new QComboBox(this);
    m_inlineCombo->addItem(tr("行内公式"));
    m_inlineCombo->addItem(tr("块级公式"));
    toolbar->addWidget(m_inlineCombo);

    toolbar->addStretch();

    m_previewBtn = new QPushButton(tr("刷新预览"), this);
    toolbar->addWidget(m_previewBtn);

    mainLayout->addLayout(toolbar);

    // -----------------------------------------------------------------------
    // 编辑区 + 预览区（分割器）
    // -----------------------------------------------------------------------
    m_splitter = new QSplitter(Qt::Horizontal, this);

    // 左侧：LaTeX 输入
    QWidget* editContainer = new QWidget(this);
    QVBoxLayout* editLayout = new QVBoxLayout(editContainer);
    editLayout->setContentsMargins(0, 0, 0, 0);
    editLayout->setSpacing(4);

    QLabel* editLabel = new QLabel(tr("LaTeX 输入"), this);
    editLabel->setStyleSheet("font-weight: bold; color: #333; padding: 4px;");
    editLayout->addWidget(editLabel);

    m_latexEdit = new QTextEdit(this);
    m_latexEdit->setPlaceholderText(tr("在此输入 LaTeX 公式...\n\n例如: E = mc^2\n      \\frac{a}{b}"));
    m_latexEdit->setStyleSheet(
        "QTextEdit { border: 1px solid #ddd; border-radius: 4px; "
        "font-family: Consolas, Monaco, monospace; font-size: 13px; padding: 8px; }");
    editLayout->addWidget(m_latexEdit, 1);

    m_splitter->addWidget(editContainer);

    // 右侧：预览
    QWidget* previewContainer = new QWidget(this);
    QVBoxLayout* previewLayout = new QVBoxLayout(previewContainer);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(4);

    QLabel* previewLabel = new QLabel(tr("公式预览"), this);
    previewLabel->setStyleSheet("font-weight: bold; color: #333; padding: 4px;");
    previewLayout->addWidget(previewLabel);

    m_previewBrowser = new QTextBrowser(this);
    m_previewBrowser->setStyleSheet(
        "QTextBrowser { border: 1px solid #ddd; border-radius: 4px; padding: 12px; "
        "background: white; }");
    m_previewBrowser->setOpenExternalLinks(true);
    previewLayout->addWidget(m_previewBrowser, 1);

    m_splitter->addWidget(previewContainer);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({400, 400});

    mainLayout->addWidget(m_splitter, 1);

    // -----------------------------------------------------------------------
    // 状态栏
    // -----------------------------------------------------------------------
    m_statusLabel = new QLabel(tr("提示: 使用 MathJax 渲染公式，需要网络连接"), this);
    m_statusLabel->setStyleSheet("color: #666; font-size: 12px; padding: 4px 0;");
    mainLayout->addWidget(m_statusLabel);

    // -----------------------------------------------------------------------
    // 底部按钮
    // -----------------------------------------------------------------------
    QHBoxLayout* buttonBar = new QHBoxLayout();
    buttonBar->setSpacing(8);

    buttonBar->addStretch();

    m_cancelBtn = new QPushButton(tr("取消"), this);
    buttonBar->addWidget(m_cancelBtn);

    m_okBtn = new QPushButton(tr("确定"), this);
    m_okBtn->setStyleSheet(
        "QPushButton { background: #4A90D9; color: white; padding: 6px 20px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #357ABD; }");
    m_okBtn->setDefault(true);
    buttonBar->addWidget(m_okBtn);

    mainLayout->addLayout(buttonBar);

    // -----------------------------------------------------------------------
    // 连接信号
    // -----------------------------------------------------------------------
    connect(m_okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_previewBtn, &QPushButton::clicked, this, &FormulaEditorDialog::onPreview);
    connect(m_templateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FormulaEditorDialog::onInsertTemplate);
    connect(m_latexEdit, &QTextEdit::textChanged, this, &FormulaEditorDialog::onTextChanged);
}

// ===========================================================================
// 预览更新
// ===========================================================================

void FormulaEditorDialog::onPreview()
{
    updatePreview();
}

void FormulaEditorDialog::onTextChanged()
{
    // 防抖：500ms 后更新预览
    QTimer::singleShot(500, this, &FormulaEditorDialog::updatePreview);
}

void FormulaEditorDialog::updatePreview()
{
    const QString latex = m_latexEdit->toPlainText().trimmed();
    m_previewBrowser->setHtml(generatePreviewHtml(latex));
}

QString FormulaEditorDialog::generatePreviewHtml(const QString& latex)
{
    if (latex.isEmpty()) {
        return "<div style='color: #999; text-align: center; margin-top: 50px;'>"
               "<p style='font-size: 48px;'>∑</p>"
               "<p>输入 LaTeX 公式后在此预览</p>"
               "</div>";
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
        ".formula-container { text-align: center; padding: 30px; background: #fafafa; "
        "border-radius: 8px; min-height: 100px; display: flex; align-items: center; "
        "justify-content: center; }\n"
        ".formula { font-size: 20px; }\n"
        ".source { margin-top: 16px; padding: 12px; background: #f5f5f5; border-radius: 4px; "
        "font-family: Consolas, monospace; font-size: 12px; color: #666; word-break: break-all; }\n"
        ".source-label { font-size: 11px; color: #999; margin-bottom: 4px; }\n"
        "</style>\n"
        "</head>\n<body>\n"
        "<div class='formula-container'>\n"
        "<div class='formula'>%1</div>\n"
        "</div>\n"
        "<div class='source'>\n"
        "<div class='source-label'>LaTeX 源码:</div>\n"
        "%2\n"
        "</div>\n"
        "</body>\n</html>"
    ).arg(formulaTag, escaped);
}

// ===========================================================================
// 模板插入
// ===========================================================================

void FormulaEditorDialog::onInsertTemplate(int index)
{
    if (index <= 0) return;  // 第一项是"选择模板..."

    const QString templateText = s_templates.at(index);
    // 提取模板中的 LaTeX 部分（格式: "描述: latex"）
    const int colonPos = templateText.indexOf(':');
    if (colonPos > 0) {
        const QString latex = templateText.mid(colonPos + 1).trimmed();
        m_latexEdit->setPlainText(latex);
    }

    // 重置选择
    m_templateCombo->setCurrentIndex(0);
}
