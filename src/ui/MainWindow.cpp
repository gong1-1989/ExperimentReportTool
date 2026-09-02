/**
 * @file MainWindow.cpp
 * @brief 主窗口实现文件
 */

#include "MainWindow.h"
#include "ui/widgets/ProjectTreeWidget.h"
#include "ui/widgets/ReportListWidget.h"
#include "ui/ReportEditorWindow.h"
#include "ui/dialogs/ProjectDialog.h"
#include "template/TemplateEditorDialog.h"
#include "search/SearchResultDialog.h"
#include "data/repositories/ProjectRepository.h"
#include "data/repositories/ReportRepository.h"
#include "data/repositories/TemplateRepository.h"
#include "core/utils/Logger.h"
#include "core/utils/AppConstants.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QDir>
#include <QStandardPaths>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QDateTime>

// ===========================================================================
// 构造与析构
// ===========================================================================

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_mainSplitter(nullptr)
    , m_centerSplitter(nullptr)
    , m_projectTree(nullptr)
    , m_reportList(nullptr)
    , m_propertyPanel(nullptr)
    , m_editorStack(nullptr)
    , m_globalSearchEdit(nullptr)
    , m_statusProjectLabel(nullptr)
    , m_statusReportLabel(nullptr)
    , m_statusCountLabel(nullptr)
    , m_currentProjectId(-1)
    , m_currentReportId(-1)
    , m_zoomFactor(1.0)
{
    setupUi();
    createActions();
    createMenus();
    createToolBar();
    createStatusBar();
    connectSignals();
    loadSettings();
    updateWindowTitle();
    updateActionsState();

    LOG_INFO("主窗口初始化完成");
}

MainWindow::~MainWindow()
{
    saveSettings();
}

// ===========================================================================
// UI 初始化
// ===========================================================================

void MainWindow::setupUi()
{
    setWindowTitle(AppConstants::APP_DISPLAY_NAME);
    resize(AppConstants::DEFAULT_WINDOW_WIDTH, AppConstants::DEFAULT_WINDOW_HEIGHT);
    setMinimumSize(800, 600);

    // -----------------------------------------------------------------------
    // 主分割器：左（项目树） | 中（报告列表+编辑器） | 右（属性面板）
    // -----------------------------------------------------------------------
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->setHandleWidth(4);
    m_mainSplitter->setChildrenCollapsible(true);

    // 左侧：项目树
    m_projectTree = new ProjectTreeWidget(this);
    m_projectTree->setMinimumWidth(200);
    m_mainSplitter->addWidget(m_projectTree);

    // 中间分割器：上（报告列表） | 下（编辑器占位）
    m_centerSplitter = new QSplitter(Qt::Vertical, m_mainSplitter);
    m_centerSplitter->setHandleWidth(4);

    // 报告列表
    m_reportList = new ReportListWidget(this);
    m_centerSplitter->addWidget(m_reportList);

    // 编辑器区域（占位，后续实现完整编辑器）
    m_editorStack = new QStackedWidget(this);
    QLabel* editorPlaceholder = new QLabel(tr(
        "<div style='color: #999; font-size: 16px; text-align: center;'>"
        "<p>📝 报告编辑器</p>"
        "<p style='font-size: 13px;'>双击左侧报告列表中的报告以打开编辑</p>"
        "<p style='font-size: 12px; color: #bbb;'>（编辑器将在后续版本中实现）</p>"
        "</div>"), this);
    editorPlaceholder->setAlignment(Qt::AlignCenter);
    m_editorStack->addWidget(editorPlaceholder);
    m_centerSplitter->addWidget(m_editorStack);

    // 设置中间分割器比例
    m_centerSplitter->setStretchFactor(0, 1);
    m_centerSplitter->setStretchFactor(1, 2);
    m_centerSplitter->setSizes({300, 500});

    m_mainSplitter->addWidget(m_centerSplitter);

    // 右侧：属性面板（占位）
    m_propertyPanel = new QWidget(this);
    m_propertyPanel->setMinimumWidth(200);
    QVBoxLayout* propLayout = new QVBoxLayout(m_propertyPanel);
    propLayout->setContentsMargins(12, 12, 12, 12);
    QLabel* propTitle = new QLabel(tr("属性面板"), m_propertyPanel);
    propTitle->setStyleSheet("font-weight: bold; font-size: 14px; padding-bottom: 8px; border-bottom: 1px solid #ddd;");
    propLayout->addWidget(propTitle);
    QLabel* propPlaceholder = new QLabel(tr(
        "<div style='color: #999; font-size: 12px; margin-top: 12px;'>"
        "选择项目或报告后，<br>此处将显示其属性信息。<br><br>"
        "（属性面板将在后续版本中实现）"
        "</div>"), m_propertyPanel);
    propPlaceholder->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    propLayout->addWidget(propPlaceholder);
    propLayout->addStretch();
    m_mainSplitter->addWidget(m_propertyPanel);

    // 设置主分割器比例
    m_mainSplitter->setStretchFactor(0, 1);  // 项目树
    m_mainSplitter->setStretchFactor(1, 3);  // 中间区域
    m_mainSplitter->setStretchFactor(2, 1);  // 属性面板
    m_mainSplitter->setSizes({250, 600, 250});

    setCentralWidget(m_mainSplitter);
}

void MainWindow::createActions()
{
    // -----------------------------------------------------------------------
    // 文件菜单动作
    // -----------------------------------------------------------------------
    m_actionNewProject = new QAction(tr("新建项目(&N)..."), this);
    m_actionNewProject->setShortcut(QKeySequence("Ctrl+Shift+N"));
    m_actionNewProject->setStatusTip(tr("创建新的实验项目"));

    m_actionNewReport = new QAction(tr("新建报告(&R)..."), this);
    m_actionNewReport->setShortcut(QKeySequence("Ctrl+N"));
    m_actionNewReport->setStatusTip(tr("创建新的实验报告"));

    m_actionOpenReport = new QAction(tr("打开报告(&O)..."), this);
    m_actionOpenReport->setShortcut(QKeySequence("Ctrl+O"));
    m_actionOpenReport->setStatusTip(tr("打开已有报告"));

    m_actionSaveReport = new QAction(tr("保存报告(&S)"), this);
    m_actionSaveReport->setShortcut(QKeySequence("Ctrl+S"));
    m_actionSaveReport->setStatusTip(tr("保存当前报告"));
    m_actionSaveReport->setEnabled(false);

    m_actionExportReport = new QAction(tr("导出报告(&E)..."), this);
    m_actionExportReport->setShortcut(QKeySequence("Ctrl+E"));
    m_actionExportReport->setStatusTip(tr("导出报告为 PDF/Word 等格式"));
    m_actionExportReport->setEnabled(false);

    m_actionExit = new QAction(tr("退出(&X)"), this);
    m_actionExit->setShortcut(QKeySequence("Ctrl+Q"));
    m_actionExit->setStatusTip(tr("退出应用程序"));

    // -----------------------------------------------------------------------
    // 编辑菜单动作
    // -----------------------------------------------------------------------
    m_actionEditProject = new QAction(tr("编辑项目(&P)..."), this);
    m_actionEditProject->setShortcut(QKeySequence("F2"));
    m_actionEditProject->setStatusTip(tr("编辑当前选中的项目"));
    m_actionEditProject->setEnabled(false);

    m_actionDeleteProject = new QAction(tr("删除项目(&D)"), this);
    m_actionDeleteProject->setShortcut(QKeySequence("Ctrl+Delete"));
    m_actionDeleteProject->setStatusTip(tr("删除当前选中的项目"));
    m_actionDeleteProject->setEnabled(false);

    m_actionDeleteReport = new QAction(tr("删除报告"), this);
    m_actionDeleteReport->setStatusTip(tr("删除当前选中的报告"));
    m_actionDeleteReport->setEnabled(false);

    m_actionFind = new QAction(tr("查找(&F)..."), this);
    m_actionFind->setShortcut(QKeySequence("Ctrl+F"));
    m_actionFind->setStatusTip(tr("全文搜索报告"));

    // -----------------------------------------------------------------------
    // 视图菜单动作
    // -----------------------------------------------------------------------
    m_actionToggleProjectPanel = new QAction(tr("项目面板"), this);
    m_actionToggleProjectPanel->setCheckable(true);
    m_actionToggleProjectPanel->setChecked(true);
    m_actionToggleProjectPanel->setStatusTip(tr("显示/隐藏项目面板"));

    m_actionTogglePropertyPanel = new QAction(tr("属性面板"), this);
    m_actionTogglePropertyPanel->setCheckable(true);
    m_actionTogglePropertyPanel->setChecked(true);
    m_actionTogglePropertyPanel->setStatusTip(tr("显示/隐藏属性面板"));

    m_actionFullscreen = new QAction(tr("全屏模式"), this);
    m_actionFullscreen->setShortcut(QKeySequence("F11"));
    m_actionFullscreen->setCheckable(true);
    m_actionFullscreen->setStatusTip(tr("切换全屏模式"));

    m_actionZoomIn = new QAction(tr("放大"), this);
    m_actionZoomIn->setShortcut(QKeySequence("Ctrl++"));
    m_actionZoomIn->setStatusTip(tr("放大界面"));

    m_actionZoomOut = new QAction(tr("缩小"), this);
    m_actionZoomOut->setShortcut(QKeySequence("Ctrl+-"));
    m_actionZoomOut->setStatusTip(tr("缩小界面"));

    m_actionResetZoom = new QAction(tr("重置缩放"), this);
    m_actionResetZoom->setShortcut(QKeySequence("Ctrl+0"));
    m_actionResetZoom->setStatusTip(tr("重置缩放为 100%"));

    // -----------------------------------------------------------------------
    // 工具菜单动作
    // -----------------------------------------------------------------------
    m_actionTemplateManager = new QAction(tr("模板管理器(&T)..."), this);
    m_actionTemplateManager->setStatusTip(tr("管理报告模板"));

    m_actionBackup = new QAction(tr("数据备份(&B)..."), this);
    m_actionBackup->setStatusTip(tr("备份所有数据到文件"));

    m_actionRestore = new QAction(tr("数据恢复(&R)..."), this);
    m_actionRestore->setStatusTip(tr("从备份文件恢复数据"));

    m_actionSettings = new QAction(tr("设置(&S)..."), this);
    m_actionSettings->setShortcut(QKeySequence("Ctrl+,"));
    m_actionSettings->setStatusTip(tr("应用程序设置"));

    // -----------------------------------------------------------------------
    // 帮助菜单动作
    // -----------------------------------------------------------------------
    m_actionAbout = new QAction(tr("关于(&A)..."), this);
    m_actionAbout->setStatusTip(tr("关于本软件"));

    m_actionAboutQt = new QAction(tr("关于 Qt"), this);
    m_actionAboutQt->setStatusTip(tr("关于 Qt 框架"));

    m_actionCheckUpdate = new QAction(tr("检查更新"), this);
    m_actionCheckUpdate->setStatusTip(tr("检查软件更新"));
}

void MainWindow::createMenus()
{
    QMenuBar* menuBar = this->menuBar();

    // 文件菜单
    QMenu* fileMenu = menuBar->addMenu(tr("文件(&F)"));
    fileMenu->addAction(m_actionNewProject);
    fileMenu->addAction(m_actionNewReport);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actionOpenReport);
    fileMenu->addAction(m_actionSaveReport);
    fileMenu->addAction(m_actionExportReport);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actionExit);

    // 编辑菜单
    QMenu* editMenu = menuBar->addMenu(tr("编辑(&E)"));
    editMenu->addAction(m_actionEditProject);
    editMenu->addAction(m_actionDeleteProject);
    editMenu->addSeparator();
    editMenu->addAction(m_actionDeleteReport);
    editMenu->addSeparator();
    editMenu->addAction(m_actionFind);

    // 视图菜单
    QMenu* viewMenu = menuBar->addMenu(tr("视图(&V)"));
    viewMenu->addAction(m_actionToggleProjectPanel);
    viewMenu->addAction(m_actionTogglePropertyPanel);
    viewMenu->addSeparator();
    viewMenu->addAction(m_actionFullscreen);
    viewMenu->addSeparator();
    viewMenu->addAction(m_actionZoomIn);
    viewMenu->addAction(m_actionZoomOut);
    viewMenu->addAction(m_actionResetZoom);

    // 工具菜单
    QMenu* toolsMenu = menuBar->addMenu(tr("工具(&T)"));
    toolsMenu->addAction(m_actionTemplateManager);
    toolsMenu->addSeparator();
    toolsMenu->addAction(m_actionBackup);
    toolsMenu->addAction(m_actionRestore);
    toolsMenu->addSeparator();
    toolsMenu->addAction(m_actionSettings);

    // 帮助菜单
    QMenu* helpMenu = menuBar->addMenu(tr("帮助(&H)"));
    helpMenu->addAction(m_actionAbout);
    helpMenu->addAction(m_actionAboutQt);
    helpMenu->addSeparator();
    helpMenu->addAction(m_actionCheckUpdate);
}

void MainWindow::createToolBar()
{
    QToolBar* toolBar = addToolBar(tr("主工具栏"));
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(20, 20));
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    // 新建项目
    toolBar->addAction(m_actionNewProject);
    // 新建报告
    toolBar->addAction(m_actionNewReport);
    toolBar->addSeparator();

    // 保存
    toolBar->addAction(m_actionSaveReport);
    // 导出
    toolBar->addAction(m_actionExportReport);
    toolBar->addSeparator();

    // 全局搜索框
    m_globalSearchEdit = new QLineEdit(toolBar);
    m_globalSearchEdit->setPlaceholderText(tr("🔍 全局搜索报告..."));
    m_globalSearchEdit->setClearButtonEnabled(true);
    m_globalSearchEdit->setMaximumWidth(280);
    toolBar->addWidget(m_globalSearchEdit);

    toolBar->addSeparator();
    toolBar->addAction(m_actionSettings);
}

void MainWindow::createStatusBar()
{
    QStatusBar* statusBar = this->statusBar();

    m_statusProjectLabel = new QLabel(tr("项目: 全部"), this);
    m_statusProjectLabel->setStyleSheet("padding: 0 8px;");
    statusBar->addWidget(m_statusProjectLabel);

    m_statusReportLabel = new QLabel(tr("报告: 无"), this);
    m_statusReportLabel->setStyleSheet("padding: 0 8px;");
    statusBar->addWidget(m_statusReportLabel);

    //statusBar->addStretch();
    QWidget* stretchWidget = new QWidget(statusBar);
    stretchWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    stretchWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    statusBar->addWidget(stretchWidget);


    m_statusCountLabel = new QLabel(this);
    m_statusCountLabel->setStyleSheet("padding: 0 8px; color: #666;");
    statusBar->addPermanentWidget(m_statusCountLabel);

    updateStatusBar();
}

void MainWindow::connectSignals()
{
    // 文件菜单
    connect(m_actionNewProject, &QAction::triggered, this, &MainWindow::onNewProject);
    connect(m_actionNewReport, &QAction::triggered, this, &MainWindow::onNewReport);
    connect(m_actionOpenReport, &QAction::triggered, this, &MainWindow::onOpenReport);
    connect(m_actionSaveReport, &QAction::triggered, this, &MainWindow::onSaveReport);
    connect(m_actionExportReport, &QAction::triggered, this, &MainWindow::onExportReport);
    connect(m_actionExit, &QAction::triggered, this, &MainWindow::close);

    // 编辑菜单
    connect(m_actionEditProject, &QAction::triggered, this, &MainWindow::onEditProject);
    connect(m_actionDeleteProject, &QAction::triggered, this, &MainWindow::onDeleteProject);
    connect(m_actionDeleteReport, &QAction::triggered, this, &MainWindow::onDeleteReport);
    connect(m_actionFind, &QAction::triggered, this, &MainWindow::onFind);

    // 视图菜单
    connect(m_actionToggleProjectPanel, &QAction::toggled, this, &MainWindow::onToggleProjectPanel);
    connect(m_actionTogglePropertyPanel, &QAction::toggled, this, &MainWindow::onTogglePropertyPanel);
    connect(m_actionFullscreen, &QAction::toggled, this, &MainWindow::onToggleFullscreen);
    connect(m_actionZoomIn, &QAction::triggered, this, &MainWindow::onZoomIn);
    connect(m_actionZoomOut, &QAction::triggered, this, &MainWindow::onZoomOut);
    connect(m_actionResetZoom, &QAction::triggered, this, &MainWindow::onResetZoom);

    // 工具菜单
    connect(m_actionTemplateManager, &QAction::triggered, this, &MainWindow::onTemplateManager);
    connect(m_actionBackup, &QAction::triggered, this, &MainWindow::onDataBackup);
    connect(m_actionRestore, &QAction::triggered, this, &MainWindow::onDataRestore);
    connect(m_actionSettings, &QAction::triggered, this, &MainWindow::onSettings);

    // 帮助菜单
    connect(m_actionAbout, &QAction::triggered, this, &MainWindow::onAbout);
    connect(m_actionAboutQt, &QAction::triggered, this, &MainWindow::onAboutQt);
    connect(m_actionCheckUpdate, &QAction::triggered, this, &MainWindow::onCheckUpdate);

    // 项目树
    connect(m_projectTree, &ProjectTreeWidget::projectSelected,
            this, &MainWindow::onProjectSelected);
    connect(m_projectTree, &ProjectTreeWidget::projectTreeChanged,
            this, &MainWindow::onProjectTreeChanged);

    // 报告列表
    connect(m_reportList, &ReportListWidget::reportOpenRequested,
            this, &MainWindow::onReportOpenRequested);
    connect(m_reportList, &ReportListWidget::reportNewRequested,
            this, &MainWindow::onReportNewRequested);
    connect(m_reportList, &ReportListWidget::reportEditRequested,
            this, &MainWindow::onReportEditRequested);
    connect(m_reportList, &ReportListWidget::reportDeleteRequested,
            this, &MainWindow::onReportDeleteRequested);
    connect(m_reportList, &ReportListWidget::reportListChanged,
            this, &MainWindow::onReportListChanged);

    // 全局搜索
    connect(m_globalSearchEdit, &QLineEdit::returnPressed,
            this, &MainWindow::onGlobalSearch);
    connect(m_globalSearchEdit, &QLineEdit::textChanged,
            this, &MainWindow::onGlobalSearchTextChanged);
}

// ===========================================================================
// 文件菜单槽函数
// ===========================================================================

void MainWindow::onNewProject()
{
    ProjectDialog dialog(this);
    dialog.setWindowTitle(tr("新建项目"));

    if (dialog.exec() == QDialog::Accepted) {
        Project::Ptr project = dialog.projectData();
        if (ProjectRepository::insert(project)) {
            m_projectTree->refreshTree();
            m_projectTree->selectProject(project->id());
            showStatusMessage(tr("项目「%1」已创建").arg(project->name()));
            LOG_INFO(QString("项目已创建: %1").arg(project->toString()));
        } else {
            QMessageBox::critical(this, tr("错误"), tr("创建项目失败，请查看日志"));
        }
    }
}

void MainWindow::onNewReport()
{
    const qint64 projectId = currentProjectId();
    if (projectId <= 0) {
        QMessageBox::information(this, tr("提示"), tr("请先在左侧选择一个项目"));
        return;
    }

    // 选择模板
    const Template::List templates = TemplateRepository::findAll();
    if (templates.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("没有可用的报告模板"));
        return;
    }

    QStringList templateNames;
    for (const Template::Ptr& t : templates) {
        templateNames.append(t->name());
    }

    bool ok = false;
    const QString selected = QInputDialog::getItem(
        this, tr("选择模板"), tr("请选择报告模板:"),
        templateNames, 0, false, &ok);

    if (!ok || selected.isEmpty()) return;

    // 找到选中的模板
    Template::Ptr selectedTemplate;
    for (const Template::Ptr& t : templates) {
        if (t->name() == selected) {
            selectedTemplate = t;
            break;
        }
    }

    if (!selectedTemplate) return;

    // 输入报告标题
    bool titleOk = false;
    const QString title = QInputDialog::getText(
        this, tr("新建报告"), tr("请输入报告标题:"),
        QLineEdit::Normal, tr("未命名实验报告"), &titleOk);

    if (!titleOk || title.trimmed().isEmpty()) return;

    // 创建报告
    Report::Ptr report = Report::create();
    report->setProjectId(projectId);
    report->setTemplateId(selectedTemplate->id());
    report->setTitle(title.trimmed());
    report->setExperimentDate(QDate::currentDate());

    // 从模板复制内容块
    for (const ContentBlock& block : selectedTemplate->blocks()) {
        report->appendBlock(block);
    }

    if (ReportRepository::insert(report)) {
        m_reportList->refreshList();
        showStatusMessage(tr("报告「%1」已创建").arg(report->title()));
        LOG_INFO(QString("报告已创建: %1").arg(report->toString()));
    } else {
        QMessageBox::critical(this, tr("错误"), tr("创建报告失败"));
    }
}

void MainWindow::onOpenReport()
{
    const qint64 reportId = currentReportId();
    if (reportId > 0) {
        onReportOpenRequested(reportId);
    } else {
        QMessageBox::information(this, tr("提示"), tr("请先在报告列表中选择一份报告"));
    }
}

void MainWindow::onSaveReport()
{
    // 编辑器实现后，这里保存当前编辑的报告
    showStatusMessage(tr("保存功能将在编辑器实现后启用"));
}

void MainWindow::onExportReport()
{
    const qint64 reportId = currentReportId();
    if (reportId <= 0) {
        QMessageBox::information(this, tr("提示"), tr("请先选择要导出的报告"));
        return;
    }

    // 导出功能将在后续版本实现
    QMessageBox::information(this, tr("导出"),
        tr("报告导出功能（PDF/Word/HTML）将在后续版本中实现。\n\n"
           "当前版本已完成数据层和基础 UI 框架。"));
}

void MainWindow::onExit()
{
    close();
}

// ===========================================================================
// 编辑菜单槽函数
// ===========================================================================

void MainWindow::onEditProject()
{
    const qint64 projectId = currentProjectId();
    if (projectId <= 0) return;

    Project::Ptr project = ProjectRepository::findById(projectId);
    if (!project) return;

    ProjectDialog dialog(this);
    dialog.setWindowTitle(tr("编辑项目"));
    dialog.setProjectData(project);

    if (dialog.exec() == QDialog::Accepted) {
        Project::Ptr updated = dialog.projectData();
        updated->setId(projectId);
        if (ProjectRepository::update(updated)) {
            m_projectTree->refreshTree();
            m_projectTree->selectProject(projectId);
            showStatusMessage(tr("项目已更新"));
        } else {
            QMessageBox::critical(this, tr("错误"), tr("更新项目失败"));
        }
    }
}

void MainWindow::onDeleteProject()
{
    const qint64 projectId = currentProjectId();
    if (projectId <= 0) return;

    Project::Ptr project = ProjectRepository::findById(projectId);
    if (!project) return;

    const auto ret = QMessageBox::warning(
        this, tr("确认删除"),
        tr("确定要删除项目「%1」吗？\n该项目下的所有报告将被同时删除，此操作不可恢复！")
            .arg(project->name()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        if (ProjectRepository::remove(projectId)) {
            m_projectTree->refreshTree();
            m_reportList->setProjectId(-1);
            showStatusMessage(tr("项目已删除"));
        } else {
            QMessageBox::critical(this, tr("错误"), tr("删除项目失败"));
        }
    }
}

void MainWindow::onDeleteReport()
{
    const qint64 reportId = currentReportId();
    if (reportId <= 0) return;

    Report::Ptr report = ReportRepository::findById(reportId);
    if (!report) return;

    const auto ret = QMessageBox::warning(
        this, tr("确认删除"),
        tr("确定要删除报告「%1」吗？此操作不可恢复！").arg(report->title()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        if (ReportRepository::remove(reportId)) {
            m_reportList->refreshList();
            showStatusMessage(tr("报告已删除"));
        } else {
            QMessageBox::critical(this, tr("错误"), tr("删除报告失败"));
        }
    }
}

void MainWindow::onFind()
{
    if (m_globalSearchEdit) {
        m_globalSearchEdit->setFocus();
        m_globalSearchEdit->selectAll();
    }
}

// ===========================================================================
// 视图菜单槽函数
// ===========================================================================

void MainWindow::onToggleProjectPanel(bool visible)
{
    if (m_projectTree) {
        m_projectTree->setVisible(visible);
    }
}

void MainWindow::onTogglePropertyPanel(bool visible)
{
    if (m_propertyPanel) {
        m_propertyPanel->setVisible(visible);
    }
}

void MainWindow::onToggleFullscreen()
{
    if (isFullScreen()) {
        showNormal();
        m_actionFullscreen->setChecked(false);
    } else {
        showFullScreen();
        m_actionFullscreen->setChecked(true);
    }
}

void MainWindow::onZoomIn()
{
    m_zoomFactor = qMin(2.0, m_zoomFactor + 0.1);
    // 编辑器实现后应用缩放到编辑器
    showStatusMessage(tr("缩放: %1%").arg(qRound(m_zoomFactor * 100)));
}

void MainWindow::onZoomOut()
{
    m_zoomFactor = qMax(0.5, m_zoomFactor - 0.1);
    showStatusMessage(tr("缩放: %1%").arg(qRound(m_zoomFactor * 100)));
}

void MainWindow::onResetZoom()
{
    m_zoomFactor = 1.0;
    showStatusMessage(tr("缩放已重置为 100%"));
}

// ===========================================================================
// 工具菜单槽函数
// ===========================================================================

void MainWindow::onTemplateManager()
{
    // 获取所有模板
    const Template::List templates = TemplateRepository::findAll();

    // 构建模板列表供用户选择
    QStringList items;
    items.append(tr("--- 新建模板 ---"));
    for (const Template::Ptr& t : templates) {
        const QString builtinMark = t->isBuiltin() ? tr(" [内置]") : "";
        items.append(QString("%1 (%2)%3").arg(t->name(), t->category(), builtinMark));
    }

    bool ok = false;
    const QString selected = QInputDialog::getItem(
        this, tr("模板管理器"), tr("选择要编辑的模板，或新建模板:"),
        items, 0, false, &ok);

    if (!ok || selected.isEmpty()) return;

    if (selected == items.first()) {
        // 新建模板
        TemplateEditorDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            showStatusMessage(tr("模板「%1」已创建").arg(dialog.templateData()->name()));
        }
    } else {
        // 编辑现有模板
        const int idx = items.indexOf(selected) - 1;  // 减 1 因为第一项是"新建"
        if (idx >= 0 && idx < templates.size()) {
            Template::Ptr temp = templates.at(idx);

            // 内置模板需要先复制才能编辑
            if (temp->isBuiltin()) {
                const auto ret = QMessageBox::question(
                    this, tr("内置模板"),
                    tr("「%1」是内置模板，不能直接修改。\n是否创建一个副本进行编辑？")
                        .arg(temp->name()),
                    QMessageBox::Yes | QMessageBox::No);
                if (ret != QMessageBox::Yes) return;

                // 创建副本
                Template::Ptr copy = Template::create();
                copy->setName(temp->name() + tr(" (副本)"));
                copy->setCategory(temp->category());
                copy->setDescription(temp->description());
                copy->setBlocks(temp->blocks());
                TemplateRepository::insert(copy);
                temp = copy;
            }

            TemplateEditorDialog dialog(this, temp);
            if (dialog.exec() == QDialog::Accepted) {
                showStatusMessage(tr("模板「%1」已更新").arg(dialog.templateData()->name()));
            }
        }
    }
}

void MainWindow::onDataBackup()
{
    const QString filePath = QFileDialog::getSaveFileName(
        this, tr("备份数据"),
        QDir::homePath() + "/experiment_report_backup_" +
            QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".db",
        tr("数据库文件 (*.db);;所有文件 (*)"));

    if (filePath.isEmpty()) return;

    // 简单的文件复制备份
    const QString dbPath = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation) + "/experiment_reports.db";

    if (QFile::copy(dbPath, filePath)) {
        QMessageBox::information(this, tr("备份成功"),
            tr("数据已备份到:\n%1").arg(filePath));
        showStatusMessage(tr("数据备份完成"));
    } else {
        QMessageBox::critical(this, tr("备份失败"),
            tr("无法复制数据库文件。\n请确保目标路径可写。"));
    }
}

void MainWindow::onDataRestore()
{
    QMessageBox::warning(this, tr("数据恢复"),
        tr("数据恢复功能将覆盖当前所有数据！\n\n"
           "此功能将在后续版本中实现，当前请手动替换数据库文件。"));
}

void MainWindow::onSettings()
{
    QMessageBox::information(this, tr("设置"),
        tr("设置面板将在后续版本中实现。\n\n"
           "计划支持：\n"
           "- 主题切换（浅色/深色）\n"
           "- 字体大小调整\n"
           "- 自动保存设置\n"
           "- 默认导出格式\n"
           "- 数据存储路径"));
}

// ===========================================================================
// 帮助菜单槽函数
// ===========================================================================

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("关于 %1").arg(AppConstants::APP_DISPLAY_NAME),
        QString(
            "<h3>%1</h3>"
            "<p>版本: %2</p>"
            "<p>基于 Qt %3 + C++ 开发的实验报告记录工具</p>"
            "<p>功能特性：</p>"
            "<ul>"
            "<li>项目树状管理</li>"
            "<li>模板化报告创建</li>"
            "<li>结构化富文本编辑</li>"
            "<li>实验数据表格与图表</li>"
            "<li>多格式导出（PDF/Word/HTML）</li>"
            "<li>全文检索</li>"
            "</ul>"
            "<p style='color: #888; font-size: 11px;'>%4</p>"
        ).arg(AppConstants::APP_DISPLAY_NAME)
         .arg(AppConstants::APP_VERSION)
         .arg(qVersion())
         .arg(tr("© 2024 实验报告记录工具开发组")));
}

void MainWindow::onAboutQt()
{
    QMessageBox::aboutQt(this, tr("关于 Qt"));
}

void MainWindow::onCheckUpdate()
{
    QMessageBox::information(this, tr("检查更新"),
        tr("当前已是最新版本: %1\n\n"
           "更新检查功能将在后续版本中实现。").arg(AppConstants::APP_VERSION));
}

// ===========================================================================
// 项目树信号槽
// ===========================================================================

void MainWindow::onProjectSelected(qint64 projectId)
{
    m_currentProjectId = projectId;
    m_reportList->setProjectId(projectId);
    updateWindowTitle();
    updateActionsState();
    updateStatusBar();
}

void MainWindow::onProjectTreeChanged()
{
    updateStatusBar();
}

// ===========================================================================
// 报告列表信号槽
// ===========================================================================

void MainWindow::onReportOpenRequested(qint64 reportId)
{
    m_currentReportId = reportId;
    Report::Ptr report = ReportRepository::findById(reportId);
    if (!report) {
        QMessageBox::warning(this, tr("错误"), tr("未找到报告"));
        return;
    }

    // 检查是否已经打开了该报告的编辑器窗口
    for (ReportEditorWindow* win : m_editorWindows) {
        if (win->currentReport() && win->currentReport()->id() == reportId) {
            win->raise();
            win->activateWindow();
            return;
        }
    }

    // 创建新的报告编辑器窗口
    ReportEditorWindow* editorWindow = new ReportEditorWindow(report, this);
    editorWindow->setAttribute(Qt::WA_DeleteOnClose);
    connect(editorWindow,&ReportEditorWindow::windowClosed,this,[this,editorWindow](){
        m_editorWindows.removeOne(editorWindow);});
    connect(editorWindow, &ReportEditorWindow::reportSaved,
            this, &MainWindow::onReportEditorSaved);
    connect(editorWindow, &ReportEditorWindow::windowClosed,
            this, &MainWindow::onReportEditorClosed);

    m_editorWindows.append(editorWindow);
    editorWindow->show();

    showStatusMessage(tr("已打开报告: %1").arg(report->title()));
    m_statusReportLabel->setText(tr("报告: %1").arg(report->title()));
    m_actionSaveReport->setEnabled(true);
    m_actionExportReport->setEnabled(true);
    updateActionsState();
}

void MainWindow::onReportNewRequested(qint64 projectId)
{
    Q_UNUSED(projectId);
    onNewReport();
}

void MainWindow::onReportEditRequested(qint64 reportId)
{
    onReportOpenRequested(reportId);
}

void MainWindow::onReportDeleteRequested(qint64 reportId)
{
    Q_UNUSED(reportId);
    onDeleteReport();
}

void MainWindow::onReportListChanged()
{
    updateStatusBar();
}

// ===========================================================================
// 报告编辑器窗口信号槽
// ===========================================================================

void MainWindow::onReportEditorSaved(qint64 reportId)
{
    Q_UNUSED(reportId);
    // 报告保存后刷新列表
    m_reportList->refreshList();
    updateStatusBar();
    showStatusMessage(tr("报告已保存"));
}

void MainWindow::onReportEditorClosed(qint64 reportId)
{
    Q_UNUSED(reportId);

    m_reportList->refreshList();
    updateStatusBar();
}

// ===========================================================================
// 全局搜索
// ===========================================================================

void MainWindow::onGlobalSearch()
{
    const QString keyword = m_globalSearchEdit->text().trimmed();

    // 打开搜索结果对话框
    SearchResultDialog dialog(this, keyword);
    connect(&dialog, &SearchResultDialog::reportOpenRequested,
            this, &MainWindow::onReportOpenRequested);

    dialog.exec();
}

void MainWindow::onGlobalSearchTextChanged(const QString& text)
{
    // 实时搜索（防抖可在后续优化）
    if (text.isEmpty()) {
        m_reportList->refreshList();
    }
}

// ===========================================================================
// 辅助方法
// ===========================================================================

void MainWindow::showStatusMessage(const QString& message, int timeout)
{
    statusBar()->showMessage(message, timeout);
}

void MainWindow::refreshAll()
{
    m_projectTree->refreshTree();
    m_reportList->refreshList();
    updateStatusBar();
}

qint64 MainWindow::currentProjectId() const
{
    return m_projectTree ? m_projectTree->currentProjectId() : -1;
}

qint64 MainWindow::currentReportId() const
{
    return m_reportList ? m_reportList->currentReportId() : -1;
}

void MainWindow::updateWindowTitle()
{
    QString title = AppConstants::APP_DISPLAY_NAME;

    if (m_currentProjectId > 0) {
        Project::Ptr project = ProjectRepository::findById(m_currentProjectId);
        if (project) {
            title += QString(" - %1").arg(project->name());
        }
    } else {
        title += tr(" - 全部项目");
    }

    setWindowTitle(title);
}

void MainWindow::updateActionsState()
{
    const bool hasProject = currentProjectId() > 0;
    const bool hasReport = currentReportId() > 0;

    m_actionNewReport->setEnabled(hasProject);
    m_actionEditProject->setEnabled(hasProject);
    m_actionDeleteProject->setEnabled(hasProject);
    m_actionDeleteReport->setEnabled(hasReport);
    m_actionOpenReport->setEnabled(hasReport);
}

void MainWindow::updateStatusBar()
{
    // 当前项目
    if (m_currentProjectId > 0) {
        Project::Ptr project = ProjectRepository::findById(m_currentProjectId);
        if (project) {
            m_statusProjectLabel->setText(tr("项目: %1").arg(project->name()));
        }
    } else {
        m_statusProjectLabel->setText(tr("项目: 全部"));
    }

    // 统计信息
    const int projectCount = ProjectRepository::count();
    const int reportCount = ReportRepository::count();
    m_statusCountLabel->setText(
        tr("项目: %1 | 报告: %2").arg(projectCount).arg(reportCount));
}

// ===========================================================================
// 设置保存与加载
// ===========================================================================

void MainWindow::loadSettings()
{
    QSettings settings;

    // 恢复窗口几何信息
    if (settings.contains(AppConstants::SettingsKeys::MAIN_WINDOW_GEOMETRY)) {
        restoreGeometry(settings.value(
            AppConstants::SettingsKeys::MAIN_WINDOW_GEOMETRY).toByteArray());
    }
    if (settings.contains(AppConstants::SettingsKeys::MAIN_WINDOW_STATE)) {
        restoreState(settings.value(
            AppConstants::SettingsKeys::MAIN_WINDOW_STATE).toByteArray());
    }

    // 恢复分割器状态
    // 注意：QSplitter 的 saveState/restoreState 需要在 setupUi 之后调用
}

void MainWindow::saveSettings()
{
    QSettings settings;

    settings.setValue(AppConstants::SettingsKeys::MAIN_WINDOW_GEOMETRY, saveGeometry());
    settings.setValue(AppConstants::SettingsKeys::MAIN_WINDOW_STATE, saveState());
}

// ===========================================================================
// 事件处理
// ===========================================================================

void MainWindow::closeEvent(QCloseEvent* event)
{
    // 检查是否有未保存的更改（编辑器实现后需要检查）
    // 当前版本直接保存设置并关闭

    saveSettings();
    LOG_INFO("主窗口关闭");
    event->accept();
}
