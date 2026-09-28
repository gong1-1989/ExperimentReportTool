/**
 * @file ReportEditorWindow.cpp
 * @brief 报告编辑窗口实现文件
 */

#include "ReportEditorWindow.h"
#include <QStyle>
#include "ui_ReportEditorWindow.h"  // 由 uic 工具从 .ui 文件自动生成
#include "editor/ReportEditor.h"
#include "editor/TextBlockEditor.h"
#include "export/ExportManager.h"
#include "print/PrintManager.h"
#include "data/repositories/TagRepository.h"
#include "service/ReportService.h"
#include "data/repositories/ReportRepository.h"
#include "core/plugin/PluginManager.h"
#include "core/utils/Logger.h"
#include "core/utils/AppConstants.h"
#include "core/utils/AppTheme.h"
#include "core/utils/AppDimensions.h"
#include "core/utils/AppConfig.h"
#include "core/utils/UserSession.h"
#include "ui/dialogs/VersionHistoryDialog.h"
#include "ui/dialogs/AttachmentManagerDialog.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include "ui/UiHelper.h"
#include <QFileDialog>
#include <QCloseEvent>
#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QComboBox>
#include <QMenu>
#include <QFile>

// ===========================================================================
// 构造与析构
// ===========================================================================

ReportEditorWindow::ReportEditorWindow(const Report::Ptr& report, QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::ReportEditorWindow)  // 创建 UI 界面对象
    , m_editor(nullptr)
    , m_report(report)
    , m_printManager(nullptr)
    , m_pluginManager(nullptr)
    , m_statusSaveLabel(nullptr)
    , m_statusWordLabel(nullptr)
    , m_statusBlockLabel(nullptr)
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
    ui->setupUi(this);  // 从 .ui 文件加载界面

    // 动态创建 ReportEditor 组件并设置为中央控件
    m_editor = new ReportEditor(this);
    setCentralWidget(m_editor);

    m_printManager = new PrintManager(this);
    createActions();
    connectSignals();

    // 初始化状态栏
    initStatusBar();

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
    // 初始化状态栏
    m_statusWordLabel->setText(tr("字数: %1").arg(m_editor->wordCount()));
    m_statusBlockLabel->setText(tr("块: %1").arg(m_editor->blockCount()));
    resize(AppDimensions::Window::EditorWidth, AppDimensions::Window::EditorHeight);

    LOG_INFO(QString("报告编辑窗口已打开: %1")
                 .arg(m_isNewReport ? "新建报告" : m_report->title()));
}

ReportEditorWindow::~ReportEditorWindow()
{
    delete ui;
}

// ===========================================================================
// UI 初始化
// ===========================================================================

void ReportEditorWindow::createActions()
{
    // 动作已在 ReportEditorWindow.ui 中定义（含菜单/工具栏引用与快捷键），
    // 此处仅取出指针、设置图标并连接信号
    m_actionSave = ui->m_actionSave;
    m_actionUndo = ui->m_actionUndo;
    m_actionRedo = ui->m_actionRedo;
    m_actionBold = ui->m_actionBold;
    m_actionItalic = ui->m_actionItalic;
    m_actionUnderline = ui->m_actionUnderline;
    m_actionVersionHistory = ui->m_actionVersionHistory;
    m_actionManageAttachments = ui->m_actionManageAttachments;

    m_actionSave->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_actionUndo->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    m_actionRedo->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    m_actionVersionHistory->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    m_actionManageAttachments->setIcon(style()->standardIcon(QStyle::SP_DirLinkIcon));

    // 成员动作连接
    connect(m_actionSave, &QAction::triggered, this, &ReportEditorWindow::onSave);
    connect(m_actionBold, &QAction::triggered, this, &ReportEditorWindow::onBold);
    connect(m_actionItalic, &QAction::triggered, this, &ReportEditorWindow::onItalic);
    connect(m_actionUnderline, &QAction::triggered, this, &ReportEditorWindow::onUnderline);

    // 一次性动作连接（原 createMenus 中的 addAction(tr(...), this, &Xxx) 移至此）
    connect(ui->actionNew, &QAction::triggered, this, &ReportEditorWindow::onNew);
    connect(ui->actionSaveAs, &QAction::triggered, this, &ReportEditorWindow::onSaveAs);
    connect(ui->actionExport, &QAction::triggered, this, &ReportEditorWindow::onExport);
    connect(ui->actionPrintPreview, &QAction::triggered, this, &ReportEditorWindow::onPrintPreview);
    connect(ui->actionPrint, &QAction::triggered, this, &ReportEditorWindow::onPrint);
    connect(ui->actionPageSetup, &QAction::triggered, this, &ReportEditorWindow::onPageSetup);
    connect(ui->actionClose, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionFind, &QAction::triggered, this, &ReportEditorWindow::onFind);
    connect(ui->actionInsertTable, &QAction::triggered, this, &ReportEditorWindow::onInsertTable);
    connect(ui->actionInsertImage, &QAction::triggered, this, &ReportEditorWindow::onInsertImage);
    connect(ui->actionInsertDivider, &QAction::triggered, this, &ReportEditorWindow::onInsertDivider);
    connect(ui->actionCodeBlock, &QAction::triggered, this, &ReportEditorWindow::onCodeBlock);
    connect(ui->actionHeading1, &QAction::triggered, this, [this]() { onHeading(1); });
    connect(ui->actionHeading2, &QAction::triggered, this, [this]() { onHeading(2); });
    connect(ui->actionHeading3, &QAction::triggered, this, [this]() { onHeading(3); });
    connect(ui->actionBulletList, &QAction::triggered, this, [this]() { onList(false); });
    connect(ui->actionNumberedList, &QAction::triggered, this, [this]() { onList(true); });
    connect(ui->actionQuote, &QAction::triggered, this, &ReportEditorWindow::onQuote);
    connect(ui->actionFullscreen, &QAction::triggered, this, &ReportEditorWindow::onToggleFullscreen);
    connect(ui->actionZoomIn, &QAction::triggered, this, &ReportEditorWindow::onZoomIn);
    connect(ui->actionZoomOut, &QAction::triggered, this, &ReportEditorWindow::onZoomOut);
    connect(ui->actionResetZoom, &QAction::triggered, this, &ReportEditorWindow::onResetZoom);

    // 工具栏标题下拉（已在 .ui 工具栏中定义）
    connect(ui->headingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) { onHeading(idx); });
}






void ReportEditorWindow::initStatusBar()
{
    QStatusBar* bar = statusBar();

    // 状态栏标签用 addWidget/addPermanentWidget 显式添加
    // （QStatusBar 子 widget 若仅写在 .ui 中，uic 不会生成 addWidget，导致控件堆叠重叠）
    m_statusSaveLabel = new QLabel(tr("已保存"), this);
    m_statusSaveLabel->setStyleSheet(QString("color: %1; padding: 0 8px;").arg(AppTheme::Color::Success));
    bar->addWidget(m_statusSaveLabel);

    m_statusBlockLabel = new QLabel(tr("块: 0"), this);
    m_statusBlockLabel->setStyleSheet(QString("color: %1; padding: 0 8px;").arg(AppTheme::Color::TextRegular));
    bar->addPermanentWidget(m_statusBlockLabel);

    m_statusWordLabel = new QLabel(tr("字数: 0"), this);
    m_statusWordLabel->setStyleSheet(QString("color: %1; padding: 0 8px;").arg(AppTheme::Color::TextRegular));
    bar->addPermanentWidget(m_statusWordLabel);

    m_statusPositionLabel = new QLabel(this);
    m_statusPositionLabel->setStyleSheet(QString("color: %1; padding: 0 8px;").arg(AppTheme::Color::TextRegular));
    bar->addPermanentWidget(m_statusPositionLabel);
}



void ReportEditorWindow::showStatusMessage(const QString& message, int timeout)
{
    // 在状态栏显示临时消息，timeout 毫秒后自动消失
    statusBar()->showMessage(message, timeout);
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

    // 工具菜单
    connect(m_actionVersionHistory, &QAction::triggered,
            this, &ReportEditorWindow::onVersionHistory);
    connect(m_actionManageAttachments, &QAction::triggered,
            this, &ReportEditorWindow::onManageAttachments);
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
        m_statusSaveLabel->setStyleSheet(
            QString("color: %1; padding: 0 %2px;")
                .arg(AppTheme::Color::Success).arg(AppTheme::Spacing::Normal));
        // 保存成功弹出提示框
        UiHelper::info(this, tr("保存成功"),
            tr("报告「%1」已成功保存。").arg(m_report->title().isEmpty() ? tr("未命名报告") : m_report->title()));
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
        copy->setAuthor(UserSession::instance().displayName());
        copy->setCreatedBy(UserSession::instance().userId());
        copy->setExperimentDate(m_report->experimentDate());

        // 复制内容块
        for (int i = 0; i < m_report->blockCount(); ++i) {
            ContentBlock block = m_report->blockAt(i);
            block.id = Report::generateBlockId();
            copy->appendBlock(block);
        }

        if (ReportService::save(copy)) {
            UiHelper::info(this, tr("另存为"),
                tr("报告已另存为「%1」").arg(copy->title()));
            emit reportSaved(copy->id());
        }
    }
}

void ReportEditorWindow::onExport()
{
    // 先保存当前报告
    if (!saveReport()) {
        UiHelper::warning(this, tr("导出失败"), tr("保存报告失败，无法导出"));
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
        UiHelper::info(this, tr("导出成功"),
            tr("报告已导出到:\n%1").arg(result.first));
        showStatusMessage(tr("导出成功: %1").arg(result.first));
    } else {
        UiHelper::error(this, tr("导出失败"),
            tr("导出报告时发生错误，请查看日志"));
    }
}

void ReportEditorWindow::onPrint()
{
    if (!m_report) return;

    // 先保存
    if (!saveReport()) {
        UiHelper::warning(this, tr("打印失败"), tr("保存报告失败，无法打印"));
        return;
    }

    m_printManager->print(m_report, this);
}

void ReportEditorWindow::onPrintPreview()
{
    if (!m_report) return;

    if (!saveReport()) {
        UiHelper::warning(this, tr("打印预览失败"), tr("保存报告失败，无法预览"));
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
    // 确保报告已保存（版本历史需要报告ID）
    if (!m_report || m_report->id() <= 0) {
        if (!saveReport()) {
            UiHelper::warning(this, tr("提示"), tr("请先保存报告"));
            return;
        }
    }

    VersionHistoryDialog dialog(m_report->id(), this);
    if (dialog.exec() == QDialog::Accepted) {
        // 版本恢复后重新加载报告
        Report::Ptr updated = ReportService::getById(m_report->id());
        if (updated) {
            m_report = updated;
            m_editor->loadReport(m_report);
            updateWindowTitle();
            showStatusMessage(tr("版本已恢复"));
        }
    }
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
    UiHelper::info(this, tr("查找"),
        tr("查找功能将在后续版本中实现。"));
}

void ReportEditorWindow::onManageAttachments()
{
    // 确保报告已保存（附件管理需要报告ID）
    if (!m_report || m_report->id() <= 0) {
        if (!saveReport()) {
            UiHelper::warning(this, tr("提示"), tr("请先保存报告"));
            return;
        }
    }

    AttachmentManagerDialog dialog(m_report->id(), this);
    dialog.exec();
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
    m_statusBlockLabel->setText(tr("块: %1").arg(m_editor->blockCount()));
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
    const QString savedStyle =
        QString("color: %1; padding: 0 %2px;")
            .arg(AppTheme::Color::Success).arg(AppTheme::Spacing::Normal);
    const QString unsavedStyle =
        QString("color: %1; padding: 0 %2px;")
            .arg(AppTheme::Color::Warning).arg(AppTheme::Spacing::Normal);

    if (saved) {
        m_statusSaveLabel->setText(tr("已保存"));
        m_statusSaveLabel->setStyleSheet(savedStyle);
    } else {
        m_statusSaveLabel->setText(tr("未保存"));
        m_statusSaveLabel->setStyleSheet(unsavedStyle);
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
        success = ReportService::save(m_report);
        if (success) {
            m_isNewReport = false;
            LOG_INFO(QString("新报告已保存: id=%1").arg(m_report->id()));
        }
    } else {
        // 权限检查：只有创建者或管理员可以修改已有报告
        if (m_report->createdBy() > 0
            && m_report->createdBy() != UserSession::instance().userId()
            && !UserSession::instance().isAdmin()) {
            UiHelper::warning(this, tr("权限不足"),
                tr("您没有权限修改此报告。\n只有创建者或管理员可以修改。"));
            return false;
        }
        success = ReportService::save(m_report);
    }

    if (success) {
        emit reportSaved(m_report->id());
        updateWindowTitle();
    } else {
        UiHelper::error(this, tr("保存失败"),
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

    setWindowTitle(QString("%1 - %2").arg(title).arg(AppConstants::APP_DISPLAY_NAME));
}

void ReportEditorWindow::updateActionsState()
{
    // 保存按钮：新报告始终可保存，已有报告在修改后可保存
    m_actionSave->setEnabled(m_isNewReport || m_editor->isModified());
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
        const auto ret = UiHelper::warning(
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
