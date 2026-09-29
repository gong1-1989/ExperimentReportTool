/**
 * @file ReportEditorWindow.cpp
 * @brief 报告编辑窗口实现文件
 */

#include "ReportEditorWindow.h"
#include <QStyle>
#include "ui_ReportEditorWindow.h"  // 由 uic 工具从 .ui 文件自动生成
#include "editor/ReportEditor.h"
#include "editor/DocumentTextEdit.h"
#include <QTextCursor>
#include <QTextList>
#include <QTextBlock>
#include <QPainter>
#include <QIcon>
#include <QColorDialog>
#include "export/ExportManager.h"
#include "print/PrintManager.h"
#include "data/repositories/TagRepository.h"
#include "service/ReportService.h"
#include "data/repositories/ReportRepository.h"
#include "data/repositories/DataTableRepository.h"
#include "core/plugin/PluginManager.h"
#include "core/utils/Logger.h"
#include "core/utils/AppConstants.h"
#include "core/utils/AppTheme.h"
#include "core/utils/AppDimensions.h"
#include "core/utils/AppConfig.h"
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
#include <QDateTime>
#include <QCloseEvent>
#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QComboBox>
#include <QColor>
#include <QToolButton>
#include <QColorDialog>
#include <QMenu>
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
        // 权限：非创建者且非管理员 → 只读（不能编辑，也不能保存）
        m_readOnly = (m_report->createdBy() > 0
            && m_report->createdBy() != UserSession::instance().userId()
            && !UserSession::instance().isAdmin());
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
        }
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
        // 权限检查：只有创建者或管理员可以修改已有报告
        if (m_report->createdBy() > 0
            && m_report->createdBy() != UserSession::instance().userId()
            && !UserSession::instance().isAdmin()) {
            UiHelper::warning(this, tr("权限不足"),
                tr("您没有权限修改此报告。\n只有创建者或管理员可以修改。"));
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
