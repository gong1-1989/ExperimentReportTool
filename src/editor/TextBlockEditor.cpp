/**
 * @file TextBlockEditor.cpp
 * @brief 文本块编辑器实现文件
 */

#include "TextBlockEditor.h"
#include "core/utils/Logger.h"

#include <QPainter>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QTextBlock>
#include <QTextList>
#include <QTextListFormat>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>
#include <QRegularExpression>

// ===========================================================================
// AutoResizeTextEdit 实现
// ===========================================================================

AutoResizeTextEdit::AutoResizeTextEdit(QWidget* parent)
    : QTextEdit(parent)
    , m_minHeight(32)
{
    // 无边框、透明背景
    setFrameStyle(QFrame::NoFrame);
    setStyleSheet("QTextEdit { background: transparent; border: none; }");

    // 关闭水平滚动条，垂直滚动条按需
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 接受富文本粘贴
    setAcceptRichText(true);

    // 不自动换行（由父容器宽度决定）
    setWordWrapMode(QTextOption::WordWrap);

    // 连接文档大小变化信号
    QAbstractTextDocumentLayout *layout = document()->documentLayout();
    connect(layout, &QAbstractTextDocumentLayout::documentSizeChanged,
            this, &AutoResizeTextEdit::onDocumentSizeChanged);

    // 设置初始高度
    setFixedHeight(m_minHeight);
}

QSize AutoResizeTextEdit::sizeHint() const
{
    // 高度根据文档内容计算
    const int docHeight = qCeil(document()->size().height());
    const int height = qMax(m_minHeight, docHeight + 8);  // +8 上下内边距
    return QSize(QWIDGETSIZE_MAX, height);
}

QSize AutoResizeTextEdit::minimumSizeHint() const
{
    return QSize(0, m_minHeight);
}

void AutoResizeTextEdit::setPlaceholderText(const QString& text)
{
    m_placeholder = text;
    viewport()->update();
}

void AutoResizeTextEdit::paintEvent(QPaintEvent* event)
{
    // 先绘制默认内容
    QTextEdit::paintEvent(event);

    // 如果文档为空，绘制占位符
    if (document()->isEmpty() && !m_placeholder.isEmpty()) {
        QPainter painter(viewport());
        painter.setPen(QColor("#aaa"));
        QFont f = font();
        f.setItalic(true);
        painter.setFont(f);

        // 在文本起始位置绘制
        const QRect rect = viewport()->rect().adjusted(4, 4, -4, -4);
        painter.drawText(rect, Qt::AlignLeft | Qt::AlignTop, m_placeholder);
    }
}

void AutoResizeTextEdit::keyPressEvent(QKeyEvent* event)
{
    QTextEdit::keyPressEvent(event);
    // 按键后可能高度变化，在 onDocumentSizeChanged 中处理
}

void AutoResizeTextEdit::insertFromMimeData(const QMimeData* source)
{
    // 粘贴时优先使用纯文本，避免外部格式污染
    // 如果是富文本（如从本编辑器复制），保留格式
    if (source->hasHtml() && source->html().contains("data-block-type")) {
        QTextEdit::insertFromMimeData(source);
    } else if (source->hasText()) {
        // 纯文本粘贴
        textCursor().insertText(source->text());
    } else {
        QTextEdit::insertFromMimeData(source);
    }
}

void AutoResizeTextEdit::onDocumentSizeChanged(const QSizeF& size)
{
    Q_UNUSED(size);
    const int newHeight = qMax(m_minHeight, qCeil(document()->size().height()) + 8);
    if (newHeight != height()) {
        setFixedHeight(newHeight);
        emit heightChanged(newHeight);
    }
}

// ===========================================================================
// TextBlockEditor 实现
// ===========================================================================

TextBlockEditor::TextBlockEditor(const ContentBlock& block, QWidget* parent)
    : BlockEditor(block, parent)
    , m_textEdit(nullptr)
    , m_textType(block.type)
    , m_loadingData(false)
{
    setupEditor();

    // 创建文本编辑控件
    m_textEdit = new AutoResizeTextEdit(this);
    contentContainer()->addWidget(m_textEdit);

    // 连接信号
    connect(m_textEdit, &QTextEdit::textChanged,
            this, &TextBlockEditor::onTextChanged);

    // 根据块类型设置初始样式和内容
    updateStyleForType();

    // 加载块数据
    if (!block.data.isEmpty()) {
        setBlockData(block.data);
    }
}

// ===========================================================================
// BlockEditor 接口实现
// ===========================================================================

QJsonObject TextBlockEditor::blockData() const
{
    QJsonObject data;
    data["text"] = m_textEdit->toHtml();
    data["plain_text"] = m_textEdit->toPlainText();

    // 列表项（如果是列表类型）
    if (m_textType == BlockType::BulletList || m_textType == BlockType::NumberedList) {
        QJsonArray items;
        QTextBlock block = m_textEdit->document()->firstBlock();
        while (block.isValid()) {
            if (!block.text().isEmpty()) {
                items.append(block.text());
            }
            block = block.next();
        }
        data["items"] = items;
    }

    // 对齐方式
    const Qt::Alignment align = m_textEdit->alignment();
    if (align & Qt::AlignCenter) data["alignment"] = "center";
    else if (align & Qt::AlignRight) data["alignment"] = "right";
    else if (align & Qt::AlignJustify) data["alignment"] = "justify";
    else data["alignment"] = "left";

    return data;
}

void TextBlockEditor::setBlockData(const QJsonObject& data)
{
    m_loadingData = true;

    // 设置文本内容（优先 HTML，降级纯文本）
    if (data.contains("text")) {
        const QString text = data.value("text").toString();
        if (text.contains("<") && text.contains(">")) {
            m_textEdit->setHtml(text);
        } else {
            m_textEdit->setPlainText(text);
        }
    } else if (data.contains("plain_text")) {
        m_textEdit->setPlainText(data.value("plain_text").toString());
    }

    // 列表项
    if (data.contains("items") && data.value("items").isArray()) {
        const QJsonArray items = data.value("items").toArray();
        QStringList lines;
        for (const QJsonValue& item : items) {
            lines.append(item.toString());
        }
        m_textEdit->setPlainText(lines.join("\n"));
    }

    // 对齐方式
    if (data.contains("alignment")) {
        const QString align = data.value("alignment").toString();
        if (align == "center") m_textEdit->setAlignment(Qt::AlignCenter);
        else if (align == "right") m_textEdit->setAlignment(Qt::AlignRight);
        else if (align == "justify") m_textEdit->setAlignment(Qt::AlignJustify);
        else m_textEdit->setAlignment(Qt::AlignLeft);
    }

    m_loadingData = false;
}

BlockType TextBlockEditor::blockType() const
{
    return m_textType;
}

QString TextBlockEditor::plainText() const
{
    return m_textEdit->toPlainText();
}

void TextBlockEditor::setFocusToEditor()
{
    m_textEdit->setFocus();
    // 将光标移到末尾
    QTextCursor cursor = m_textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_textEdit->setTextCursor(cursor);
}

bool TextBlockEditor::isEmpty() const
{
    return m_textEdit->toPlainText().trimmed().isEmpty();
}

// ===========================================================================
// 文本块特有方法
// ===========================================================================

QString TextBlockEditor::toHtml() const
{
    return m_textEdit->toHtml();
}

QString TextBlockEditor::toPlainText() const
{
    return m_textEdit->toPlainText();
}

void TextBlockEditor::setHtml(const QString& html)
{
    m_textEdit->setHtml(html);
}

void TextBlockEditor::setPlainText(const QString& text)
{
    m_textEdit->setPlainText(text);
}

void TextBlockEditor::setTextBlockType(BlockType type)
{
    if (m_textType == type) return;

    m_textType = type;
    setBlockType(type);
    updateStyleForType();
    notifyContentChanged();
}

void TextBlockEditor::applyFormat(const QString& format)
{
    QTextCursor cursor = m_textEdit->textCursor();
    QTextCharFormat charFormat;

    if (format == "bold") {
        charFormat.setFontWeight(cursor.charFormat().fontWeight() == QFont::Bold
                                     ? QFont::Normal : QFont::Bold);
    } else if (format == "italic") {
        charFormat.setFontItalic(!cursor.charFormat().fontItalic());
    } else if (format == "underline") {
        charFormat.setFontUnderline(!cursor.charFormat().fontUnderline());
    } else if (format == "strikethrough") {
        charFormat.setFontStrikeOut(!cursor.charFormat().fontStrikeOut());
    } else if (format == "code") {
        // 行内代码：等宽字体 + 背景色
        if (cursor.charFormat().fontFamily() == "Consolas") {
            charFormat.setFontFamily(m_textEdit->font().family());
            charFormat.setBackground(Qt::transparent);
        } else {
            charFormat.setFontFamily("Consolas");
            charFormat.setBackground(QColor("#f0f0f0"));
        }
    }

    cursor.mergeCharFormat(charFormat);
    m_textEdit->setTextCursor(cursor);
}

// ===========================================================================
// 样式更新
// ===========================================================================

void TextBlockEditor::updateStyleForType()
{
    QFont font = m_textEdit->font();
    QString styleSheet;
    int minHeight = 32;

    switch (m_textType) {
    case BlockType::Heading1:
        font.setPointSize(22);
        font.setBold(true);
        minHeight = 44;
        styleSheet = "QTextEdit { color: #1a1a1a; padding: 8px 0; }";
        break;
    case BlockType::Heading2:
        font.setPointSize(18);
        font.setBold(true);
        minHeight = 38;
        styleSheet = "QTextEdit { color: #2a2a2a; padding: 6px 0; }";
        break;
    case BlockType::Heading3:
        font.setPointSize(15);
        font.setBold(true);
        minHeight = 34;
        styleSheet = "QTextEdit { color: #333; padding: 4px 0; }";
        break;
    case BlockType::Paragraph:
        font.setPointSize(14);
        font.setBold(false);
        styleSheet = "QTextEdit { color: #333; line-height: 1.6; padding: 2px 0; }";
        break;
    case BlockType::BulletList:
    case BlockType::NumberedList:
        font.setPointSize(14);
        font.setBold(false);
        styleSheet = "QTextEdit { color: #333; padding: 2px 0; }";
        break;
    case BlockType::Quote:
        font.setPointSize(14);
        font.setItalic(true);
        styleSheet = "QTextEdit { color: #666; border-left: 3px solid #ddd; "
                     "padding-left: 12px; margin-left: 8px; }";
        break;
    default:
        font.setPointSize(14);
        styleSheet = "QTextEdit { color: #333; }";
        break;
    }

    m_textEdit->setFont(font);
    m_textEdit->setStyleSheet(styleSheet);
    m_textEdit->setPlaceholderText(placeholderForType());

    // 设置最小高度
    m_textEdit->setMinimumHeight(minHeight);
}

QString TextBlockEditor::placeholderForType() const
{
    switch (m_textType) {
    case BlockType::Heading1:     return tr("一级标题");
    case BlockType::Heading2:     return tr("二级标题");
    case BlockType::Heading3:     return tr("三级标题");
    case BlockType::Paragraph:    return tr("输入正文，支持 Markdown 快捷输入...");
    case BlockType::BulletList:   return tr("列表项");
    case BlockType::NumberedList: return tr("列表项");
    case BlockType::Quote:        return tr("引用内容");
    default:                       return tr("输入内容...");
    }
}

// ===========================================================================
// 信号槽
// ===========================================================================

void TextBlockEditor::onTextChanged()
{
    if (m_loadingData) return;

    // 检查 Markdown 快捷输入
    checkMarkdownShortcuts();

    // 更新列表编号
    if (m_textType == BlockType::NumberedList) {
        updateListNumbering();
    }

    notifyContentChanged();
}

void TextBlockEditor::checkMarkdownShortcuts()
{
    // 仅在段落类型时检测快捷输入
    if (m_textType != BlockType::Paragraph) return;

    const QString text = m_textEdit->toPlainText();

    // 定义 Markdown 快捷输入规则
    struct MarkdownRule {
        QRegularExpression regex;
        BlockType targetType;
    };

    const QList<MarkdownRule> rules = {
        {QRegularExpression("^#\\s"), BlockType::Heading1},
        {QRegularExpression("^##\\s"), BlockType::Heading2},
        {QRegularExpression("^###\\s"), BlockType::Heading3},
        {QRegularExpression("^[-*]\\s"), BlockType::BulletList},
        {QRegularExpression("^\\d+\\.\\s"), BlockType::NumberedList},
        {QRegularExpression("^>\\s"), BlockType::Quote},
    };

    for (const MarkdownRule& rule : rules) {
        QRegularExpressionMatch match = rule.regex.match(text);
        if (match.hasMatch()) {
            // 移除前缀标记，转换块类型
            const QString prefix = match.captured(0);
            const QString remaining = text.mid(prefix.length());

            m_loadingData = true;
            m_textEdit->setPlainText(remaining);
            m_loadingData = false;

            setTextBlockType(rule.targetType);
            return;
        }
    }
}

void TextBlockEditor::updateListNumbering()
{
    // 有序列表自动编号（简化实现，实际渲染由 QTextDocument 处理）
    // 这里主要确保每行以数字开头
    QTextCursor cursor = m_textEdit->textCursor();
    QTextBlock block = m_textEdit->document()->firstBlock();
    int number = 1;

    while (block.isValid()) {
        const QString blockText = block.text();
        if (!blockText.isEmpty()) {
            // 检查是否已有编号
            QRegularExpression re("^(\\d+)\\.\\s*");
            QRegularExpressionMatch match = re.match(blockText);
            if (match.hasMatch()) {
                // 替换为正确编号
                const QString newText = QString("%1. %2")
                    .arg(number)
                    .arg(blockText.mid(match.captured(0).length()));
                // 注意：这里不直接修改，避免光标跳动
                // 实际项目中可以在失去焦点时统一更新
            }
            ++number;
        }
        block = block.next();
    }
}

// ===========================================================================
// 键盘事件
// ===========================================================================

void TextBlockEditor::keyPressEvent(QKeyEvent* event)
{
    // 先处理通用键（Enter、Backspace、方向键等）
    if (handleCommonKeyPress(event)) {
        return;
    }

    // -----------------------------------------------------------------------
    // 文本格式快捷键
    // -----------------------------------------------------------------------
    const Qt::KeyboardModifiers mods = event->modifiers();

    if (mods == Qt::ControlModifier) {
        switch (event->key()) {
        case Qt::Key_B:
            applyFormat("bold");
            event->accept();
            return;
        case Qt::Key_I:
            applyFormat("italic");
            event->accept();
            return;
        case Qt::Key_U:
            applyFormat("underline");
            event->accept();
            return;
        case Qt::Key_E:
            applyFormat("code");
            event->accept();
            return;
        // Ctrl+0~3：转换为段落/标题
        case Qt::Key_0:
            setTextBlockType(BlockType::Paragraph);
            event->accept();
            return;
        case Qt::Key_1:
            setTextBlockType(BlockType::Heading1);
            event->accept();
            return;
        case Qt::Key_2:
            setTextBlockType(BlockType::Heading2);
            event->accept();
            return;
        case Qt::Key_3:
            setTextBlockType(BlockType::Heading3);
            event->accept();
            return;
        default:
            break;
        }
    }

    // Ctrl+Shift+7/8：列表
    if (mods == (Qt::ControlModifier | Qt::ShiftModifier)) {
        if (event->key() == Qt::Key_7) {
            setTextBlockType(BlockType::NumberedList);
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_8) {
            setTextBlockType(BlockType::BulletList);
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_C) {
            setTextBlockType(BlockType::CodeBlock);
            event->accept();
            return;
        }
    }

    // Tab：列表缩进（预留）
    if (event->key() == Qt::Key_Tab) {
        event->accept();
        return;
    }

    // 其他键交给 QTextEdit 处理
    // 注意：我们不直接调用 QTextEdit 的事件，因为 m_textEdit 是子控件
    // 这里的 keyPressEvent 是 BlockEditor（QWidget）的，通常不会触发
    // 实际按键由 m_textEdit 处理
    QWidget::keyPressEvent(event);
}
