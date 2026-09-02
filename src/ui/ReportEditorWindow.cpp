/**
 * @file ReportEditorWindow.cpp
 * @brief 报告编辑窗口实现文件
 */

#include "ReportEditorWindow.h"
#include "editor/ReportEditor.h"
#include "editor/TextBlockEditor.h"
#include "export/ExportManager.h"
#include "print/PrintManager.h"
#include "version/VersionHistoryDialog.h"
#include "ui/dialogs/TagManagerDialog.h"
#include "ui/dialogs/ReportTagDialog.h"
#include "ui/dialogs/AttachmentManagerDialog.h"
#include "data/repositories/TagRepository.h"
#include "data/repositories/ReportRepository.h"
#include "core/utils/Logger.h"
#include "core/utils/AppConstants.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QApplication>
#include <QClipboard>

// ===========================================================================
// 构造与析构
// ===========================================================================

ReportEditorWindow::ReportEditorWindow(const Report::Ptr& report, QWidget* parent)
    : QMainWindow(parent)
    , m_editor(nullptr)
    , m_report(report)
    , m_printManager(nullptr)
    , m_statusSaveLabel(nullptr)
    , m_statusWordLabel(nullptr)
    , m_statusPositionLabel(nullptr)
    , m_actionSave(nullptr)
    , m_actionUndo(nullptr)
    , m_actionRedo(nullptr)
    , m_actionBold(nullptr)
    , m_actionItalic(nullptr)
    , m_actionUnderline(nullptr)
    , m_isNewReport(report.isNull() || !report->isPersisted())
    , m_zoomFactor(1.0)
{
    setupUi();
    m_printManager = new PrintManager(this);
    createActions();
    createMenus();
    createToolBar();
    createStatusBar();
    connectSignals();

    // 加载报告
    if (m_report) {
        m_editor->loadReport(m_report);
    } else {
        // 新建报告
        m_report = Report::create();
        m_report->setTitle(tr("未命名实验报告"));
        m_editor->loadReport(m_report);
    }

    updateWindowTitle();
    resize(1200, 800);

    LOG_INFO(QString("报告编辑窗口已打开: %1")
                 .arg(m_isNewReport ? "新建报告" : m_report->title()));
}

ReportEditorWindow::~ReportEditorWindow()
{
}

// ===========================================================================
// UI 初始化
// ===========================================================================

void ReportEditorWindow::setupUi()
{
    m_editor = new ReportEditor(this);
    setCentralWidget(m_editor);
}

void ReportEditorWindow::createActions()
{
    // 文件
    m_actionSave = new QAction(tr("保存(&S)"), this);
    m_actionSave->setShortcut(QKeySequence::Save);
    connect(m_actionSave, &QAction::triggered, this, &ReportEditorWindow::onSave);

    // 编辑
    m_actionUndo = new QAction(tr("撤销(&U)"), this);
    m_actionUndo->setShortcut(QKeySequence::Undo);
    m_actionUndo->setEnabled(false);

    m_actionRedo = new QAction(tr("重做(&R)"), this);
    m_actionRedo->setShortcut(QKeySequence::Redo);
    m_actionRedo->setEnabled(false);

    // 格式
    m_actionBold = new QAction(tr("加粗"), this);
    m_actionBold->setShortcut(QKeySequence::Bold);
    connect(m_actionBold, &QAction::triggered, this, &ReportEditorWindow::onBold);

    m_actionItalic = new QAction(tr("斜体"), this);
    m_actionItalic->setShortcut(QKeySequence::Italic);
    connect(m_actionItalic, &QAction::triggered, this, &ReportEditorWindow::onItalic);

    m_actionUnderline = new QAction(tr("下划线"), this);
    m_actionUnderline->setShortcut(QKeySequence::Underline);
    connect(m_actionUnderline, &QAction::triggered, this, &ReportEditorWindow::onUnderline);
}

void ReportEditorWindow::createMenus()
{
    QMenuBar* bar = menuBar();

    // 文件菜单
    QMenu* fileMenu = bar->addMenu(tr("文件(&F)"));
    fileMenu->addAction(tr("新建(&N)"), this, &ReportEditorWindow::onNew, QKeySequence::New);
    fileMenu->addAction(m_actionSave);
    fileMenu->addAction(tr("另存为(&A)..."), this, &ReportEditorWindow::onSaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("导出(&E)..."), this, &ReportEditorWindow::onExport);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("打印预览(&V)..."), this, &ReportEditorWindow::onPrintPreview);
    fileMenu->addAction(tr("打印(&P)..."), this, &ReportEditorWindow::onPrint, QKeySequence::Print);
    fileMenu->addAction(tr("页面设置(&G)..."), this, &ReportEditorWindow::onPageSetup);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("版本历史(&H)..."), this, &ReportEditorWindow::onVersionHistory);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("关闭(&C)"), this, &QWidget::close, QKeySequence::Close);

    // 编辑菜单
    QMenu* editMenu = bar->addMenu(tr("编辑(&E)"));
    editMenu->addAction(m_actionUndo);
    editMenu->addAction(m_actionRedo);
    editMenu->addSeparator();
    editMenu->addAction(tr("查找(&F)..."), this, &ReportEditorWindow::onFind, QKeySequence::Find);
    editMenu->addSeparator();
    editMenu->addAction(tr("编辑标签(&T)..."), this, &ReportEditorWindow::onEditTags);
    editMenu->addAction(tr("管理标签(&M)..."), this, &ReportEditorWindow::onManageTags);
    editMenu->addSeparator();
    editMenu->addAction(tr("附件管理(&A)..."), this, &ReportEditorWindow::onManageAttachments);

    // 插入菜单
    QMenu* insertMenu = bar->addMenu(tr("插入(&I)"));
    insertMenu->addAction(tr("表格"), this, &ReportEditorWindow::onInsertTable);
    insertMenu->addAction(tr("图片"), this, &ReportEditorWindow::onInsertImage);
    insertMenu->addSeparator();
    insertMenu->addAction(tr("分割线"), this, &ReportEditorWindow::onInsertDivider);
    insertMenu->addAction(tr("代码块"), this, &ReportEditorWindow::onCodeBlock);

    // 格式菜单
    QMenu* formatMenu = bar->addMenu(tr("格式(&O)"));
    formatMenu->addAction(m_actionBold);
    formatMenu->addAction(m_actionItalic);
    formatMenu->addAction(m_actionUnderline);
    formatMenu->addSeparator();
    QMenu* headingMenu = formatMenu->addMenu(tr("标题"));
    headingMenu->addAction(tr("一级标题"), this, [this]() { onHeading(1); }, QKeySequence("Ctrl+1"));
    headingMenu->addAction(tr("二级标题"), this, [this]() { onHeading(2); }, QKeySequence("Ctrl+2"));
    headingMenu->addAction(tr("三级标题"), this, [this]() { onHeading(3); }, QKeySequence("Ctrl+3"));
    formatMenu->addSeparator();
    formatMenu->addAction(tr("无序列表"), this, [this]() { onList(false); });
    formatMenu->addAction(tr("有序列表"), this, [this]() { onList(true); });
    formatMenu->addAction(tr("引用"), this, &ReportEditorWindow::onQuote);

    // 视图菜单
    QMenu* viewMenu = bar->addMenu(tr("视图(&V)"));
    viewMenu->addAction(tr("全屏"), this, &ReportEditorWindow::onToggleFullscreen, QKeySequence("F11"));
    viewMenu->addSeparator();
    viewMenu->addAction(tr("放大"), this, &ReportEditorWindow::onZoomIn, QKeySequence("Ctrl++"));
    viewMenu->addAction(tr("缩小"), this, &ReportEditorWindow::onZoomOut, QKeySequence("Ctrl+-"));
    viewMenu->addAction(tr("重置缩放"), this, &ReportEditorWindow::onResetZoom, QKeySequence("Ctrl+0"));
}

void ReportEditorWindow::createToolBar()
{
    QToolBar* toolBar = addToolBar(tr("编辑工具栏"));
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(18, 18));

    toolBar->addAction(m_actionSave);
    toolBar->addSeparator();
    toolBar->addAction(m_actionUndo);
    toolBar->addAction(m_actionRedo);
    toolBar->addSeparator();
    toolBar->addAction(m_actionBold);
    toolBar->addAction(m_actionItalic);
    toolBar->addAction(m_actionUnderline);
    toolBar->addSeparator();

    // 标题下拉
    QComboBox* headingCombo = new QComboBox(toolBar);
    headingCombo->addItems({tr("正文"), tr("H1"), tr("H2"), tr("H3")});
    headingCombo->setToolTip(tr("段落样式"));
    connect(headingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) { onHeading(idx); });
    toolBar->addWidget(headingCombo);

    toolBar->addSeparator();
    toolBar->addAction(tr("表格"), this, &ReportEditorWindow::onInsertTable);
    toolBar->addAction(tr("图片"), this, &ReportEditorWindow::onInsertImage);
    toolBar->addAction(tr("代码"), this, &ReportEditorWindow::onCodeBlock);
    toolBar->addAction(tr("分割线"), this, &ReportEditorWindow::onInsertDivider);
}

void ReportEditorWindow::createStatusBar()
{
    QStatusBar* bar = statusBar();

    m_statusSaveLabel = new QLabel(tr("已保存"), this);
    m_statusSaveLabel->setStyleSheet("color: #67C23A; padding: 0 8px;");
    bar->addWidget(m_statusSaveLabel);

    QFrame* vLine = new QFrame();
    vLine->setFrameShape(QFrame::VLine);         // 竖线
    vLine->setFrameShadow(QFrame::Sunken);
    vLine->setFixedWidth(2);
    bar->addWidget(vLine);

    m_statusWordLabel = new QLabel(tr("字数: 0"), this);
    m_statusWordLabel->setStyleSheet("color: #666; padding: 0 8px;");
    bar->addPermanentWidget(m_statusWordLabel);

    m_statusPositionLabel = new QLabel(this);
    m_statusPositionLabel->setStyleSheet("color: #666; padding: 0 8px;");
    bar->addPermanentWidget(m_statusPositionLabel);
}

void ReportEditorWindow::connectSignals()
{
    connect(m_editor, &ReportEditor::contentChanged,
            this, &ReportEditorWindow::onContentChanged);
    connect(m_editor, &ReportEditor::titleChanged,
            this, &ReportEditorWindow::onTitleChanged);
    connect(m_editor, &ReportEditor::saveRequested,
            this, &ReportEditorWindow::onSaveTriggered);
    connect(m_editor, &ReportEditor::saveStateChanged,
            this, &ReportEditorWindow::onSaveStateChanged);
}

// ===========================================================================
// 文件操作
// ===========================================================================

void ReportEditorWindow::onNew()
{
    // 新建报告（在新窗口中打开）
    ReportEditorWindow* newWindow = new ReportEditorWindow(Report::Ptr(), nullptr);
    newWindow->show();
}

void ReportEditorWindow::onSave()
{
    if (saveReport()) {
        m_statusSaveLabel->setText(tr("已保存"));
        m_statusSaveLabel->setStyleSheet("color: #67C23A; padding: 0 8px;");
    }
}

void ReportEditorWindow::onSaveAs()
{
    // 另存为（创建副本）
    if (saveReport()) {
        // 创建副本
        Report::Ptr copy = Report::create();
        copy->setTitle(m_report->title() + tr(" (副本)"));
        copy->setProjectId(m_report->projectId());
        copy->setTemplateId(m_report->templateId());
        copy->setAuthor(m_report->author());
        copy->setExperimentDate(m_report->experimentDate());

        // 复制内容块
        for (int i = 0; i < m_report->blockCount(); ++i) {
            ContentBlock block = m_report->blockAt(i);
            block.id = Report::generateBlockId();
            copy->appendBlock(block);
        }

        if (ReportRepository::insert(copy)) {
            QMessageBox::information(this, tr("另存为"),
                tr("报告已另存为「%1」").arg(copy->title()));
            emit reportSaved(copy->id());
        }
    }
}

void ReportEditorWindow::onExport()
{
    // 先保存当前报告
    if (!saveReport()) {
        QMessageBox::warning(this, tr("导出失败"), tr("保存报告失败，无法导出"));
        return;
    }

    // 显示导出文件对话框
    const QString defaultName = m_report->title().isEmpty()
        ? tr("未命名报告") : m_report->title();
    const auto result = ExportManager::getSaveFilePath(this, defaultName);

    if (result.first.isEmpty()) {
        return;  // 用户取消
    }

    // 执行导出
    ExportManager exporter;
    ExportConfig config;
    config.format = result.second;
    config.filePath = result.first;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool success = exporter.exportReport(m_report, config, this);
    QApplication::restoreOverrideCursor();

    if (success) {
        QMessageBox::information(this, tr("导出成功"),
            tr("报告已导出到:\n%1").arg(result.first));
        showStatusMessage(tr("导出成功: %1").arg(result.first));
    } else {
        QMessageBox::critical(this, tr("导出失败"),
            tr("导出报告时发生错误，请查看日志"));
    }
}

void ReportEditorWindow::onPrint()
{
    if (!m_report) return;

    // 先保存
    if (!saveReport()) {
        QMessageBox::warning(this, tr("打印失败"), tr("保存报告失败，无法打印"));
        return;
    }

    m_printManager->print(m_report, this);
}

void ReportEditorWindow::onPrintPreview()
{
    if (!m_report) return;

    if (!saveReport()) {
        QMessageBox::warning(this, tr("打印预览失败"), tr("保存报告失败，无法预览"));
        return;
    }

    m_printManager->printPreview(m_report, this);
}

void ReportEditorWindow::onPageSetup()
{
    PrintConfig config = m_printManager->currentConfig();
    if (m_printManager->pageSetup(config, this)) {
        showStatusMessage(tr("页面设置已更新"));
    }
}

void ReportEditorWindow::onVersionHistory()
{
    if (!m_report || m_report->id() <= 0) {
        QMessageBox::information(this, tr("版本历史"), tr("请先保存报告后再查看版本历史"));
        return;
    }

    VersionHistoryDialog dialog(m_report->id(), this);
    connect(&dialog, &VersionHistoryDialog::versionRestored,
            this, [this](qint64 reportId, qint64 versionId) {
                Q_UNUSED(versionId);
                // 重新加载报告
                Report::Ptr restored = ReportRepository::findById(reportId);
                if (restored) {
                    m_report = restored;
                    m_editor->loadReport(m_report);
                    updateWindowTitle();
                    showStatusMessage(tr("已恢复到历史版本"));
                }
            });

    dialog.exec();
}

void ReportEditorWindow::onEditTags()
{
    if (!m_report || m_report->id() <= 0) {
        // 先保存报告
        if (!saveReport()) {
            QMessageBox::warning(this, tr("提示"), tr("请先保存报告"));
            return;
        }
    }

    ReportTagDialog dialog(m_report->id(), this);
    if (dialog.exec() == QDialog::Accepted) {
        const QList<qint64> tagIds = dialog.selectedTagIds();
        TagRepository::setReportTags(m_report->id(), tagIds);
        showStatusMessage(tr("标签已更新: %1 个").arg(tagIds.size()));
    }
}

void ReportEditorWindow::onManageTags()
{
    TagManagerDialog dialog(this);
    dialog.exec();
}

void ReportEditorWindow::onManageAttachments()
{
    if (!m_report || m_report->id() <= 0) {
        if (!saveReport()) {
            QMessageBox::warning(this, tr("提示"), tr("请先保存报告"));
            return;
        }
    }

    AttachmentManagerDialog dialog(m_report->id(), this);
    dialog.exec();
}

// ===========================================================================
// 编辑操作
// ===========================================================================

void ReportEditorWindow::onUndo()
{
    // 撤销（待实现完整的撤销/重做栈）
}

void ReportEditorWindow::onRedo()
{
}

void ReportEditorWindow::onFind()
{
    QMessageBox::information(this, tr("查找"),
        tr("查找功能将在后续版本中实现。"));
}

// ===========================================================================
// 格式操作
// ===========================================================================

void ReportEditorWindow::onBold()
{
    applyFormatToCurrentBlock("bold");
}

void ReportEditorWindow::onItalic()
{
    applyFormatToCurrentBlock("italic");
}

void ReportEditorWindow::onUnderline()
{
    applyFormatToCurrentBlock("underline");
}

void ReportEditorWindow::onHeading(int level)
{
    const int idx = m_editor->currentBlockIndex();
    if (idx < 0) return;

    BlockType type = BlockType::Paragraph;
    switch (level) {
    case 1: type = BlockType::Heading1; break;
    case 2: type = BlockType::Heading2; break;
    case 3: type = BlockType::Heading3; break;
    default: type = BlockType::Paragraph; break;
    }

    m_editor->convertBlock(idx, type);
}

void ReportEditorWindow::onList(bool numbered)
{
    const int idx = m_editor->currentBlockIndex();
    if (idx < 0) return;
    m_editor->convertBlock(idx, numbered ? BlockType::NumberedList : BlockType::BulletList);
}

void ReportEditorWindow::onQuote()
{
    const int idx = m_editor->currentBlockIndex();
    if (idx < 0) return;
    m_editor->convertBlock(idx, BlockType::Quote);
}

void ReportEditorWindow::onCodeBlock()
{
    const int idx = m_editor->currentBlockIndex();
    if (idx < 0) {
        m_editor->appendBlock(BlockType::CodeBlock);
    } else {
        m_editor->convertBlock(idx, BlockType::CodeBlock);
    }
}

void ReportEditorWindow::onInsertTable()
{
    const int idx = m_editor->currentBlockIndex();
    m_editor->insertBlock(idx + 1, BlockType::Table);
}

void ReportEditorWindow::onInsertImage()
{
    const int idx = m_editor->currentBlockIndex();
    m_editor->insertBlock(idx + 1, BlockType::Image);
}

void ReportEditorWindow::onInsertDivider()
{
    const int idx = m_editor->currentBlockIndex();
    m_editor->insertBlock(idx + 1, BlockType::Divider);
}

// ===========================================================================
// 视图操作
// ===========================================================================

void ReportEditorWindow::onToggleFullscreen()
{
    if (isFullScreen()) showNormal();
    else showFullScreen();
}

void ReportEditorWindow::onZoomIn()
{
    m_zoomFactor = qMin(2.0, m_zoomFactor + 0.1);
    // 应用缩放到编辑器（后续实现）
}

void ReportEditorWindow::onZoomOut()
{
    m_zoomFactor = qMax(0.5, m_zoomFactor - 0.1);
}

void ReportEditorWindow::onResetZoom()
{
    m_zoomFactor = 1.0;
}

// ===========================================================================
// 编辑器信号
// ===========================================================================

void ReportEditorWindow::onContentChanged()
{
    updateWindowTitle();
    m_statusWordLabel->setText(tr("字数: %1").arg(m_editor->wordCount()));
}

void ReportEditorWindow::onTitleChanged(const QString& title)
{
    Q_UNUSED(title);
    updateWindowTitle();
}

void ReportEditorWindow::onSaveTriggered()
{
    // 自动保存触发
    if (saveReport()) {
        // 通知自动保存管理器成功
        // 注意：这里简化处理，实际应通过 AutoSaveManager 的 markSaveSuccess
    }
}

void ReportEditorWindow::onSaveStateChanged(bool saved)
{
    if (saved) {
        m_statusSaveLabel->setText(tr("已保存"));
        m_statusSaveLabel->setStyleSheet("color: #67C23A; padding: 0 8px;");
    } else {
        m_statusSaveLabel->setText(tr("未保存"));
        m_statusSaveLabel->setStyleSheet("color: #E6A23C; padding: 0 8px;");
    }
    updateWindowTitle();
}

// ===========================================================================
// 内部方法
// ===========================================================================

bool ReportEditorWindow::saveReport()
{
    m_report = m_editor->saveToReport();

    bool success = false;
    if (m_isNewReport) {
        success = ReportRepository::insert(m_report);
        if (success) {
            m_isNewReport = false;
            LOG_INFO(QString("新报告已保存: id=%1").arg(m_report->id()));
        }
    } else {
        success = ReportRepository::update(m_report);
    }

    if (success) {
        emit reportSaved(m_report->id());
        updateWindowTitle();
    } else {
        QMessageBox::critical(this, tr("保存失败"),
            tr("保存报告时发生错误，请查看日志。"));
    }

    return success;
}

void ReportEditorWindow::updateWindowTitle()
{
    QString title = m_editor->reportTitle();
    if (title.isEmpty()) title = tr("未命名报告");

    if (m_editor->isModified()) {
        title += " *";  // 未保存标记
    }

    setWindowTitle(QString("%1 - %2").arg(title, AppConstants::APP_DISPLAY_NAME));
}

void ReportEditorWindow::updateActionsState()
{
    m_actionSave->setEnabled(m_editor->isModified());
}

void ReportEditorWindow::showStatusMessage(const QString &msg, int timeout)
{
    // statusBar()->showMessage 展示提示
    statusBar()->showMessage(msg, timeout);
}

Report::Ptr ReportEditorWindow::currentReport() const
{
    return m_report;
}

bool ReportEditorWindow::isModified() const
{
    return m_editor ? m_editor->isModified() : false;
}

void ReportEditorWindow::applyFormatToCurrentBlock(const QString& format)
{
    const int idx = m_editor->currentBlockIndex();
    if (idx < 0) return;

    BlockEditor* editor = m_editor->blockEditorAt(idx);
    if (TextBlockEditor* textEditor = qobject_cast<TextBlockEditor*>(editor)) {
        textEditor->applyFormat(format);
    }
}

// ===========================================================================
// 事件处理
// ===========================================================================

void ReportEditorWindow::closeEvent(QCloseEvent* event)
{
    if (m_editor->isModified()) {
        const auto ret = QMessageBox::warning(
            this, tr("未保存的更改"),
            tr("报告「%1」有未保存的更改，是否保存？")
                .arg(m_editor->reportTitle()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save);

        if (ret == QMessageBox::Save) {
            if (!saveReport()) {
                event->ignore();
                return;
            }
        } else if (ret == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }

    emit windowClosed(m_report ? m_report->id() : -1);
    event->accept();
}
