/**
 * @file BlockEditor.cpp
 * @brief 内容块编辑器基类实现文件
 */

#include "BlockEditor.h"
#include "core/utils/Logger.h"

#include <QPainter>
#include <QStyleOption>
#include <QApplication>

// ===========================================================================
// 构造与析构
// ===========================================================================

BlockEditor::BlockEditor(const ContentBlock& block, QWidget* parent)
    : QWidget(parent)
    , m_block(block)
    , m_readOnly(false)
    , m_selected(false)
    , m_actionMoveUp(nullptr)
    , m_actionMoveDown(nullptr)
    , m_actionConvert(nullptr)
    , m_actionDelete(nullptr)
    , m_actionDuplicate(nullptr)
    , m_handleButton(nullptr)
    , m_contentLayout(nullptr)
    , m_mainLayout(nullptr)
{
    // 块编辑器默认接受焦点
    setFocusPolicy(Qt::StrongFocus);

    // 设置最小高度，避免块太矮难以点击
    setMinimumHeight(32);

    // 设置对象名，方便 QSS 样式选择
    setObjectName("blockEditor");
}

BlockEditor::~BlockEditor()
{
}

// ===========================================================================
// 初始化 UI
// ===========================================================================

void BlockEditor::setupEditor()
{
    // 主布局：左侧手柄 + 右侧内容
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 2, 0, 2);
    m_mainLayout->setSpacing(4);

    // 左侧操作手柄
    createHandle();
    m_mainLayout->addWidget(m_handleButton);

    // 右侧内容容器
    QWidget* contentWidget = new QWidget(this);
    m_contentLayout = new QVBoxLayout(contentWidget);
    m_contentLayout->setContentsMargins(4, 0, 4, 0);
    m_contentLayout->setSpacing(0);
    m_mainLayout->addWidget(contentWidget, 1);  // 内容区占剩余空间
}

void BlockEditor::createHandle()
{
    m_handleButton = new QPushButton(this);
    m_handleButton->setFixedSize(20, 24);
    m_handleButton->setCursor(Qt::SizeAllCursor);
    m_handleButton->setFlat(true);
    m_handleButton->setToolTip(tr("拖拽移动 / 点击打开菜单"));
    m_handleButton->setStyleSheet(
        "QPushButton { border: none; border-radius: 3px; color: #999; "
        "font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #e8e8e8; color: #333; }"
    );
    m_handleButton->setText("⋮⋮");  // 拖拽手柄图标

    // -----------------------------------------------------------------------
    // 创建菜单动作
    // -----------------------------------------------------------------------

    m_actionMoveUp = new QAction(tr("上移"), this);
    m_actionMoveUp->setShortcut(QKeySequence("Alt+Up"));
    connect(m_actionMoveUp, &QAction::triggered, this, [this]() {
        emit requestMoveUp(this);
    });

    m_actionMoveDown = new QAction(tr("下移"), this);
    m_actionMoveDown->setShortcut(QKeySequence("Alt+Down"));
    connect(m_actionMoveDown, &QAction::triggered, this, [this]() {
        emit requestMoveDown(this);
    });

    m_actionConvert = new QAction(tr("转换为..."), this);
    connect(m_actionConvert, &QAction::triggered, this, [this]() {
        showBlockTypeMenu(m_handleButton->mapToGlobal(QPoint(0, m_handleButton->height())));
    });

    m_actionDuplicate = new QAction(tr("复制块"), this);
    m_actionDuplicate->setShortcut(QKeySequence("Ctrl+D"));
    // 复制块的实现由 ReportEditor 处理（需要访问块列表）
    // 这里先预留，实际在 ReportEditor 中连接

    m_actionDelete = new QAction(tr("删除块"), this);
    m_actionDelete->setShortcut(QKeySequence("Ctrl+Backspace"));
    connect(m_actionDelete, &QAction::triggered, this, [this]() {
        emit requestDeleteBlock(this);
    });

    // 点击手柄显示菜单
    connect(m_handleButton, &QPushButton::clicked, this, [this]() {
        QMenu menu(this);
        menu.addAction(m_actionMoveUp);
        menu.addAction(m_actionMoveDown);
        menu.addSeparator();
        menu.addAction(m_actionConvert);
        menu.addAction(m_actionDuplicate);
        menu.addSeparator();
        menu.addAction(m_actionDelete);
        menu.exec(m_handleButton->mapToGlobal(QPoint(0, m_handleButton->height())));
    });

    // 初始隐藏手柄（鼠标悬停时显示）
    m_handleButton->hide();
}

// ===========================================================================
// 数据访问
// ===========================================================================

ContentBlock BlockEditor::contentBlock() const
{
    ContentBlock block;
    block.id = m_block.id;
    block.type = blockType();
    block.data = blockData();
    return block;
}

void BlockEditor::setBlockType(BlockType type)
{
    m_block.type = type;
}

// ===========================================================================
// 状态设置
// ===========================================================================

void BlockEditor::setBlockSelected(bool selected)
{
    m_selected = selected;
    setStyleSheet(selected
        ? "#blockEditor { background-color: #e8f0fe; border-radius: 4px; }"
        : "#blockEditor { background-color: transparent; }");
    update();
}

void BlockEditor::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
    if (m_handleButton) {
        m_handleButton->setVisible(!readOnly);
    }
}

// ===========================================================================
// 键盘事件处理
// ===========================================================================

bool BlockEditor::handleCommonKeyPress(QKeyEvent* event)
{
    // 只读模式不处理编辑键
    if (m_readOnly) return false;

    const int key = event->key();
    const Qt::KeyboardModifiers modifiers = event->modifiers();

    // -----------------------------------------------------------------------
    // Enter：在当前块之后插入新段落
    // -----------------------------------------------------------------------
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        if (modifiers == Qt::NoModifier) {
            emit requestInsertBlockAfter(this, BlockType::Paragraph);
            return true;
        }
        // Shift+Enter：在当前块之前插入
        if (modifiers == Qt::ShiftModifier) {
            emit requestInsertBlockBefore(this, BlockType::Paragraph);
            return true;
        }
    }

    // -----------------------------------------------------------------------
    // Backspace：空块时删除当前块，并将焦点移到上一个块
    // -----------------------------------------------------------------------
    if (key == Qt::Key_Backspace && modifiers == Qt::NoModifier) {
        if (isEmpty()) {
            emit requestDeleteBlock(this);
            emit requestFocusPrevious(this);
            return true;
        }
    }

    // -----------------------------------------------------------------------
    // 方向键：块间导航
    // -----------------------------------------------------------------------
    if (key == Qt::Key_Up && modifiers == Qt::NoModifier) {
        // 子类可以判断光标是否在第一行，这里基类简单处理
        // 实际的 TextBlockEditor 会更精确地判断
        emit requestFocusPrevious(this);
        return true;
    }

    if (key == Qt::Key_Down && modifiers == Qt::NoModifier) {
        emit requestFocusNext(this);
        return true;
    }

    // -----------------------------------------------------------------------
    // Alt+Up/Down：移动块
    // -----------------------------------------------------------------------
    if (key == Qt::Key_Up && modifiers == Qt::AltModifier) {
        emit requestMoveUp(this);
        return true;
    }

    if (key == Qt::Key_Down && modifiers == Qt::AltModifier) {
        emit requestMoveDown(this);
        return true;
    }

    // -----------------------------------------------------------------------
    // Ctrl+D：复制块（预留，实际在 ReportEditor 处理）
    // -----------------------------------------------------------------------
    if (key == Qt::Key_D && modifiers == Qt::ControlModifier) {
        // 复制块需要 ReportEditor 介入，这里不直接处理
        // 子类可以重写此行为
        return false;
    }

    return false;  // 未处理，交给子类
}

// ===========================================================================
// 块类型转换菜单
// ===========================================================================

void BlockEditor::showBlockTypeMenu(const QPoint& pos)
{
    QMenu menu(this);
    menu.setTitle(tr("转换为"));

    // 定义可转换的块类型列表
    struct BlockTypeItem {
        QString name;
        BlockType type;
        QString shortcut;
    };

    const QList<BlockTypeItem> types = {
        {tr("正文"), BlockType::Paragraph, "Ctrl+0"},
        {tr("一级标题"), BlockType::Heading1, "Ctrl+1"},
        {tr("二级标题"), BlockType::Heading2, "Ctrl+2"},
        {tr("三级标题"), BlockType::Heading3, "Ctrl+3"},
        {tr("无序列表"), BlockType::BulletList, "Ctrl+Shift+8"},
        {tr("有序列表"), BlockType::NumberedList, "Ctrl+Shift+7"},
        {tr("引用"), BlockType::Quote, ""},
        {tr("代码块"), BlockType::CodeBlock, "Ctrl+Shift+C"},
        {tr("分割线"), BlockType::Divider, ""},
    };

    for (const BlockTypeItem& item : types) {
        QAction* action = menu.addAction(item.name);
        if (!item.shortcut.isEmpty()) {
            action->setShortcut(QKeySequence(item.shortcut));
        }
        // 标记当前类型（禁用）
        if (item.type == blockType()) {
            action->setEnabled(false);
            action->setCheckable(true);
            action->setChecked(true);
        }
        connect(action, &QAction::triggered, this, [this, item]() {
            emit requestConvertBlock(this, item.type);
        });
    }

    menu.exec(pos);
}

// ===========================================================================
// 内容变化通知
// ===========================================================================

void BlockEditor::notifyContentChanged()
{
    emit contentChanged();
}

// ===========================================================================
// 事件处理
// ===========================================================================

void BlockEditor::focusInEvent(QFocusEvent* event)
{
    Q_UNUSED(event);
    emit blockFocused(this);
    setBlockSelected(true);
}

void BlockEditor::enterEvent(QEnterEvent* event)
{
    Q_UNUSED(event);
    if (m_handleButton && !m_readOnly) {
        m_handleButton->show();
    }
}

void BlockEditor::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    if (m_handleButton && !m_selected) {
        m_handleButton->hide();
    }
}
