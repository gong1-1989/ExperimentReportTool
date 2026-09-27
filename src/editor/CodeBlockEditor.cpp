/**
 * @file CodeBlockEditor.cpp
 * @brief 代码块编辑器实现文件
 */

#include "CodeBlockEditor.h"
#include "core/utils/AppDimensions.h"

#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>

// ===========================================================================
// 构造函数
// ===========================================================================

CodeBlockEditor::CodeBlockEditor(const ContentBlock& block, QWidget* parent)
    : BlockEditor(block, parent)
    , m_codeEdit(nullptr)
    , m_languageCombo(nullptr)
    , m_copyBtn(nullptr)
    , m_language("plaintext")
{
    setupEditor();

    QVBoxLayout* layout = contentContainer();

    // 工具栏
    QHBoxLayout* toolbar = new QHBoxLayout();
    m_languageCombo = new QComboBox(this);
    m_languageCombo->addItems({
        "Plain Text", "C/C++", "Python", "Java", "JavaScript",
        "TypeScript", "Go", "Rust", "SQL", "Shell", "JSON", "XML", "HTML/CSS"
    });
    m_copyBtn = new QPushButton(tr("复制"), this);
    toolbar->addWidget(new QLabel(tr("语言:"), this));
    toolbar->addWidget(m_languageCombo);
    toolbar->addStretch();
    toolbar->addWidget(m_copyBtn);
    layout->addLayout(toolbar);

    // 代码编辑区
    m_codeEdit = new QPlainTextEdit(this);
    m_codeEdit->setFont(QFont("Consolas", 11));
    m_codeEdit->setStyleSheet(
        "QPlainTextEdit { background-color: #1e1e1e; color: #d4d4d4; "
        "border: 1px solid #333; border-radius: 4px; padding: 8px; "
        "selection-background-color: #264f78; }");
    m_codeEdit->setPlaceholderText(tr("在此输入代码..."));
    m_codeEdit->setMinimumHeight(AppDimensions::Widget::CodeMinHeight);
    m_codeEdit->setMaximumHeight(AppDimensions::Widget::CodeMaxHeight);
    layout->addWidget(m_codeEdit);

    connect(m_copyBtn, &QPushButton::clicked, this, &CodeBlockEditor::onCopyCode);
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CodeBlockEditor::onLanguageChanged);
    connect(m_codeEdit, &QPlainTextEdit::textChanged, this, [this]() {
        notifyContentChanged();
    });

    if (!block.data.isEmpty()) {
        setBlockData(block.data);
    }

    updateHeight();
}

// ===========================================================================
// 数据存取
// ===========================================================================

QJsonObject CodeBlockEditor::blockData() const
{
    QJsonObject data;
    data["code"] = m_codeEdit->toPlainText();
    data["language"] = m_language;
    return data;
}

void CodeBlockEditor::setBlockData(const QJsonObject& data)
{
    m_codeEdit->setPlainText(data.value("code").toString());
    m_language = data.value("language").toString("plaintext");

    const QStringList langs = {"plaintext", "cpp", "python", "java", "javascript",
                                 "typescript", "go", "rust", "sql", "shell", "json", "xml", "html"};
    const int idx = langs.indexOf(m_language);
    if (idx >= 0) m_languageCombo->setCurrentIndex(idx);
}

// ===========================================================================
// 槽函数
// ===========================================================================

void CodeBlockEditor::onCopyCode()
{
    QApplication::clipboard()->setText(m_codeEdit->toPlainText());
    m_copyBtn->setText(tr("已复制!"));
    QTimer::singleShot(AppDimensions::Delay::ButtonReset, this, [this]() {
        m_copyBtn->setText(tr("复制"));
    });
}

void CodeBlockEditor::onLanguageChanged(int index)
{
    const QStringList langs = {"plaintext", "cpp", "python", "java", "javascript",
                                 "typescript", "go", "rust", "sql", "shell", "json", "xml", "html"};
    if (index >= 0 && index < langs.size()) {
        m_language = langs[index];
    }
    notifyContentChanged();
}
