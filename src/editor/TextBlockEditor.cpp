/**
 * @file TextBlockEditor.cpp
 * @brief 文本块编辑器实现文件
 */

#include "TextBlockEditor.h"
#include "core/utils/Logger.h"
#include "core/utils/AppTheme.h"
#include "core/utils/AppDimensions.h"

#include <QPainter>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextList>
#include <QTextListFormat>
#include <QMimeData>
#include <QApplication>
#include <QTimer>  // 用于延迟更新高度
#include <QClipboard>
#include <QRegularExpression>
#include <QResizeEvent>  // 用于尺寸变化事件

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

    // 关闭水平滚动条，垂直滚动条总是关闭（自动调整高度）
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 接受富文本粘贴
    setAcceptRichText(true);

    // 自动换行（由父容器宽度决定）
    setWordWrapMode(QTextOption::WordWrap);

    // 设置尺寸策略：水平方向扩展，垂直方向根据内容
    // Minimum 意味着 sizeHint 是最小尺寸，布局不会压缩到比 sizeHint 更小
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    // 连接文档内容变化信号（用于自动调整高度）
    // 注意：Qt6 中没有 contentsSizeChanged 信号，使用 contentsChanged 替代
    connect(document(), &QTextDocument::contentsChanged,
            this, &AutoResizeTextEdit::onDocumentSizeChanged);

    // 初始不设置固定高度，让 sizeHint 决定高度
}

QSize AutoResizeTextEdit::sizeHint() const
{
    // 高度根据文档内容计算
    // document()->size().height() 返回文档的理想高度（像素）
    // +8 是上下内边距
    const int docHeight = qCeil(document()->size().height());
    const int height = qMax(m_minHeight, docHeight + 8);
    return QSize(QWIDGETSIZE_MAX, height);
}

QSize AutoResizeTextEdit::minimumSizeHint() const
{
    // 最小高度为 m_minHeight，确保至少能显示一行
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
        painter.setPen(QColor(AppTheme::Color::TextPlaceholder));
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

void AutoResizeTextEdit::resizeEvent(QResizeEvent* event)
{
    QTextEdit::resizeEvent(event);

    // 关键：宽度变化时，document 的文本宽度可能变化，需要重新计算高度
    // 设置 document 的文本宽度为视口宽度，确保 document()->size().height() 正确
    if (document()) {
        document()->setTextWidth(viewport()->width());
    }

    // 延迟更新高度，确保布局完成
    QTimer::singleShot(AppDimensions::Delay::Immediate, this, [this]() {
        updateGeometry();
        const int newHeight = qMax(m_minHeight, qCeil(document()->size().height()) + 8);
        emit heightChanged(newHeight);
    });
}

void AutoResizeTextEdit::onDocumentSizeChanged()
{
    // contentsChanged 信号触发时，通知布局系统重新计算尺寸
    // updateGeometry() 会触发父布局重新调用 sizeHint()
    updateGeometry();

    // 计算新高度并发出信号
    const int newHeight = qMax(m_minHeight, qCeil(document()->size().height()) + 8);
    emit heightChanged(newHeight);
}

/**
 * @brief 公共方法：手动触发高度更新
 *
 * 在字体改变、加载内容等场景下调用，
 * 使用 QTimer::singleShot(0) 延迟到事件循环，确保文档布局完成。
 */
void AutoResizeTextEdit::updateHeight()
{
    QTimer::singleShot(AppDimensions::Delay::Immediate, this, [this]() {
        updateGeometry();
        const int newHeight = qMax(m_minHeight, qCeil(document()->size().height()) + 8);
        emit heightChanged(newHeight);
    });
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

    // 关键：文本编辑控件高度变化时，更新块编辑器的固定高度
    connect(m_textEdit, &AutoResizeTextEdit::heightChanged,
            this, &BlockEditor::updateHeight);

    // 根据块类型设置初始样式和内容
    updateStyleForType();

    // 加载块数据
    if (!block.data.isEmpty()) {
        setBlockData(block.data);
    }

    // 初始更新高度
    updateHeight();
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

    // 加载内容后更新高度
    m_textEdit->updateHeight();
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
        // Qt6 中 fontFamily()/setFontFamily() 已弃用，使用 fontFamilies()/setFontFamilies()
        // fontFamilies() 返回 QVariant，需要用 toStringList() 转换
        const QStringList families = cursor.charFormat().fontFamilies().toStringList();
        if (families.contains("Consolas")) {
            charFormat.setFontFamilies(QStringList{m_textEdit->font().family()});
            charFormat.setBackground(Qt::transparent);
        } else {
            charFormat.setFontFamilies(QStringList{"Consolas"});
            charFormat.setBackground(QColor(AppTheme::Color::BgGray));
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
    int minHeight = AppDimensions::Widget::BlockMinHeight;

    switch (m_textType) {
    case BlockType::Heading1:
        font.setPointSize(AppTheme::Heading::H1);
        font.setBold(true);
        minHeight = 44;
        styleSheet = QString("QTextEdit { color: %1; padding: %2px 0; }")
                         .arg(AppTheme::Color::TextPrimary).arg(AppTheme::Spacing::Normal);
        break;
    case BlockType::Heading2:
        font.setPointSize(AppTheme::Heading::H2);
        font.setBold(true);
        minHeight = 38;
        styleSheet = QString("QTextEdit { color: %1; padding: %2px 0; }")
                         .arg(AppTheme::Color::TextPrimary).arg(AppTheme::Spacing::Medium);
        break;
    case BlockType::Heading3:
        font.setPointSize(AppTheme::Heading::H3);
        font.setBold(true);
        minHeight = 34;
        styleSheet = QString("QTextEdit { color: %1; padding: %2px 0; }")
                         .arg(AppTheme::Color::Gray333).arg(AppTheme::Spacing::Small);
        break;
    case BlockType::Paragraph:
        font.setPointSize(AppTheme::FontSize::Normal);
        font.setBold(false);
        styleSheet = QString("QTextEdit { color: %1; line-height: 1.6; padding: %2px 0; }")
                         .arg(AppTheme::Color::Gray333).arg(AppTheme::Spacing::Tiny);
        break;
    case BlockType::BulletList:
    case BlockType::NumberedList:
        font.setPointSize(AppTheme::FontSize::Normal);
        font.setBold(false);
        styleSheet = QString("QTextEdit { color: %1; padding: %2px 0; }")
                         .arg(AppTheme::Color::Gray333).arg(AppTheme::Spacing::Tiny);
        break;
    case BlockType::Quote:
        font.setPointSize(AppTheme::FontSize::Normal);
        font.setItalic(true);
        styleSheet = QString("QTextEdit { color: %1; border-left: 3px solid %2; "
                             "padding-left: %3px; margin-left: %4px; }")
                         .arg(AppTheme::Color::Gray666)
                         .arg(AppTheme::Color::GrayDDD)
                         .arg(AppTheme::Spacing::Large)
                         .arg(AppTheme::Spacing::Normal);
        break;
    default:
        font.setPointSize(AppTheme::FontSize::Normal);
        styleSheet = QString("QTextEdit { color: %1; }").arg(AppTheme::Color::Gray333);
        break;
    }

    m_textEdit->setFont(font);
    m_textEdit->setStyleSheet(styleSheet);
    m_textEdit->setPlaceholderText(placeholderForType());

    // 设置最小高度
    m_textEdit->setMinHeight(minHeight);
    m_textEdit->setMinimumHeight(minHeight);

    // 字体改变后，文档大小可能变化，需要重新计算高度
    // updateHeight() 内部使用 QTimer::singleShot(0) 延迟更新，确保文档布局完成
    m_textEdit->updateHeight();
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
