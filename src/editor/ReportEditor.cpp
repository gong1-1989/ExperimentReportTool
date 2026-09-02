/**
 * @file ReportEditor.cpp
 * @brief 报告编辑器主组件实现文件
 */

#include "ReportEditor.h"
#include "editor/TextBlockEditor.h"
#include "editor/OtherBlockEditors.h"
#include "editor/AutoSaveManager.h"
#include "core/utils/Logger.h"

#include <QScrollBar>
#include <QMenu>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>

// ===========================================================================
// 构造与析构
// ===========================================================================

ReportEditor::ReportEditor(QWidget* parent)
    : QWidget(parent)
    , m_titleEdit(nullptr)
    , m_statusCombo(nullptr)
    , m_dateEdit(nullptr)
    , m_authorEdit(nullptr)
    , m_addBlockBtn(nullptr)
    , m_undoBtn(nullptr)
    , m_redoBtn(nullptr)
    , m_scrollArea(nullptr)
    , m_blocksContainer(nullptr)
    , m_blocksLayout(nullptr)
    , m_wordCountLabel(nullptr)
    , m_blockCountLabel(nullptr)
    , m_saveStatusLabel(nullptr)
    , m_currentBlockIndex(-1)
    , m_modified(false)
    , m_readOnly(false)
    , m_loading(false)
    , m_autoSave(nullptr)
{
    setupUi();

    // 初始化自动保存管理器
    m_autoSave = new AutoSaveManager(this);
    connect(m_autoSave, &AutoSaveManager::saveTriggered,
            this, &ReportEditor::saveRequested);
    connect(m_autoSave, &AutoSaveManager::saveStateChanged,
            this, &ReportEditor::saveStateChanged);
    connect(this, &ReportEditor::contentChanged,
            m_autoSave, &AutoSaveManager::notifyContentChanged);
}

ReportEditor::~ReportEditor()
{
    clearBlocks();
}

// ===========================================================================
// UI 初始化
// ===========================================================================

void ReportEditor::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // -----------------------------------------------------------------------
    // 顶部标题栏
    // -----------------------------------------------------------------------
    QWidget* headerWidget = new QWidget(this);
    headerWidget->setStyleSheet("QWidget { background-color: #f8f9fa; border-bottom: 1px solid #e0e0e0; }");
    QVBoxLayout* headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 12, 20, 12);
    headerLayout->setSpacing(8);

    // 标题输入
    m_titleEdit = new QLineEdit(headerWidget);
    m_titleEdit->setPlaceholderText(tr("输入报告标题..."));
    m_titleEdit->setStyleSheet(
        "QLineEdit { font-size: 22px; font-weight: bold; border: none; "
        "background: transparent; padding: 4px 0; }"
        "QLineEdit:focus { border-bottom: 2px solid #4A90D9; }");
    headerLayout->addWidget(m_titleEdit);

    // 元信息行
    QHBoxLayout* metaLayout = new QHBoxLayout();
    metaLayout->setSpacing(12);

    m_statusCombo = new QComboBox(headerWidget);
    m_statusCombo->addItem(tr("草稿"), static_cast<int>(ReportStatus::Draft));
    m_statusCombo->addItem(tr("已提交"), static_cast<int>(ReportStatus::Submitted));
    m_statusCombo->addItem(tr("已审核"), static_cast<int>(ReportStatus::Reviewed));
    m_statusCombo->setStyleSheet("QComboBox { padding: 2px 8px; }");
    metaLayout->addWidget(new QLabel(tr("状态:"), headerWidget));
    metaLayout->addWidget(m_statusCombo);

    m_dateEdit = new QDateEdit(QDate::currentDate(), headerWidget);
    m_dateEdit->setDisplayFormat("yyyy-MM-dd");
    m_dateEdit->setCalendarPopup(true);
    metaLayout->addWidget(new QLabel(tr("实验日期:"), headerWidget));
    metaLayout->addWidget(m_dateEdit);

    m_authorEdit = new QLineEdit(headerWidget);
    m_authorEdit->setPlaceholderText(tr("作者"));
    m_authorEdit->setMaximumWidth(150);
    metaLayout->addWidget(new QLabel(tr("作者:"), headerWidget));
    metaLayout->addWidget(m_authorEdit);

    metaLayout->addStretch();

    // 工具栏按钮
    m_addBlockBtn = new QPushButton(tr("+ 添加块"), headerWidget);
    m_addBlockBtn->setStyleSheet(
        "QPushButton { padding: 4px 12px; border: 1px solid #ddd; "
        "border-radius: 4px; background: white; }"
        "QPushButton:hover { background: #f0f0f0; }");
    metaLayout->addWidget(m_addBlockBtn);

    headerLayout->addLayout(metaLayout);
    mainLayout->addWidget(headerWidget);

    // -----------------------------------------------------------------------
    // 滚动区域（块编辑器容器）
    // -----------------------------------------------------------------------
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet("QScrollArea { background: white; }");

    m_blocksContainer = new QWidget();
    m_blocksContainer->setStyleSheet("QWidget { background: white; }");
    m_blocksLayout = new QVBoxLayout(m_blocksContainer);
    m_blocksLayout->setContentsMargins(40, 20, 40, 20);
    m_blocksLayout->setSpacing(2);
    m_blocksLayout->addStretch();  // 底部弹性空间

    m_scrollArea->setWidget(m_blocksContainer);
    mainLayout->addWidget(m_scrollArea, 1);

    // -----------------------------------------------------------------------
    // 底部状态栏
    // -----------------------------------------------------------------------
    QWidget* statusWidget = new QWidget(this);
    statusWidget->setStyleSheet("QWidget { background-color: #f8f9fa; border-top: 1px solid #e0e0e0; }");
    QHBoxLayout* statusLayout = new QHBoxLayout(statusWidget);
    statusLayout->setContentsMargins(20, 6, 20, 6);

    m_wordCountLabel = new QLabel(tr("字数: 0"), statusWidget);
    m_wordCountLabel->setStyleSheet("color: #666; font-size: 12px;");
    statusLayout->addWidget(m_wordCountLabel);

    m_blockCountLabel = new QLabel(tr("块: 0"), statusWidget);
    m_blockCountLabel->setStyleSheet("color: #666; font-size: 12px;");
    statusLayout->addWidget(m_blockCountLabel);

    statusLayout->addStretch();

    m_saveStatusLabel = new QLabel(tr("已保存"), statusWidget);
    m_saveStatusLabel->setStyleSheet("color: #67C23A; font-size: 12px;");
    statusLayout->addWidget(m_saveStatusLabel);

    mainLayout->addWidget(statusWidget);

    // -----------------------------------------------------------------------
    // 连接信号
    // -----------------------------------------------------------------------
    connect(m_titleEdit, &QLineEdit::textChanged, this, &ReportEditor::onTitleChanged);
    connect(m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReportEditor::onStatusChanged);
    connect(m_dateEdit, &QDateEdit::dateChanged, this, &ReportEditor::onDateChanged);
    connect(m_authorEdit, &QLineEdit::textChanged, this, [this](const QString&) {
        setModified(true);
        emit contentChanged();
    });
    connect(m_addBlockBtn, &QPushButton::clicked, this, &ReportEditor::onAddBlock);
}

// ===========================================================================
// 报告加载与保存
// ===========================================================================

void ReportEditor::loadReport(const Report::Ptr& report)
{
    m_loading = true;
    m_report = report;

    // 加载元信息
    m_titleEdit->setText(report->title());
    m_authorEdit->setText(report->author());
    m_dateEdit->setDate(report->experimentDate());

    // 设置状态
    const int statusIdx = m_statusCombo->findData(static_cast<int>(report->status()));
    if (statusIdx >= 0) m_statusCombo->setCurrentIndex(statusIdx);

    // 重建块编辑器
    rebuildBlocks();

    m_modified = false;
    m_loading = false;

    updateStatusBar();
    LOG_INFO(QString("报告已加载到编辑器: id=%1, title='%2'")
                 .arg(report->id()).arg(report->title()));
}

Report::Ptr ReportEditor::saveToReport()
{
    if (!m_report) {
        m_report = Report::create();
    }

    m_report->setTitle(m_titleEdit->text().trimmed());
    m_report->setAuthor(m_authorEdit->text().trimmed());
    m_report->setExperimentDate(m_dateEdit->date());
    m_report->setStatus(static_cast<ReportStatus>(
        m_statusCombo->currentData().toInt()));

    // 收集所有块的内容
    m_report->clearBlocks();
    for (BlockEditor* editor : m_blockEditors) {
        m_report->appendBlock(editor->contentBlock());
    }

    m_modified = false;
    updateStatusBar();

    return m_report;
}

// ===========================================================================
// 块操作
// ===========================================================================

BlockEditor* ReportEditor::blockEditorAt(int index) const
{
    if (index >= 0 && index < m_blockEditors.size()) {
        return m_blockEditors.at(index);
    }
    return nullptr;
}

BlockEditor* ReportEditor::insertBlock(int index, BlockType type)
{
    if (index < 0) index = 0;
    if (index > m_blockEditors.size()) index = m_blockEditors.size();

    // 创建内容块
    ContentBlock block(type);
    block.id = Report::generateBlockId();

    // 创建块编辑器
    BlockEditor* editor = BlockEditorFactory::createEditor(block, this);
    connectBlockEditor(editor);

    // 插入到布局和列表
    m_blockEditors.insert(index, editor);
    m_blocksLayout->insertWidget(index, editor);

    // 设置焦点到新块
    m_currentBlockIndex = index;
    editor->setFocusToEditor();

    updateBlockSelection();
    updateStatusBar();
    setModified(true);
    emit contentChanged();
    emit blockCountChanged(m_blockEditors.size());
    updateChartBlockReportId();

    return editor;
}

BlockEditor* ReportEditor::appendBlock(BlockType type)
{
    return insertBlock(m_blockEditors.size(), type);
}

void ReportEditor::removeBlock(int index)
{
    if (index < 0 || index >= m_blockEditors.size()) return;

    // 至少保留一个块
    if (m_blockEditors.size() <= 1) {
        // 如果只剩一个块，清空其内容而不是删除
        BlockEditor* editor = m_blockEditors.first();
        if (TextBlockEditor* textEditor = qobject_cast<TextBlockEditor*>(editor)) {
            textEditor->setPlainText("");
        }
        return;
    }

    BlockEditor* editor = m_blockEditors.takeAt(index);
    m_blocksLayout->removeWidget(editor);
    editor->deleteLater();

    // 调整当前焦点索引
    if (m_currentBlockIndex >= m_blockEditors.size()) {
        m_currentBlockIndex = m_blockEditors.size() - 1;
    }

    updateBlockSelection();
    updateStatusBar();
    setModified(true);
    emit contentChanged();
    emit blockCountChanged(m_blockEditors.size());
}

void ReportEditor::moveBlock(int from, int to)
{
    if (from < 0 || from >= m_blockEditors.size()) return;
    if (to < 0 || to >= m_blockEditors.size()) return;
    if (from == to) return;

    // 移动列表中的元素
    BlockEditor* editor = m_blockEditors.takeAt(from);
    m_blockEditors.insert(to, editor);

    // 重新排列布局（移除并重新插入）
    m_blocksLayout->removeWidget(editor);
    m_blocksLayout->insertWidget(to, editor);

    m_currentBlockIndex = to;
    updateBlockSelection();
    setModified(true);
    emit contentChanged();
}

void ReportEditor::convertBlock(int index, BlockType newType)
{
    if (index < 0 || index >= m_blockEditors.size()) return;

    BlockEditor* oldEditor = m_blockEditors.at(index);

    // 保存旧块的 ID 和文本内容（用于转换时保留）
    const QString blockId = oldEditor->blockId();
    const QString plainText = oldEditor->plainText();

    // 创建新块
    ContentBlock newBlock(newType);
    newBlock.id = blockId;
    if (!plainText.isEmpty()) {
        newBlock.data["text"] = plainText;
    }

    BlockEditor* newEditor = BlockEditorFactory::createEditor(newBlock, this);
    connectBlockEditor(newEditor);

    // 替换
    m_blockEditors.replace(index, newEditor);
    m_blocksLayout->removeWidget(oldEditor);
    m_blocksLayout->insertWidget(index, newEditor);
    oldEditor->deleteLater();

    m_currentBlockIndex = index;
    newEditor->setFocusToEditor();
    updateBlockSelection();
    setModified(true);
    emit contentChanged();
}

void ReportEditor::duplicateBlock(int index)
{
    if (index < 0 || index >= m_blockEditors.size()) return;

    BlockEditor* source = m_blockEditors.at(index);
    ContentBlock block = source->contentBlock();
    block.id = Report::generateBlockId();  // 新 ID

    BlockEditor* newEditor = BlockEditorFactory::createEditor(block, this);
    connectBlockEditor(newEditor);

    const int insertIdx = index + 1;
    m_blockEditors.insert(insertIdx, newEditor);
    m_blocksLayout->insertWidget(insertIdx, newEditor);

    m_currentBlockIndex = insertIdx;
    newEditor->setFocusToEditor();
    updateBlockSelection();
    updateStatusBar();
    setModified(true);
    emit contentChanged();
}

// ===========================================================================
// 编辑操作
// ===========================================================================

void ReportEditor::focusBlock(int index)
{
    if (index >= 0 && index < m_blockEditors.size()) {
        m_currentBlockIndex = index;
        m_blockEditors.at(index)->setFocusToEditor();
        updateBlockSelection();
    }
}

void ReportEditor::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
    m_titleEdit->setReadOnly(readOnly);
    m_authorEdit->setReadOnly(readOnly);
    m_dateEdit->setReadOnly(readOnly);
    m_statusCombo->setEnabled(!readOnly);
    m_addBlockBtn->setEnabled(!readOnly);

    for (BlockEditor* editor : m_blockEditors) {
        editor->setReadOnly(readOnly);
    }
}

void ReportEditor::setModified(bool modified)
{
    if (m_modified == modified) return;
    m_modified = modified;

    if (modified) {
        m_saveStatusLabel->setText(tr("未保存"));
        m_saveStatusLabel->setStyleSheet("color: #E6A23C; font-size: 12px;");
    } else {
        m_saveStatusLabel->setText(tr("已保存"));
        m_saveStatusLabel->setStyleSheet("color: #67C23A; font-size: 12px;");
    }

    emit saveStateChanged(!modified);
}

int ReportEditor::wordCount() const
{
    int count = 0;
    for (BlockEditor* editor : m_blockEditors) {
        count += editor->plainText().length();
    }
    // 标题也算入
    count += m_titleEdit->text().length();
    return count;
}

// ===========================================================================
// 块编辑器信号处理
// ===========================================================================

void ReportEditor::onBlockContentChanged()
{
    if (m_loading) return;
    setModified(true);
    emit contentChanged();
    updateStatusBar();
}

void ReportEditor::onBlockFocused(BlockEditor* editor)
{
    const int idx = indexOfBlockEditor(editor);
    if (idx >= 0) {
        m_currentBlockIndex = idx;
        updateBlockSelection();
    }
}

void ReportEditor::onRequestInsertAfter(BlockEditor* editor, BlockType type)
{
    const int idx = indexOfBlockEditor(editor);
    if (idx >= 0) {
        insertBlock(idx + 1, type);
    }
}

void ReportEditor::onRequestInsertBefore(BlockEditor* editor, BlockType type)
{
    const int idx = indexOfBlockEditor(editor);
    if (idx >= 0) {
        insertBlock(idx, type);
    }
}

void ReportEditor::onRequestDelete(BlockEditor* editor)
{
    const int idx = indexOfBlockEditor(editor);
    if (idx >= 0) {
        removeBlock(idx);
    }
}

void ReportEditor::onRequestConvert(BlockEditor* editor, BlockType newType)
{
    const int idx = indexOfBlockEditor(editor);
    if (idx >= 0) {
        convertBlock(idx, newType);
    }
}

void ReportEditor::onRequestFocusPrevious(BlockEditor* editor)
{
    const int idx = indexOfBlockEditor(editor);
    if (idx > 0) {
        focusBlock(idx - 1);
    }
}

void ReportEditor::onRequestFocusNext(BlockEditor* editor)
{
    const int idx = indexOfBlockEditor(editor);
    if (idx >= 0 && idx < m_blockEditors.size() - 1) {
        focusBlock(idx + 1);
    }
}

void ReportEditor::onRequestMoveUp(BlockEditor* editor)
{
    const int idx = indexOfBlockEditor(editor);
    if (idx > 0) {
        moveBlock(idx, idx - 1);
    }
}

void ReportEditor::onRequestMoveDown(BlockEditor* editor)
{
    const int idx = indexOfBlockEditor(editor);
    if (idx >= 0 && idx < m_blockEditors.size() - 1) {
        moveBlock(idx, idx + 1);
    }
}

// ===========================================================================
// 标题栏信号
// ===========================================================================

void ReportEditor::onTitleChanged(const QString& title)
{
    if (m_loading) return;
    setModified(true);
    emit titleChanged(title);
    emit contentChanged();
}

void ReportEditor::onStatusChanged(int index)
{
    Q_UNUSED(index);
    if (m_loading) return;
    setModified(true);
    emit contentChanged();
}

void ReportEditor::onDateChanged(const QDate& date)
{
    Q_UNUSED(date);
    if (m_loading) return;
    setModified(true);
    emit contentChanged();
}

// ===========================================================================
// 工具栏
// ===========================================================================

void ReportEditor::onAddBlock()
{
    // 显示块类型选择菜单
    QMenu menu(this);
    menu.setTitle(tr("添加块"));

    const QList<BlockType> types = BlockEditorFactory::supportedTypes();
    for (BlockType type : types) {
        QAction* action = menu.addAction(BlockEditorFactory::typeDisplayName(type));
        connect(action, &QAction::triggered, this, [this, type]() {
            appendBlock(type);
        });
    }

    menu.exec(m_addBlockBtn->mapToGlobal(QPoint(0, m_addBlockBtn->height())));
}

void ReportEditor::onUndo()
{
    // 撤销功能（简化版，后续实现完整的撤销/重做栈）
    LOG_INFO("撤销功能待实现");
}

void ReportEditor::onRedo()
{
    LOG_INFO("重做功能待实现");
}

// ===========================================================================
// 内部方法
// ===========================================================================

void ReportEditor::rebuildBlocks()
{
    clearBlocks();

    if (!m_report) return;

    const QList<ContentBlock>& blocks = m_report->blocks();
    for (const ContentBlock& block : blocks) {
        BlockEditor* editor = BlockEditorFactory::createEditor(block, this);
        connectBlockEditor(editor);
        m_blockEditors.append(editor);
        m_blocksLayout->insertWidget(m_blockEditors.size() - 1, editor);
    }

    // 如果报告没有块，添加一个空段落
    if (m_blockEditors.isEmpty()) {
        appendBlock(BlockType::Paragraph);
    }

    m_currentBlockIndex = 0;
    updateBlockSelection();
    updateChartBlockReportId();
}

void ReportEditor::clearBlocks()
{
    for (BlockEditor* editor : m_blockEditors) {
        m_blocksLayout->removeWidget(editor);
        editor->deleteLater();
    }
    m_blockEditors.clear();
    m_currentBlockIndex = -1;
}

int ReportEditor::indexOfBlockEditor(BlockEditor* editor) const
{
    return m_blockEditors.indexOf(editor);
}

void ReportEditor::connectBlockEditor(BlockEditor* editor)
{
    connect(editor, &BlockEditor::contentChanged,
            this, &ReportEditor::onBlockContentChanged);
    connect(editor, &BlockEditor::blockFocused,
            this, &ReportEditor::onBlockFocused);
    connect(editor, &BlockEditor::requestInsertBlockAfter,
            this, &ReportEditor::onRequestInsertAfter);
    connect(editor, &BlockEditor::requestInsertBlockBefore,
            this, &ReportEditor::onRequestInsertBefore);
    connect(editor, &BlockEditor::requestDeleteBlock,
            this, &ReportEditor::onRequestDelete);
    connect(editor, &BlockEditor::requestConvertBlock,
            this, &ReportEditor::onRequestConvert);
    connect(editor, &BlockEditor::requestFocusPrevious,
            this, &ReportEditor::onRequestFocusPrevious);
    connect(editor, &BlockEditor::requestFocusNext,
            this, &ReportEditor::onRequestFocusNext);
    connect(editor, &BlockEditor::requestMoveUp,
            this, &ReportEditor::onRequestMoveUp);
    connect(editor, &BlockEditor::requestMoveDown,
            this, &ReportEditor::onRequestMoveDown);
}

void ReportEditor::updateStatusBar()
{
    m_wordCountLabel->setText(tr("字数: %1").arg(wordCount()));
    m_blockCountLabel->setText(tr("块: %1").arg(m_blockEditors.size()));
}

void ReportEditor::updateBlockSelection()
{
    for (int i = 0; i < m_blockEditors.size(); ++i) {
        m_blockEditors.at(i)->setBlockSelected(i == m_currentBlockIndex);
    }
}

void ReportEditor::updateChartBlockReportId()
{
    if (!m_report) return;
    const qint64 reportId = m_report->id();

    // 前向声明 ChartBlockEditor，避免循环 include
    // 实际类型在 OtherBlockEditors.h 中定义
    // 我们通过 blockType() 判断，然后使用 QMetaObject 调用 setReportId
    for (BlockEditor* editor : m_blockEditors) {
        if (editor->blockType() == BlockType::Chart) {
            // 使用 Qt 元对象系统调用 setReportId
            // ChartBlockEditor 声明了 Q_INVOKABLE void setReportId(qint64)
            QMetaObject::invokeMethod(editor, "setReportId",
                                       Qt::DirectConnection,
                                       Q_ARG(qint64, reportId));
        }
    }
}
