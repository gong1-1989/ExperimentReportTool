/**
 * @file ReportEditor.cpp
 * @brief 报告编辑器主组件实现文件
 */

#include "ReportEditor.h"
#include "ui_ReportEditor.h"  // 由 uic 工具从 .ui 文件自动生成
#include "editor/TextBlockEditor.h"
#include "editor/OtherBlockEditors.h"
#include "editor/AutoSaveManager.h"
#include "editor/DataTableEditorDialog.h"
#include "data/repositories/DataTableRepository.h"
#include "core/models/DataTable.h"
#include "core/utils/Logger.h"

#include <QScrollBar>
#include <QMenu>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QInputDialog>

// ===========================================================================
// 构造与析构
// ===========================================================================

ReportEditor::ReportEditor(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ReportEditor)  // 创建 UI 界面对象
    , m_currentBlockIndex(-1)
    , m_modified(false)
    , m_readOnly(false)
    , m_loading(false)
    , m_autoSave(nullptr)
{
    ui->setupUi(this);  // 从 .ui 文件加载界面

    // ========================================================================
    // 关键修复：彻底简化布局结构
    // ========================================================================
    // 原布局：QScrollArea > scrollAreaWidgetContents > scrollLayout > m_blocksContainer > m_blocksLayout > 块编辑器
    // 新布局：QScrollArea > scrollAreaWidgetContents > m_blocksLayout > 块编辑器
    // 移除 m_blocksContainer 中间层，减少布局嵌套，解决高度计算问题

    // 1. 从 scrollLayout 中移除 m_blocksContainer
    ui->scrollLayout->removeWidget(ui->m_blocksContainer);
    ui->m_blocksContainer->hide();

    // 2. 将 m_emptyLabel 从 m_blocksLayout 中移除（暂时）
    ui->m_blocksLayout->removeWidget(ui->m_emptyLabel);

    // 3. 将 m_blocksLayout 重新设置为 scrollAreaWidgetContents 的布局
    //    先移除 scrollAreaWidgetContents 的旧布局（scrollLayout）
    delete ui->scrollAreaWidgetContents->layout();
    //    将 m_blocksLayout 的父对象设置为 scrollAreaWidgetContents
    ui->m_blocksLayout->setParent(ui->scrollAreaWidgetContents);
    ui->scrollAreaWidgetContents->setLayout(ui->m_blocksLayout);

    // 4. 将 m_emptyLabel 添加回 m_blocksLayout（在最前面）
    ui->m_blocksLayout->insertWidget(0, ui->m_emptyLabel);

    // 5. 布局设置
    ui->scrollArea->setWidgetResizable(true);
    // scrollAreaWidgetContents：高度由内容决定（Minimum）
    ui->scrollAreaWidgetContents->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    ui->m_blocksLayout->setAlignment(Qt::AlignTop);
    ui->m_blocksLayout->addStretch(1);

    // 初始更新空提示标签显示状态
    updateEmptyLabelVisibility();

    // 设置状态下拉框的 itemData
    ui->m_statusCombo->setItemData(0, static_cast<int>(ReportStatus::Draft));
    ui->m_statusCombo->setItemData(1, static_cast<int>(ReportStatus::Submitted));
    ui->m_statusCombo->setItemData(2, static_cast<int>(ReportStatus::Reviewed));

    // 连接信号
    connect(ui->m_titleEdit, &QLineEdit::textChanged, this, &ReportEditor::onTitleChanged);
    connect(ui->m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReportEditor::onStatusChanged);
    connect(ui->m_dateEdit, &QDateEdit::dateChanged, this, &ReportEditor::onDateChanged);
    connect(ui->m_addBlockBtn, &QPushButton::clicked, this, &ReportEditor::onAddBlock);

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
    delete ui;
}

// ===========================================================================
// 报告加载与保存
// ===========================================================================

void ReportEditor::loadReport(const Report::Ptr& report)
{
    m_loading = true;
    m_report = report;

    // 加载元信息
    ui->m_titleEdit->setText(report->title());
    ui->m_authorEdit->setText(report->author());
    ui->m_dateEdit->setDate(report->experimentDate());

    // 设置状态
    const int statusIdx = ui->m_statusCombo->findData(static_cast<int>(report->status()));
    if (statusIdx >= 0) ui->m_statusCombo->setCurrentIndex(statusIdx);

    // 重建块编辑器
    rebuildBlocks();

    m_modified = false;
    m_loading = false;

    updateStatusBar();
    LOG_INFO(QString("报告已加载到编辑器: id=%1, title='%2'")
                 .arg(report->id()).arg(report->title()));
}

QString ReportEditor::reportTitle() const
{
    // 通过 ui 指针访问 .ui 文件中定义的标题编辑框控件
    // 此函数不能在头文件中内联实现，因为 Ui::ReportEditor 在头文件中只有前向声明
    return ui->m_titleEdit->text();
}

Report::Ptr ReportEditor::saveToReport()
{
    if (!m_report) {
        m_report = Report::create();
    }

    m_report->setTitle(ui->m_titleEdit->text().trimmed());
    m_report->setAuthor(ui->m_authorEdit->text().trimmed());
    m_report->setExperimentDate(ui->m_dateEdit->date());
    m_report->setStatus(static_cast<ReportStatus>(
        ui->m_statusCombo->currentData().toInt()));

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
    // 空指针检查：确保块编辑器创建成功
    if (!editor) {
        LOG_ERROR(QString("创建块编辑器失败，类型: %1").arg(static_cast<int>(type)));
        return nullptr;
    }
    connectBlockEditor(editor);

    // 插入到布局和列表
    m_blockEditors.insert(index, editor);
    // 插入到 m_emptyLabel 之后（m_emptyLabel 在 index 0）
    ui->m_blocksLayout->insertWidget(index + 1, editor);

    // 隐藏空提示标签
    updateEmptyLabelVisibility();

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
    ui->m_blocksLayout->removeWidget(editor);
    editor->deleteLater();

    // 调整当前焦点索引
    if (m_currentBlockIndex >= m_blockEditors.size()) {
        m_currentBlockIndex = m_blockEditors.size() - 1;
    }

    updateEmptyLabelVisibility();  // 更新空提示标签显示状态
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
    ui->m_blocksLayout->removeWidget(editor);
    ui->m_blocksLayout->insertWidget(to + 1, editor);  // +1 因为 m_emptyLabel 在 index 0

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
    ui->m_blocksLayout->removeWidget(oldEditor);
    ui->m_blocksLayout->insertWidget(index + 1, newEditor);  // +1 因为 m_emptyLabel 在 index 0
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
    ui->m_blocksLayout->insertWidget(insertIdx + 1, newEditor);  // +1 因为 m_emptyLabel 在 index 0

    m_currentBlockIndex = insertIdx;
    newEditor->setFocusToEditor();
    updateEmptyLabelVisibility();  // 更新空提示标签显示状态
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
    ui->m_titleEdit->setReadOnly(readOnly);
    ui->m_authorEdit->setReadOnly(readOnly);
    ui->m_dateEdit->setReadOnly(readOnly);
    ui->m_statusCombo->setEnabled(!readOnly);
    ui->m_addBlockBtn->setEnabled(!readOnly);

    for (BlockEditor* editor : m_blockEditors) {
        editor->setReadOnly(readOnly);
    }
}

void ReportEditor::setModified(bool modified)
{
    if (m_modified == modified) return;
    m_modified = modified;

    if (modified) {
        ui->m_saveStatusLabel->setText(tr("未保存"));
        ui->m_saveStatusLabel->setStyleSheet("color: #E6A23C; font-size: 12px;");
    } else {
        ui->m_saveStatusLabel->setText(tr("已保存"));
        ui->m_saveStatusLabel->setStyleSheet("color: #67C23A; font-size: 12px;");
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
    count += ui->m_titleEdit->text().length();
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

    // 添加分隔线
    menu.addSeparator();

    // 添加"新建数据表"选项（不是块类型，而是创建数据表供图表引用）
    QAction* newDataTableAction = menu.addAction(tr("📊 新建数据表..."));
    connect(newDataTableAction, &QAction::triggered, this, [this]() {
        createNewDataTable();
    });

    menu.exec(ui->m_addBlockBtn->mapToGlobal(QPoint(0, ui->m_addBlockBtn->height())));
}

void ReportEditor::createNewDataTable()
{
    if (!m_report) return;

    // 弹出对话框让用户输入数据表名称
    bool ok;
    const QString tableName = QInputDialog::getText(
        this, tr("新建数据表"),
        tr("请输入数据表名称:"), QLineEdit::Normal,
        tr("数据表 %1").arg(QDateTime::currentDateTime().toString("MMddHHmm")),
        &ok);

    if (!ok || tableName.trimmed().isEmpty()) return;

    // 创建新的数据表
    DataTable::Ptr table = DataTable::create();
    table->setName(tableName.trimmed());
    table->setReportId(m_report->id());
    table->setDescription(tr("由报告编辑器创建的数据表"));

    // 默认创建 3 列 3 行
    QList<ColumnDefinition> columns;
    for (int i = 0; i < 3; ++i) {
        ColumnDefinition col;
        col.name = QString("列%1").arg(i + 1);
        col.type = ColumnType::Text;
        columns.append(col);
    }
    table->setColumns(columns);

    // 添加 3 行空数据
    for (int row = 0; row < 3; ++row) {
        QVariantList rowData;
        for (int col = 0; col < 3; ++col) {
            rowData.append(QString(""));
        }
        table->appendRow(rowData);
    }

    // 保存到数据库
    if (DataTableRepository::insert(table)) {
        LOG_INFO(QString("数据表创建成功: %1 (ID=%2)").arg(tableName).arg(table->id()));

        // 打开数据表编辑器
        DataTableEditorDialog dialog(table, this);
        if (dialog.exec() == QDialog::Accepted) {
            // 更新数据表
            DataTableRepository::update(dialog.tableData());
            QMessageBox::information(this, tr("提示"),
                tr("数据表已创建！\n现在可以添加图表块并引用此数据表。"));
        }
    } else {
        QMessageBox::warning(this, tr("错误"), tr("数据表创建失败！"));
    }
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
        // 插入到 m_emptyLabel 之后（m_emptyLabel 在 index 0）
        ui->m_blocksLayout->insertWidget(m_blockEditors.size(), editor);
    }

    // 如果报告没有块，添加一个空段落
    if (m_blockEditors.isEmpty()) {
        appendBlock(BlockType::Paragraph);
    }

    m_currentBlockIndex = 0;
    updateEmptyLabelVisibility();  // 更新空提示标签显示状态
    updateBlockSelection();
    updateChartBlockReportId();
}

void ReportEditor::clearBlocks()
{
    for (BlockEditor* editor : m_blockEditors) {
        ui->m_blocksLayout->removeWidget(editor);
        editor->deleteLater();
    }
    m_blockEditors.clear();
    m_currentBlockIndex = -1;
    updateEmptyLabelVisibility();  // 显示空提示标签
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
    ui->m_wordCountLabel->setText(tr("字数: %1").arg(wordCount()));
    ui->m_blockCountLabel->setText(tr("块: %1").arg(m_blockEditors.size()));
}

void ReportEditor::updateEmptyLabelVisibility()
{
    // 有块编辑器时隐藏空提示标签，无块时显示
    if (ui->m_emptyLabel) {
        ui->m_emptyLabel->setVisible(m_blockEditors.isEmpty());
    }
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
