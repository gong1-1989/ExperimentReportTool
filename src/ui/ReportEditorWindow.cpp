/**
 * @file ReportEditorWindow.cpp
 * @brief 报告编辑窗口实现文件
 */

#include "ReportEditorWindow.h"
#include <QStyle>
#include "ui_ReportEditorWindow.h"  // 由 uic 工具从 .ui 文件自动生成
#include "editor/ReportEditor.h"
#include <QTextCursor>
#include <QTextList>
#include <QTextBlock>
#include <QPainter>
#include <QIcon>
#include <QColorDialog>
#include "print/PrintManager.h"
#include "core/models/Template.h"
#include "service/ReportService.h"
#include "service/TemplateService.h"
#include "core/utils/Logger.h"
#include "core/utils/AppConstants.h"
#include "core/utils/AppTheme.h"
#include "core/utils/AppDimensions.h"
#include "core/utils/UserSession.h"
#include "ui/dialogs/VersionHistoryDialog.h"
#include "ui/dialogs/AttachmentManagerDialog.h"
#include "ui/dialogs/FindTextDialog.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include "ui/ObjectInsertionController.h"
#include "ui/ReportExportController.h"
#include "ui/UiHelper.h"
#include "data/repositories/UserRepository.h"
#include "editor/DocumentTextEdit.h"
#include "export/ExportManager.h"
#include "data/repositories/TagRepository.h"
#include "data/repositories/ReportRepository.h"
#include "data/repositories/DataTableRepository.h"
#include "core/plugin/PluginManager.h"
#include "core/utils/AppConfig.h"
#include "extension/DocumentTool.h"
#include "dialogs/WritingToolsDialog.h"
#include <QDateTime>
#include <QCloseEvent>
#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QColor>
#include <QToolButton>
#include <QMenu>
#include <QInputDialog>
#include <QLineEdit>
#include <QActionGroup>
#include <QFile>
#include <QTimer>

// ===========================================================================
// 构造与析构
// ===========================================================================

QString ReportEditorWindow::reportTitle() const
{
    return m_editor ? m_editor->reportTitle() : QString();
}

ReportEditorWindow::ReportEditorWindow(const Report::Ptr& report, QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::ReportEditorWindow)  // 创建 UI 界面对象
    , m_editor(nullptr)
    , m_report(report)
    , m_loadedUpdatedAt(report ? report->updatedAt() : QDateTime())
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

    // 动态创建 ReportEditor 组件，用 QWidget 包裹以容纳顶部工作流栏
    m_editor = new ReportEditor(this);
    QWidget* centralWrapper = new QWidget(this);
    QVBoxLayout* wrapperLayout = new QVBoxLayout(centralWrapper);
    wrapperLayout->setContentsMargins(0, 0, 0, 0);
    wrapperLayout->setSpacing(0);

    // 顶部工作流信息栏（退回意见 + 状态提示），默认隐藏
    m_workflowBar = new QWidget(centralWrapper);
    m_workflowBar->setStyleSheet("background:#fff3cd;border-bottom:1px solid #ffc107;padding:4px 8px;");
    QHBoxLayout* barLayout = new QHBoxLayout(m_workflowBar);
    barLayout->setContentsMargins(4, 2, 4, 2);
    m_rejectCommentLabel = new QLabel(m_workflowBar);
    m_rejectCommentLabel->setWordWrap(true);
    m_rejectCommentLabel->setStyleSheet("color:#856404;");
    barLayout->addWidget(m_rejectCommentLabel);
    m_workflowBar->setVisible(false);
    wrapperLayout->addWidget(m_workflowBar);
    wrapperLayout->addWidget(m_editor);
    setCentralWidget(centralWrapper);

    m_printManager = new PrintManager(this);
    m_insertionController = new ObjectInsertionController(this, m_editor);
    m_exportController = new ReportExportController(this, m_printManager,
        [this](const QString& msg) { showStatusMessage(msg); });
    createActions();
    connectSignals();

    // 初始化状态栏
    initStatusBar();

    // 加载报告
    if (m_report) {
        m_editor->loadReport(m_report);
        // 权限：使用 PermissionService 判断编辑权限
        m_readOnly = !PermissionService::canEditReport(m_report, UserSession::instance().currentUser());
        m_editor->setReadOnly(m_readOnly);
        m_actionSave->setEnabled(!m_readOnly);   // 只读时禁用保存按钮
        ui->m_lineHeightCombo->setEnabled(!m_readOnly);
        m_actionAlignLeft->setEnabled(!m_readOnly);
        m_actionAlignCenter->setEnabled(!m_readOnly);
        m_actionAlignRight->setEnabled(!m_readOnly);
    } else {
        // 新建报告
        m_report = Report::create();
        m_report->setTitle(tr("未命名实验报告"));
        m_editor->loadReport(m_report);
    }

    // 延迟一帧再刷新一次工具栏状态（确保首次打开/加载后回显正确）
    QTimer::singleShot(0, this, [this]() {
        if (m_editor) m_editor->refreshFormattingState();
    });

    // 创建工作流动作（动态加到文件菜单）
    createWorkflowActions();
    updateWorkflowActions();
    refreshRejectComment();

    updateWindowTitle();
    // 初始化状态栏
    m_statusWordLabel->setText(tr("字数: %1").arg(m_editor->wordCount()));
    m_statusBlockLabel->setText(tr("对象: %1").arg(m_editor->objectCount()));
    resize(AppDimensions::Window::EditorWidth, AppDimensions::Window::EditorHeight);

    LOG_DEBUG(QString("报告编辑窗口已打开: %1")
                 .arg(m_isNewReport ? "新建报告" : m_report->title()));
}

ReportEditorWindow::~ReportEditorWindow()
{
    delete m_insertionController;
    delete m_exportController;
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
    m_actionAlignLeft = ui->m_actionAlignLeft;
    m_actionAlignCenter = ui->m_actionAlignCenter;
    m_actionAlignRight = ui->m_actionAlignRight;

    m_actionSave->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_actionUndo->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    m_actionRedo->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    // 显式设置快捷键（确保与块编辑控件的 Ctrl+Z/Y 拦截一致）
    m_actionUndo->setShortcut(QKeySequence::Undo);
    m_actionRedo->setShortcut(QKeySequence::Redo);
    m_actionVersionHistory->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    // 对齐图标（自绘三条线示意）
    m_actionAlignLeft->setIcon(makeAlignIcon(Qt::AlignLeft));
    m_actionAlignCenter->setIcon(makeAlignIcon(Qt::AlignCenter));
    m_actionAlignRight->setIcon(makeAlignIcon(Qt::AlignRight));
    // 对齐按钮互斥（同一时间只亮一个）
    QActionGroup* alignGroup = new QActionGroup(this);
    alignGroup->setExclusive(true);
    alignGroup->addAction(m_actionAlignLeft);
    alignGroup->addAction(m_actionAlignCenter);
    alignGroup->addAction(m_actionAlignRight);
    m_actionManageAttachments->setIcon(style()->standardIcon(QStyle::SP_DirLinkIcon));

    // 成员动作连接
    connect(m_actionSave, &QAction::triggered, this, &ReportEditorWindow::onSave);
    connect(m_actionUndo, &QAction::triggered, this, &ReportEditorWindow::onUndo);
    connect(m_actionRedo, &QAction::triggered, this, &ReportEditorWindow::onRedo);
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
    connect(ui->actionInsertChart, &QAction::triggered, this, &ReportEditorWindow::onInsertChart);
    connect(ui->actionInsertFormula, &QAction::triggered, this, &ReportEditorWindow::onInsertFormula);
    connect(ui->actionInsertImage, &QAction::triggered, this, &ReportEditorWindow::onInsertImage);
    connect(ui->actionInsertDivider, &QAction::triggered, this, &ReportEditorWindow::onInsertDivider);
    connect(ui->actionBulletList, &QAction::triggered, this, [this]() { onList(false); });
    connect(ui->actionNumberedList, &QAction::triggered, this, [this]() { onList(true); });
    connect(ui->actionQuote, &QAction::triggered, this, &ReportEditorWindow::onQuote);
    connect(ui->actionFullscreen, &QAction::triggered, this, &ReportEditorWindow::onToggleFullscreen);
    connect(ui->actionZoomIn, &QAction::triggered, this, &ReportEditorWindow::onZoomIn);
    connect(ui->actionZoomOut, &QAction::triggered, this, &ReportEditorWindow::onZoomOut);
    connect(ui->actionResetZoom, &QAction::triggered, this, &ReportEditorWindow::onResetZoom);

    // 字体大小下拉（.ui 已定义，现接入功能）
    ui->fontSizeCombo->setEditable(true);
    ui->fontSizeCombo->setInsertPolicy(QComboBox::NoInsert);
    connect(ui->fontSizeCombo, &QComboBox::currentTextChanged,
            this, [this](const QString& text) {
                if (m_updatingFormat) return;
                bool ok = false;
                const int size = text.toInt(&ok);
                if (ok && size >= 6 && size <= 120) onFontSize(size);
            });
    // 用户点击列表项时直接应用（不依赖 editable 文本框的回流，最可靠）
    connect(ui->fontSizeCombo, &QComboBox::activated,
            this, [this](int index) {
                if (m_updatingFormat) return;
                bool ok = false;
                const int size = ui->fontSizeCombo->itemText(index).toInt(&ok);
                if (ok && size >= 6 && size <= 120) onFontSize(size);
            });

    // 行高下拉
    connect(ui->m_lineHeightCombo, &QComboBox::currentTextChanged,
            this, [this](const QString& text) {
                if (m_updatingFormat) return;
                bool ok = false;
                const qreal lh = text.toDouble(&ok);
                if (ok && lh >= 0.5 && lh <= 5.0) {
                    m_editor->applyLineHeight(lh);
                }
            });
    connect(ui->m_lineHeightCombo, &QComboBox::activated,
            this, [this](int index) {
                if (m_updatingFormat) return;
                bool ok = false;
                const qreal lh = ui->m_lineHeightCombo->itemText(index).toDouble(&ok);
                if (ok && lh >= 0.5 && lh <= 5.0) {
                    m_editor->applyLineHeight(lh);
                }
            });

    // 对齐按钮
    connect(m_actionAlignLeft, &QAction::triggered,
            this, [this]() { m_editor->applyParagraphAlignment(Qt::AlignLeft); });
    connect(m_actionAlignCenter, &QAction::triggered,
            this, [this]() { m_editor->applyParagraphAlignment(Qt::AlignCenter); });
    connect(m_actionAlignRight, &QAction::triggered,
            this, [this]() { m_editor->applyParagraphAlignment(Qt::AlignRight); });

    // 文字颜色按钮
    connect(ui->m_colorBtn, &QToolButton::clicked,
            this, &ReportEditorWindow::onTextColor);

    // 写作工具（F 域：字数统计/常用片段/质量检查）
    m_actionWritingTools = new QAction(tr("写作工具"), this);
    m_actionWritingTools->setToolTip(tr("字数统计、常用片段、质量检查"));
    connect(m_actionWritingTools, &QAction::triggered,
            this, &ReportEditorWindow::onWritingTools);
    ui->formatToolBar->addAction(m_actionWritingTools);

    // D 域文档对象：附件卡片 / 音视频引用（对象类型可插拔框架）
    m_actionInsertAttachmentCard = new QAction(tr("附件卡片"), this);
    m_actionInsertAttachmentCard->setToolTip(tr("以卡片形式插入附件文件"));
    connect(m_actionInsertAttachmentCard, &QAction::triggered, this,
            [this]() { if (m_insertionController) m_insertionController->insertAttachmentCard(); });
    ui->menuInsert->addAction(m_actionInsertAttachmentCard);
    m_actionInsertMediaRef = new QAction(tr("音视频引用"), this);
    m_actionInsertMediaRef->setToolTip(tr("插入视频/音频文件引用（导出为 HTML5 播放器）"));
    connect(m_actionInsertMediaRef, &QAction::triggered, this,
            [this]() { if (m_insertionController) m_insertionController->insertMediaRef(); });
    ui->menuInsert->addAction(m_actionInsertMediaRef);
}






void ReportEditorWindow::initStatusBar()
{
    QStatusBar* bar = statusBar();

    // 状态栏标签用 addWidget/addPermanentWidget 显式添加
    // （QStatusBar 子 widget 若仅写在 .ui 中，uic 不会生成 addWidget，导致控件堆叠重叠）
    m_statusSaveLabel = new QLabel(tr("已保存"), this);
    m_statusSaveLabel->setStyleSheet(QString("color: %1; padding: 0 8px;").arg(AppTheme::Color::Success));
    bar->addWidget(m_statusSaveLabel);

    m_statusBlockLabel = new QLabel(tr("对象: 0"), this);
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
    connect(m_editor, &ReportEditor::objectDoubleClicked,
            this, &ReportEditorWindow::onObjectEdit);

    // 撤销/重做按钮随栈状态置灰
    connect(m_editor, &ReportEditor::undoAvailableChanged,
            this, [this](bool available) { m_actionUndo->setEnabled(available); });
    connect(m_editor, &ReportEditor::redoAvailableChanged,
            this, [this](bool available) { m_actionRedo->setEnabled(available); });
    m_actionUndo->setEnabled(false);
    m_actionRedo->setEnabled(false);

    // 格式化状态同步：字号回显 / B/I/U 按钮状态 / 光标位置
    connect(m_editor, &ReportEditor::formattingStateChanged,
            this, [this](int fontSize, bool bold, bool italic, bool underline, int line, int col,
                         Qt::Alignment alignment, qreal lineHeightMultiplier,
                         int headingLevel, const QColor& textColor) {
                Q_UNUSED(headingLevel);
                m_actionBold->setChecked(bold);
                m_actionItalic->setChecked(italic);
                m_actionUnderline->setChecked(underline);
                // 对齐按钮状态
                m_actionAlignLeft->setChecked(alignment.testFlag(Qt::AlignLeft));
                m_actionAlignCenter->setChecked(alignment.testFlag(Qt::AlignHCenter));
                m_actionAlignRight->setChecked(alignment.testFlag(Qt::AlignRight));
                // 下拉回显统一加锁，防止触发各自的应用槽（循环/破坏格式）
                m_updatingFormat = true;
                // 字号回显
                const QString sizeText = QString::number(fontSize);
                if (ui->fontSizeCombo->currentText() != sizeText) {
                    const int sIdx = ui->fontSizeCombo->findText(sizeText);
                    if (sIdx >= 0) ui->fontSizeCombo->setCurrentIndex(sIdx);
                    else ui->fontSizeCombo->setEditText(sizeText);
                }
                // 行高回显：数值匹配列表项
                // （"1.00" 与列表项 "1.0" 字符串不匹配，findText 会失败；
                //  且行高下拉不可编辑，setEditText 无效 → 必须按数值 setCurrentIndex）
                bool lhMatched = false;
                for (int i = 0; i < ui->m_lineHeightCombo->count(); ++i) {
                    bool ok = false;
                    const qreal itemVal = ui->m_lineHeightCombo->itemText(i).toDouble(&ok);
                    if (ok && qFuzzyCompare(itemVal, lineHeightMultiplier)) {
                        ui->m_lineHeightCombo->setCurrentIndex(i);
                        lhMatched = true;
                        break;
                    }
                }
                if (!lhMatched) ui->m_lineHeightCombo->setCurrentIndex(-1);
                m_updatingFormat = false;
                // 文字颜色回显（按钮文字显示当前颜色）
                ui->m_colorBtn->setStyleSheet(
                    QString("color: %1; font-weight: bold; font-size: 13px; padding: 2px 8px;"
                            "border: 1px solid #DCDFE6; border-radius: 4px; background: white;")
                        .arg(textColor.name()));
                m_statusPositionLabel->setText(tr("行: %1 列: %2").arg(line).arg(col));
            });

    // 双保险：直接连编辑控件的光标/选区信号 → 强制刷新工具栏回显
    // （即使 ReportEditor 内部连接异常，光标切换/选中变化也能驱动回显）
    connect(m_editor->textEdit(), &QTextEdit::cursorPositionChanged,
            this, [this]() { m_editor->refreshFormattingState(); });
    connect(m_editor->textEdit(), &QTextEdit::selectionChanged,
            this, [this]() { m_editor->refreshFormattingState(); });

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
    if (!checkConflictBeforeSave()) return;  // 打开后被他人修改：用户取消则不保存
    if (saveReport()) {
        m_statusSaveLabel->setText(tr("已保存"));
        m_statusSaveLabel->setStyleSheet(
            QString("color: %1; padding: 0 %2px;")
                .arg(AppTheme::Color::Success).arg(AppTheme::Spacing::Normal));
        // 手动保存时自动生成版本快照（自动保存不生成，避免版本过多）
        if (m_report && m_report->id() > 0) {
            const QString snapshotName = QDateTime::currentDateTime()
                .toString("yyyy-MM-dd hh:mm:ss");
            ReportService::saveVersion(m_report->id(), snapshotName);
            m_loadedUpdatedAt = m_report->updatedAt();  // 保存成功后刷新冲突基准
        }
        // 保存成功弹出提示框
        UiHelper::info(this, tr("保存成功"),
            tr("报告「%1」已成功保存。").arg(m_report->title().isEmpty() ? tr("未命名报告") : m_report->title()));
    }
}

bool ReportEditorWindow::checkConflictBeforeSave()
{
    if (m_isNewReport || !m_report || m_report->id() <= 0) return true;
    Report::Ptr fresh = ReportService::getById(m_report->id());
    if (!fresh) return true;
    if (fresh->updatedAt() > m_loadedUpdatedAt) {
        return UiHelper::confirm(this, tr("检测到并发修改"),
            tr("该报告在您打开后已被其他人修改。\n"
               "继续保存将覆盖对方的修改内容。\n\n是否仍然保存？"));
    }
    return true;
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

        // 复制内容（连续文档 + 对象）
        copy->setDocument(m_report->document());
        QList<ContentBlock> objects = m_report->objects();
        for (ContentBlock& obj : objects) {
            obj.id = Report::generateObjectId();
        }
        copy->setObjects(objects);

        if (ReportService::save(copy)) {
            UiHelper::info(this, tr("另存为"),
                tr("报告已另存为「%1」").arg(copy->title()));
            emit reportSaved(copy->id());
        }
    }
}

void ReportEditorWindow::onExport()
{
    if (!m_report) return;

    // 可编辑时先保存当前报告；只读模式直接用当前内容导出
    if (!m_readOnly && !saveReport()) {
        UiHelper::warning(this, tr("导出失败"), tr("保存报告失败，无法导出"));
        return;
    }

    if (m_exportController) m_exportController->exportReport(m_report);
}

void ReportEditorWindow::onPrint()
{
    if (!m_report) return;

    // 可编辑时先保存；只读模式直接用当前内容打印
    if (!m_readOnly && !saveReport()) {
        UiHelper::warning(this, tr("打印失败"), tr("保存报告失败，无法打印"));
        return;
    }

    if (m_exportController) m_exportController->print(m_report);
}

void ReportEditorWindow::onPrintPreview()
{
    if (!m_report) return;

    if (!m_readOnly && !saveReport()) {
        UiHelper::warning(this, tr("打印预览失败"), tr("保存报告失败，无法预览"));
        return;
    }

    if (m_exportController) m_exportController->printPreview(m_report);
}

void ReportEditorWindow::onPageSetup()
{
    if (m_exportController) m_exportController->pageSetup();
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
            m_loadedUpdatedAt = updated->updatedAt();
            m_editor->loadReport(m_report);
            // 权限：非创建者且非管理员 → 只读
            m_readOnly = (m_report->createdBy() > 0
                && m_report->createdBy() != UserSession::instance().userId()
                && !UserSession::instance().isAdmin());
            m_editor->setReadOnly(m_readOnly);
            m_actionSave->setEnabled(!m_readOnly);   // 只读时禁用保存按钮
            ui->m_lineHeightCombo->setEnabled(!m_readOnly);
            m_actionAlignLeft->setEnabled(!m_readOnly);
            m_actionAlignCenter->setEnabled(!m_readOnly);
            m_actionAlignRight->setEnabled(!m_readOnly);
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
    if (m_editor) m_editor->undo();
}

void ReportEditorWindow::onRedo()
{
    if (m_editor) m_editor->redo();
}

void ReportEditorWindow::onFind()
{
    if (!m_editor) return;

    // 收集文档段落纯文本（连续文档按段落切分）
    const QStringList paragraphs = m_editor->documentParagraphs();

    FindTextDialog dialog(this);
    dialog.setBlocks(paragraphs);
    connect(&dialog, &FindTextDialog::jumpRequested, this,
            [this](int paragraphIndex) {
                m_editor->scrollToParagraph(paragraphIndex);
            });
    dialog.exec();
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
    if (m_editor) m_editor->setBold(m_actionBold->isChecked());
}

void ReportEditorWindow::onItalic()
{
    if (m_editor) m_editor->setItalic(m_actionItalic->isChecked());
}

void ReportEditorWindow::onUnderline()
{
    if (m_editor) m_editor->setUnderline(m_actionUnderline->isChecked());
}

void ReportEditorWindow::onFontSize(int size)
{
    if (size <= 0) return;
    if (m_editor) m_editor->setFontSize(size);
}

QIcon ReportEditorWindow::makeAlignIcon(Qt::Alignment align) const
{
    QPixmap pixmap(28, 28);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(QPalette().color(QPalette::WindowText), 2);
    painter.setPen(pen);
    const int x1 = 4, x2 = 24, yTop = 7, gap = 8;
    // 三条线：左对齐全部靠左；居中按文本宽度居中；右对齐靠右
    const int widths[3] = { 14, 18, 10 };
    for (int i = 0; i < 3; ++i) {
        const int y = yTop + i * gap;
        int lx = x1, rx = x1 + widths[i];
        if (align == Qt::AlignCenter) {
            lx = x1 + (x2 - x1 - widths[i]) / 2;
            rx = lx + widths[i];
        } else if (align == Qt::AlignRight) {
            lx = x2 - widths[i];
            rx = x2;
        }
        painter.drawLine(lx, y, rx, y);
    }
    return QIcon(pixmap);
}

void ReportEditorWindow::onTextColor()
{
    const QColor color = QColorDialog::getColor(Qt::black, this, tr("选择文字颜色"));
    if (!color.isValid()) return;

    if (m_editor) m_editor->setTextColor(color);
}

void ReportEditorWindow::onList(bool numbered)
{
    if (m_editor) m_editor->setList(numbered);
}

void ReportEditorWindow::onQuote()
{
    if (m_editor) m_editor->setQuote();
}

void ReportEditorWindow::onInsertTable()
{
    if (m_insertionController) m_insertionController->insertTable();
}

void ReportEditorWindow::onInsertImage()
{
    if (m_insertionController) m_insertionController->insertImage();
}

void ReportEditorWindow::onInsertChart()
{
    if (m_insertionController) m_insertionController->insertChart();
}

void ReportEditorWindow::onInsertFormula()
{
    if (m_insertionController) m_insertionController->insertFormula();
}

void ReportEditorWindow::onInsertDivider()
{
    if (m_insertionController) m_insertionController->insertDivider();
}

void ReportEditorWindow::onObjectEdit(const QString& objectId)
{
    if (m_insertionController) m_insertionController->editObject(objectId);
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
    m_statusBlockLabel->setText(tr("对象: %1").arg(m_editor->objectCount()));
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
    LOG_DEBUG(QString("保存报告开始: %1").arg(m_report ? m_report->title() : QStringLiteral("(空)")));
    m_report = m_editor->saveToReport();

    bool success = false;
    if (m_isNewReport) {
        // 新建报告：修改者 = 当前创建用户
        m_report->setModifiedBy(UserSession::instance().userId());
        success = ReportService::save(m_report);
        if (success) {
            m_isNewReport = false;
            LOG_INFO(QString("新报告已保存: id=%1").arg(m_report->id()));
        }
    } else {
        // 权限检查：统一走 PermissionService（草稿的创建者 + 超管可编辑）
        if (!PermissionService::canEditReport(m_report, UserSession::instance().currentUser())) {
            UiHelper::warning(this, tr("权限不足"),
                tr("您没有权限修改此报告。\n只有草稿的创建者或管理员可以修改。"));
            return false;
        }
        // 记录最后修改者
        m_report->setModifiedBy(UserSession::instance().userId());
        success = ReportService::save(m_report);
    }

    if (success) {
        m_editor->saveReportTags();   // 报告入库后保存标签
        m_editor->setModified(false); // 保存成功清除修改标记（关闭不再误弹未保存提示）
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

// ===========================================================================
// 工作流操作（提交/审核/审批）
// ===========================================================================

void ReportEditorWindow::createWorkflowActions()
{
    m_actionSubmit = new QAction(tr("提交报告"), this);
    m_actionReviewApprove = new QAction(tr("审核通过"), this);
    m_actionReviewReject = new QAction(tr("审核退回"), this);
    m_actionApproveApprove = new QAction(tr("审批通过"), this);
    m_actionApproveReject = new QAction(tr("审批退回"), this);

    connect(m_actionSubmit, &QAction::triggered, this, &ReportEditorWindow::onSubmit);
    m_actionRecall = new QAction(tr("撤回提交"), this);
    connect(m_actionRecall, &QAction::triggered, this, &ReportEditorWindow::onRecall);
    connect(m_actionReviewApprove, &QAction::triggered, this, &ReportEditorWindow::onReviewApprove);
    connect(m_actionReviewReject, &QAction::triggered, this, &ReportEditorWindow::onReviewReject);
    connect(m_actionApproveApprove, &QAction::triggered, this, &ReportEditorWindow::onApproveApprove);
    connect(m_actionApproveReject, &QAction::triggered, this, &ReportEditorWindow::onApproveReject);

    // 归档报告（仅已审批可归档，归档后只读存档）
    m_actionArchive = new QAction(tr("归档报告"), this);
    connect(m_actionArchive, &QAction::triggered, this, &ReportEditorWindow::onArchive);

    // 加到文件菜单末尾，加分隔线
    ui->menuFile->addSeparator();
    ui->menuFile->addAction(m_actionSubmit);
    ui->menuFile->addAction(m_actionRecall);
    ui->menuFile->addAction(m_actionReviewApprove);
    ui->menuFile->addAction(m_actionReviewReject);
    ui->menuFile->addAction(m_actionApproveApprove);
    ui->menuFile->addAction(m_actionApproveReject);

    // 保存为模板
    m_actionSaveAsTemplate = new QAction(tr("保存为模板"), this);
    connect(m_actionSaveAsTemplate, &QAction::triggered, this, &ReportEditorWindow::onSaveAsTemplate);
    ui->menuFile->addSeparator();
    ui->menuFile->addAction(m_actionSaveAsTemplate);
}

void ReportEditorWindow::updateWorkflowActions()
{
    if (!m_report || !m_report->isPersisted()) {
        m_actionSubmit->setVisible(false);
        m_actionRecall->setVisible(false);
        m_actionReviewApprove->setVisible(false);
        m_actionReviewReject->setVisible(false);
        m_actionApproveApprove->setVisible(false);
        m_actionApproveReject->setVisible(false);
        m_actionArchive->setVisible(false);
        return;
    }

    User::Ptr user = UserSession::instance().currentUser();
    m_actionSubmit->setVisible(PermissionService::canSubmitReport(m_report, user));
    m_actionRecall->setVisible(PermissionService::canRecallReport(m_report, user));
    m_actionReviewApprove->setVisible(PermissionService::canReviewReport(m_report, user));
    m_actionReviewReject->setVisible(PermissionService::canReviewReport(m_report, user));
    m_actionApproveApprove->setVisible(PermissionService::canApproveReport(m_report, user));
    m_actionApproveReject->setVisible(PermissionService::canApproveReport(m_report, user));
    m_actionArchive->setVisible(PermissionService::canArchiveReport(m_report, user));
}

void ReportEditorWindow::onArchive()
{
    if (!m_report || m_report->status() != ReportStatus::Approved) return;

    const bool confirmed = UiHelper::confirm(
        this, tr("归档报告"),
        tr("归档后报告将进入只读存档状态，不能再修改或流转。\n确定归档「%1」吗？")
            .arg(m_report->title()));
    if (!confirmed) return;

    if (!ReportService::updateStatus(m_report->id(), ReportStatus::Archived)) {
        UiHelper::error(this, tr("归档失败"), tr("归档报告时发生错误，请查看日志。"));
        return;
    }
    m_report = ReportService::getById(m_report->id());
    if (m_report) {
        m_editor->loadReport(m_report);
        m_loadedUpdatedAt = m_report->updatedAt();
        updateWorkflowActions();
        refreshRejectComment();
        showStatusMessage(tr("报告已归档"));
    }
}

void ReportEditorWindow::refreshRejectComment()
{
    if (!m_report || !m_workflowBar || !m_rejectCommentLabel) return;

    const QString action = m_report->lastAction();
    const QString comment = m_report->lastActionComment();

    // 仅在退回操作时显示意见栏
    if (action == "review_reject" || action == "approve_reject") {
        QString typeText = (action == "review_reject") ? tr("审核退回") : tr("审批退回");
        QString byName;
        if (m_report->lastActionBy() > 0) {
            User::Ptr byUser = UserRepository::findById(m_report->lastActionBy());
            if (byUser) byName = byUser->displayNameOrUsername();
        }
        m_rejectCommentLabel->setText(
            QString("⚠ %1意见（%2%3）：%4")
                .arg(typeText)
                .arg(byName.isEmpty() ? "" : byName + "，")
                .arg(m_report->lastActionAt().toString("yyyy-MM-dd hh:mm"))
                .arg(comment.toHtmlEscaped()));
        m_workflowBar->setVisible(true);
    } else {
        m_workflowBar->setVisible(false);
    }
}

void ReportEditorWindow::onRecall()
{
    if (!m_report || m_report->status() != ReportStatus::Submitted) return;

    const bool confirmed = UiHelper::confirm(
        this, tr("撤回提交"),
        tr("撤回后报告将回到草稿状态，可继续编辑。\n确定撤回「%1」的提交吗？")
            .arg(m_report->title()));
    if (!confirmed) return;

    WorkflowService::Result r = WorkflowService::recall(
        m_report->id(), UserSession::instance().userId());
    if (r.success) {
        m_report = ReportService::getById(m_report->id());
        if (m_report) {
            m_editor->loadReport(m_report);
            m_loadedUpdatedAt = m_report->updatedAt();
            updateWorkflowActions();
            refreshRejectComment();
            showStatusMessage(tr("已撤回提交，报告回到草稿状态"));
        }
    } else {
        UiHelper::error(this, tr("撤回失败"), r.errorMessage);
    }
}

void ReportEditorWindow::onSubmit()
{
    if (!m_report) return;
    // 先保存当前编辑内容
    onSave();
    WorkflowService::Result r = WorkflowService::submit(m_report->id(), UserSession::instance().userId());
    if (r.success) {
        m_report = ReportService::getById(m_report->id());
        m_editor->loadReport(m_report);
        if (m_report) m_loadedUpdatedAt = m_report->updatedAt();
        updateWorkflowActions();
        refreshRejectComment();
        showStatusMessage(tr("报告已提交，等待审核"));
    } else {
        UiHelper::error(this, tr("提交失败"), r.errorMessage);
    }
}

void ReportEditorWindow::onReviewApprove()
{
    if (!m_report) return;
    WorkflowService::Result r = WorkflowService::reviewApprove(
        m_report->id(), UserSession::instance().userId(), QString());
    if (r.success) {
        m_report = ReportService::getById(m_report->id());
        m_editor->loadReport(m_report);
        if (m_report) m_loadedUpdatedAt = m_report->updatedAt();
        updateWorkflowActions();
        refreshRejectComment();
        showStatusMessage(tr("审核通过，等待审批"));
    } else {
        UiHelper::error(this, tr("审核失败"), r.errorMessage);
    }
}

void ReportEditorWindow::onReviewReject()
{
    if (!m_report) return;
    bool ok = false;
    const QString comment = QInputDialog::getMultiLineText(
        this, tr("审核退回"), tr("请填写退回意见（必填）："), QString(), &ok);
    if (!ok || comment.trimmed().isEmpty()) {
        if (ok) UiHelper::error(this, tr("退回失败"), tr("退回意见不能为空"));
        return;
    }
    WorkflowService::Result r = WorkflowService::reviewReject(
        m_report->id(), UserSession::instance().userId(), comment);
    if (r.success) {
        m_report = ReportService::getById(m_report->id());
        m_editor->loadReport(m_report);
        if (m_report) m_loadedUpdatedAt = m_report->updatedAt();
        updateWorkflowActions();
        refreshRejectComment();
        showStatusMessage(tr("已退回，意见已记录"));
    } else {
        UiHelper::error(this, tr("退回失败"), r.errorMessage);
    }
}

void ReportEditorWindow::onApproveApprove()
{
    if (!m_report) return;
    WorkflowService::Result r = WorkflowService::approveApprove(
        m_report->id(), UserSession::instance().userId(), QString());
    if (r.success) {
        m_report = ReportService::getById(m_report->id());
        m_editor->loadReport(m_report);
        if (m_report) m_loadedUpdatedAt = m_report->updatedAt();
        updateWorkflowActions();
        refreshRejectComment();
        showStatusMessage(tr("审批通过"));
    } else {
        UiHelper::error(this, tr("审批失败"), r.errorMessage);
    }
}

void ReportEditorWindow::onApproveReject()
{
    if (!m_report) return;
    bool ok = false;
    const QString comment = QInputDialog::getMultiLineText(
        this, tr("审批退回"), tr("请填写退回意见（必填）："), QString(), &ok);
    if (!ok || comment.trimmed().isEmpty()) {
        if (ok) UiHelper::error(this, tr("退回失败"), tr("退回意见不能为空"));
        return;
    }
    WorkflowService::Result r = WorkflowService::approveReject(
        m_report->id(), UserSession::instance().userId(), comment);
    if (r.success) {
        m_report = ReportService::getById(m_report->id());
        m_editor->loadReport(m_report);
        if (m_report) m_loadedUpdatedAt = m_report->updatedAt();
        updateWorkflowActions();
        refreshRejectComment();
        showStatusMessage(tr("已退回，意见已记录"));
    } else {
        UiHelper::error(this, tr("退回失败"), r.errorMessage);
    }
}


void ReportEditorWindow::onWritingTools()
{
    if (!m_editor || !m_editor->textEdit()) return;
    DocumentTextEdit* te = m_editor->textEdit();

    DocumentContext ctx;
    ctx.text = te->toPlainText();
    const QTextCursor c = te->textCursor();
    ctx.cursorPos = c.position();
    ctx.selectionStart = c.selectionStart();
    ctx.selectionEnd = c.selectionEnd();
    ctx.reportTitle = m_report ? m_report->title() : QString();
    ctx.tableCount = 0;  // 表格对象数未提供时省略该指标

    WritingToolsDialog dlg(ctx, this);
    dlg.exec();
    if (dlg.insertRequested()) {
        QTextCursor cur = te->textCursor();
        const int maxPos = te->document()->characterCount() - 1;
        cur.setPosition(qBound(0, dlg.insertPos(), maxPos));
        cur.insertText(dlg.insertText());
        te->setTextCursor(cur);
        m_editor->refreshFormattingState();
        showStatusMessage(tr("已插入常用片段"), 2000);
    }
}

void ReportEditorWindow::onSaveAsTemplate()
{
    if (!m_report || !m_report->isPersisted()) {
        UiHelper::error(this, tr("保存失败"), tr("请先保存报告后再保存为模板"));
        return;
    }
    User::Ptr user = UserSession::instance().currentUser();
    if (!PermissionService::canSaveAsTemplate(user)) {
        UiHelper::error(this, tr("无权限"), tr("您没有权限保存为模板"));
        return;
    }

    // 对话框：模板名+分类+可见性
    bool ok = false;
    const QString name = QInputDialog::getText(
        this, tr("保存为模板"), tr("模板名称："), QLineEdit::Normal, m_report->title(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    const QString category = QInputDialog::getText(
        this, tr("保存为模板"), tr("模板分类（可选）："), QLineEdit::Normal, "general", &ok);
    if (!ok) return;

    // 可见性：学生/组长默认 private，总管/超管可选择
    QString visibility = "private";
    if (user->isManager() || user->isSuperAdmin()) {
        QStringList items = {tr("全局模板（所有用户可见）"), tr("私有模板（仅自己可见）")};
        bool visOk = false;
        const QString choice = QInputDialog::getItem(
            this, tr("保存为模板"), tr("模板可见性："), items, 0, false, &visOk);
        if (!visOk) return;
        visibility = (choice == items.first()) ? "public" : "private";
    }

    // 从报告复制内容创建模板
    Template::Ptr tpl = Template::create();
    tpl->setName(name.trimmed());
    tpl->setCategory(category.trimmed().isEmpty() ? "general" : category.trimmed());
    tpl->setDescription(tr("由报告「%1」保存").arg(m_report->title()));
    tpl->setDocument(m_report->document());
    tpl->setObjects(m_report->objects());
    tpl->setVisibility(visibility);
    tpl->setCreatedBy(user->id());

    if (TemplateService::save(tpl)) {
        showStatusMessage(tr("模板已保存：%1").arg(tpl->name()));
    } else {
        UiHelper::error(this, tr("保存失败"), tr("保存模板失败，请重试"));
    }
}
