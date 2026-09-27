/**
 * @file MainWindow.cpp
 * @brief 主窗口实现文件
 */

#include "MainWindow.h"
#include "ui_MainWindow.h"  // 由 uic 工具从 .ui 文件自动生成
#include "ui/widgets/ProjectTreeWidget.h"
#include "ui/widgets/ReportListWidget.h"
#include "ui/ReportEditorWindow.h"
#include "ui/dialogs/ProjectDialog.h"
#include "ui/dialogs/SettingsDialog.h"
#include "ui/dialogs/SearchResultDialog.h"
#include "ui/dialogs/TemplateEditorDialog.h"
#include "ui/dialogs/TagManagerDialog.h"
#include "ui/dialogs/ChangePasswordDialog.h"
#include "ui/dialogs/PluginManagerDialog.h"
#include "ui/dialogs/UserManagerDialog.h"
#include "core/plugin/PluginManager.h"
#include "core/plugin/ImportPluginInterface.h"
#include "data/repositories/ProjectRepository.h"
#include "data/repositories/ReportRepository.h"
#include "data/repositories/TemplateRepository.h"
#include "data/repositories/TagRepository.h"
#include "data/repositories/DataTableRepository.h"
#include "core/models/Tag.h"
#include "core/models/DataTable.h"
#include "export/ExportManager.h"
#include "core/utils/Logger.h"
#include "core/utils/AppConstants.h"
#include "core/utils/AppTheme.h"
#include "core/utils/AppDimensions.h"
#include "core/utils/AppConfig.h"
#include "core/utils/UserSession.h"
#include "data/repositories/UserRepository.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QFile>
#include <QStandardPaths>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QDateTime>
#include <QSplitter>
#include <QStackedWidget>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>

// ===========================================================================
// 构造与析构
// ===========================================================================

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)  // 创建 UI 界面对象
    , m_mainSplitter(nullptr)
    , m_projectTree(nullptr)
    , m_reportList(nullptr)
    , m_propertyPanel(nullptr)
    , m_propertyContentLabel(nullptr)
    , m_globalSearchEdit(nullptr)
    , m_statusProjectLabel(nullptr)
    , m_statusReportLabel(nullptr)
    , m_statusCountLabel(nullptr)
    , m_currentProjectId(-1)
    , m_currentReportId(-1)
    , m_zoomFactor(1.0)
{
    ui->setupUi(this);  // 从 .ui 文件加载基本界面结构

    // 动态创建复杂控件（自定义组件、工具栏、状态栏等）
    setupUi();

    createActions();
    createMenus();
    createToolBar();
    createStatusBar();
    connectSignals();
    loadSettings();
    updateWindowTitle();
    updateActionsState();

    // 根据用户角色显示/隐藏用户管理菜单
    m_actionUserManager->setVisible(UserSession::instance().isAdmin());

    LOG_INFO("主窗口初始化完成");
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}

// ===========================================================================
// UI 初始化（动态创建复杂控件）
// ===========================================================================

void MainWindow::setupUi()
{
    // 设置窗口基本属性
    setWindowTitle(AppConstants::APP_DISPLAY_NAME);
    // 从配置文件读取窗口大小
    resize(AppConfig::instance().mainWindowWidth(),
           AppConfig::instance().mainWindowHeight());
    setMinimumSize(800, 600);

    // -----------------------------------------------------------------------
    // 主分割器（左-中-右三栏布局）
    // -----------------------------------------------------------------------
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->setHandleWidth(AppTheme::Spacing::Medium);
    m_mainSplitter->setChildrenCollapsible(false);

    // 左侧：项目树
    QWidget* leftPanel = new QWidget(this);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(AppTheme::Spacing::Small, AppTheme::Spacing::Small,
                                   AppTheme::Spacing::Small, AppTheme::Spacing::Small);
    leftLayout->setSpacing(AppTheme::Spacing::Small);

    QLabel* projectTitle = new QLabel(tr("📁 项目树"), leftPanel);
    projectTitle->setStyleSheet(
        QString("font-weight: bold; padding: %1px; color: %2;")
            .arg(AppTheme::Spacing::Small).arg(AppTheme::Color::Gray333));
    leftLayout->addWidget(projectTitle);

    m_projectTree = new ProjectTreeWidget(leftPanel);
    m_projectTree->setHeaderHidden(true);
    leftLayout->addWidget(m_projectTree);

    m_mainSplitter->addWidget(leftPanel);

    // 中间：报告列表 + 编辑器占位（垂直分割）
    // 中间：报告列表面板（直接占满中间区域，移除多余的编辑器占位区）
    QWidget* reportListPanel = new QWidget(this);
    QVBoxLayout* reportListLayout = new QVBoxLayout(reportListPanel);
    reportListLayout->setContentsMargins(AppTheme::Spacing::Small, AppTheme::Spacing::Small,
                                         AppTheme::Spacing::Small, AppTheme::Spacing::Small);
    reportListLayout->setSpacing(AppTheme::Spacing::Small);

    QLabel* reportListTitle = new QLabel(tr("📝 报告列表"), reportListPanel);
    reportListTitle->setStyleSheet(
        QString("font-weight: bold; padding: %1px; color: %2;")
            .arg(AppTheme::Spacing::Small).arg(AppTheme::Color::Gray333));
    reportListLayout->addWidget(reportListTitle);

    m_reportList = new ReportListWidget(reportListPanel);
    reportListLayout->addWidget(m_reportList);

    m_mainSplitter->addWidget(reportListPanel);

    // 右侧：属性面板
    m_propertyPanel = new QWidget(this);
    m_propertyPanel->setMinimumWidth(AppDimensions::Widget::PropertyPanelMinWidth);
    QVBoxLayout* propLayout = new QVBoxLayout(m_propertyPanel);
    propLayout->setContentsMargins(AppTheme::Spacing::Large, AppTheme::Spacing::Large,
                                   AppTheme::Spacing::Large, AppTheme::Spacing::Large);
    QLabel* propTitle = new QLabel(tr("属性面板"), m_propertyPanel);
    propTitle->setStyleSheet(
        QString("font-weight: bold; font-size: %1px; padding-bottom: %2px; "
                "border-bottom: 1px solid %3;")
            .arg(AppTheme::FontSize::Normal)
            .arg(AppTheme::Spacing::Normal)
            .arg(AppTheme::Color::GrayDDD));
    propLayout->addWidget(propTitle);
    m_propertyContentLabel = new QLabel(m_propertyPanel);
    m_propertyContentLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_propertyContentLabel->setWordWrap(true);
    m_propertyContentLabel->setText(
        QString("<div style='color: %1; font-size: %2px; margin-top: %3px;'>"
                "选择项目或报告后，<br>此处将显示其属性信息。"
                "</div>")
            .arg(AppTheme::Color::Gray999)
            .arg(AppTheme::FontSize::Small)
            .arg(AppTheme::Spacing::Large));
    propLayout->addWidget(m_propertyContentLabel);
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

    m_actionImportData = new QAction(tr("导入数据(&I)..."), this);
    m_actionImportData->setShortcut(QKeySequence("Ctrl+I"));
    m_actionImportData->setStatusTip(tr("从外部文件导入数据表（CSV等）"));

    m_actionExportReport = new QAction(tr("导出项目(&E)..."), this);
    m_actionExportReport->setShortcut(QKeySequence("Ctrl+E"));
    m_actionExportReport->setStatusTip(tr("批量导出当前项目下的所有报告"));
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
    m_actionZoomIn->setShortcut(QKeySequence("Ctrl+="));
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

    m_actionTagManager = new QAction(tr("标签管理(&G)..."), this);
    m_actionTagManager->setStatusTip(tr("管理报告标签"));

    m_actionChangePassword = new QAction(tr("修改密码(&P)..."), this);
    m_actionChangePassword->setStatusTip(tr("修改当前用户密码"));

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

    m_actionPluginManager = new QAction(tr("插件管理..."), this);
    m_actionPluginManager->setStatusTip(tr("查看和管理已加载的插件"));

    m_actionUserManager = new QAction(tr("用户管理..."), this);
    m_actionUserManager->setStatusTip(tr("管理系统用户（仅管理员）"));
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
    fileMenu->addAction(m_actionImportData);
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
    toolsMenu->addAction(m_actionTagManager);
    toolsMenu->addSeparator();
    toolsMenu->addAction(m_actionChangePassword);
    toolsMenu->addSeparator();
    toolsMenu->addAction(m_actionBackup);
    toolsMenu->addAction(m_actionRestore);
    toolsMenu->addSeparator();
    toolsMenu->addAction(m_actionUserManager);
    toolsMenu->addAction(m_actionSettings);

    // 帮助菜单
    QMenu* helpMenu = menuBar->addMenu(tr("帮助(&H)"));
    helpMenu->addAction(m_actionAbout);
    helpMenu->addAction(m_actionAboutQt);
    helpMenu->addSeparator();
    helpMenu->addAction(m_actionPluginManager);
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

    // 导出
    toolBar->addAction(m_actionExportReport);
    toolBar->addSeparator();

    // 全局搜索框
    m_globalSearchEdit = new QLineEdit(toolBar);
    m_globalSearchEdit->setPlaceholderText(tr("🔍 全局搜索报告..."));
    m_globalSearchEdit->setClearButtonEnabled(true);
    m_globalSearchEdit->setMaximumWidth(AppDimensions::Widget::GlobalSearchMaxWidth);
    toolBar->addWidget(m_globalSearchEdit);

    toolBar->addSeparator();
    toolBar->addAction(m_actionSettings);
}

void MainWindow::createStatusBar()
{
    QStatusBar* statusBar = this->statusBar();

    const QString statusLabelStyle =
        QString("padding: 0 %1px;").arg(AppTheme::Spacing::Normal);

    m_statusProjectLabel = new QLabel(tr("项目: 全部"), this);
    m_statusProjectLabel->setStyleSheet(statusLabelStyle);
    statusBar->addWidget(m_statusProjectLabel);

    m_statusReportLabel = new QLabel(tr("报告: 无"), this);
    m_statusReportLabel->setStyleSheet(statusLabelStyle);
    statusBar->addWidget(m_statusReportLabel);

    // QStatusBar 没有 addStretch 方法，用一个空 QWidget 作为弹簧
    // 使后续的 permanent widget 靠右显示
    QWidget* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    statusBar->addWidget(spacer, 1);

    m_statusCountLabel = new QLabel(this);
    m_statusCountLabel->setStyleSheet(
        QString("padding: 0 %1px; color: %2;")
            .arg(AppTheme::Spacing::Normal).arg(AppTheme::Color::Gray666));
    statusBar->addPermanentWidget(m_statusCountLabel);

    updateStatusBar();

    // 根据配置显示或隐藏状态栏
    statusBar->setVisible(AppConfig::instance().showStatusBar());
}

void MainWindow::connectSignals()
{
    // 文件菜单
    connect(m_actionNewProject, &QAction::triggered, this, &MainWindow::onNewProject);
    connect(m_actionNewReport, &QAction::triggered, this, &MainWindow::onNewReport);
    connect(m_actionOpenReport, &QAction::triggered, this, &MainWindow::onOpenReport);
    connect(m_actionImportData, &QAction::triggered, this, &MainWindow::onImportData);
    connect(m_actionExportReport, &QAction::triggered, this, &MainWindow::onExportProject);
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
    connect(m_actionTagManager, &QAction::triggered, this, &MainWindow::onTagManager);
    connect(m_actionChangePassword, &QAction::triggered, this, &MainWindow::onChangePassword);
    connect(m_actionBackup, &QAction::triggered, this, &MainWindow::onDataBackup);
    connect(m_actionRestore, &QAction::triggered, this, &MainWindow::onDataRestore);
    connect(m_actionSettings, &QAction::triggered, this, &MainWindow::onSettings);

    // 帮助菜单
    connect(m_actionAbout, &QAction::triggered, this, &MainWindow::onAbout);
    connect(m_actionAboutQt, &QAction::triggered, this, &MainWindow::onAboutQt);
    connect(m_actionCheckUpdate, &QAction::triggered, this, &MainWindow::onCheckUpdate);
    connect(m_actionPluginManager, &QAction::triggered, this, &MainWindow::onPluginManager);
    connect(m_actionUserManager, &QAction::triggered, this, &MainWindow::onUserManager);

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
    connect(m_reportList, &ReportListWidget::reportSelected,
            this, [this](qint64) { updatePropertyPanel(); });

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
    // 新建报告时自动设置创建者为当前用户
    report->setCreatedBy(UserSession::instance().userId());
    report->setAuthor(UserSession::instance().displayName());

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

// ===========================================================================
// 导入数据
// ===========================================================================

void MainWindow::onImportData()
{
    // 获取所有导入插件
    if (!m_pluginManager) {
        QMessageBox::warning(this, tr("错误"), tr("插件管理器未初始化"));
        return;
    }

    const QList<ImportPluginInterface*> importPlugins =
        m_pluginManager->pluginsOfType<ImportPluginInterface>();

    if (importPlugins.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("没有可用的导入插件"));
        return;
    }

    // 构建文件过滤器
    QStringList filters;
    QMap<QString, ImportPluginInterface*> filterToPlugin;
    for (ImportPluginInterface* plugin : importPlugins) {
        const QString filter = plugin->fileFilter();
        if (!filter.isEmpty()) {
            filters.append(filter);
            filterToPlugin.insert(filter, plugin);
        }
    }
    filters.append(tr("所有文件 (*.*)"));

    // 让用户选择文件
    const QString filePath = QFileDialog::getOpenFileName(this,
        tr("导入数据"), QDir::homePath(), filters.join(";;"));

    if (filePath.isEmpty()) return;

    // 根据文件扩展名选择合适的导入插件
    ImportPluginInterface* selectedPlugin = nullptr;
    const QFileInfo fileInfo(filePath);
    const QString suffix = fileInfo.suffix().toLower();

    for (ImportPluginInterface* plugin : importPlugins) {
        if (plugin->supportedFormats().contains(suffix)) {
            selectedPlugin = plugin;
            break;
        }
    }

    // 如果没有匹配的插件，让用户选择
    if (!selectedPlugin) {
        QStringList pluginNames;
        for (ImportPluginInterface* plugin : importPlugins) {
            pluginNames.append(plugin->name());
        }
        bool ok = false;
        const QString choice = QInputDialog::getItem(this, tr("选择导入插件"),
            tr("无法自动识别文件格式，请选择导入插件："),
            pluginNames, 0, false, &ok);
        if (!ok || choice.isEmpty()) return;

        for (ImportPluginInterface* plugin : importPlugins) {
            if (plugin->name() == choice) {
                selectedPlugin = plugin;
                break;
            }
        }
    }

    if (!selectedPlugin) {
        QMessageBox::warning(this, tr("错误"), tr("未选择导入插件"));
        return;
    }

    // 执行导入
    QApplication::setOverrideCursor(Qt::WaitCursor);
    DataTable::Ptr table = selectedPlugin->importFromFile(filePath, this);
    QApplication::restoreOverrideCursor();

    if (!table) {
        QMessageBox::warning(this, tr("导入失败"), tr("数据导入失败，请检查文件格式"));
        return;
    }

    // 设置数据表名称（使用文件名）
    if (table->name().isEmpty()) {
        table->setName(fileInfo.baseName());
    }
    // 导入的数据表为全局数据表，不关联到特定报告
    table->setReportId(0);

    // 保存到数据库
    if (!DataTableRepository::insert(table)) {
        QMessageBox::warning(this, tr("保存失败"), tr("数据表保存到数据库失败"));
        return;
    }

    QMessageBox::information(this, tr("导入成功"),
        tr("数据导入成功！\n\n"
           "数据表名称：%1\n"
           "行数：%2\n"
           "列数：%3\n\n"
           "可在报告编辑器的图表配置中选择此数据表作为数据源。")
            .arg(table->name())
            .arg(table->rowCount())
            .arg(table->columnCount()));

    showStatusMessage(tr("数据导入成功：%1").arg(table->name()));
    LOG_INFO(QString("数据导入成功: %1 (行数=%2, 列数=%3)")
                 .arg(table->name()).arg(table->rowCount()).arg(table->columnCount()));
}

void MainWindow::onExportProject()
{
    const qint64 projectId = currentProjectId();
    if (projectId <= 0) {
        QMessageBox::information(this, tr("提示"), tr("请先在左侧选择要导出的项目"));
        return;
    }

    // 加载项目信息
    Project::Ptr project = ProjectRepository::findById(projectId);
    if (!project) {
        QMessageBox::warning(this, tr("错误"), tr("无法加载项目信息"));
        return;
    }

    // 查询该项目下的所有报告
    QList<Report::Ptr> reports = ReportRepository::findByProject(projectId);
    if (reports.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("该项目下没有报告可导出"));
        return;
    }

    // 让用户选择导出格式
    const QString formatStr = QInputDialog::getItem(this, tr("导出项目"),
        tr("选择导出格式："),
        QStringList() << tr("PDF 文档") << tr("HTML 网页") << tr("Word 文档") << tr("纯文本"),
        0, false);

    if (formatStr.isEmpty()) {
        return;  // 用户取消
    }

    // 解析选择的格式
    ExportFormat format = ExportFormat::Pdf;
    QString ext = "pdf";
    if (formatStr == tr("PDF 文档")) {
        format = ExportFormat::Pdf;
        ext = "pdf";
    } else if (formatStr == tr("HTML 网页")) {
        format = ExportFormat::Html;
        ext = "html";
    } else if (formatStr == tr("Word 文档")) {
        format = ExportFormat::Word;
        ext = "docx";
    } else if (formatStr == tr("纯文本")) {
        format = ExportFormat::Text;
        ext = "txt";
    }

    // 让用户选择保存目录
    const QString dirPath = QFileDialog::getExistingDirectory(this,
        tr("选择导出目录"), QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dirPath.isEmpty()) {
        return;  // 用户取消
    }

    // 创建以项目名命名的子目录
    const QString projectDir = QDir(dirPath).filePath(project->name());
    QDir().mkpath(projectDir);

    // 循环导出每个报告
    int successCount = 0;
    int failCount = 0;
    QStringList failedReports;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    ExportManager exporter;
    for (const Report::Ptr& report : reports) {
        // 构造文件名（用报告标题，替换非法字符）
        QString fileName = report->title();
        if (fileName.isEmpty()) {
            fileName = tr("未命名报告_%1").arg(report->id());
        }
        // 替换文件名中的非法字符
        fileName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
        const QString filePath = QDir(projectDir).filePath(QString("%1.%2").arg(fileName, ext));

        // 执行导出
        ExportConfig config;
        config.format = format;
        config.filePath = filePath;
        config.includeTitle = true;
        config.includeMeta = true;

        if (exporter.exportReport(report, config, this)) {
            successCount++;
        } else {
            failCount++;
            failedReports << report->title();
        }
    }

    QApplication::restoreOverrideCursor();

    // 显示导出结果
    QString message = tr("项目导出完成！\n\n"
                         "成功：%1 份\n"
                         "失败：%2 份\n"
                         "导出目录：\n%3").arg(successCount).arg(failCount).arg(projectDir);

    if (!failedReports.isEmpty()) {
        message += tr("\n\n失败的报告：\n") + failedReports.join("\n");
    }

    showStatusMessage(tr("项目导出完成：成功 %1，失败 %2").arg(successCount).arg(failCount));

    if (failCount == 0) {
        QMessageBox::information(this, tr("导出成功"), message);
    } else {
        QMessageBox::warning(this, tr("导出完成（部分失败）"), message);
    }
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

    // 权限检查
    if (!canModifyProject(projectId)) {
        showPermissionDenied();
        return;
    }

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

    // 权限检查
    if (!canModifyReport(reportId)) {
        showPermissionDenied();
        return;
    }

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
    m_globalSearchEdit->setFocus();
    m_globalSearchEdit->selectAll();
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

void MainWindow::onTagManager()
{
    TagManagerDialog dialog(this);
    dialog.exec();
    // 标签可能被修改，刷新报告列表和属性面板
    m_reportList->refreshList();
    updatePropertyPanel();
}

void MainWindow::onChangePassword()
{
    ChangePasswordDialog dialog(false, this);
    if (dialog.exec() != QDialog::Accepted) return;

    // 验证原密码
    const QString username = UserSession::instance().username();
    User::Ptr user = UserRepository::authenticate(username, dialog.oldPassword());
    if (!user) {
        QMessageBox::warning(this, tr("修改失败"), tr("原密码不正确"));
        return;
    }

    // 修改密码
    if (UserRepository::changePassword(user->id(), dialog.newPassword())) {
        QMessageBox::information(this, tr("修改成功"), tr("密码已修改成功，下次登录请使用新密码"));
    } else {
        QMessageBox::critical(this, tr("修改失败"), tr("修改密码时发生错误"));
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
    const QString dbPath = QCoreApplication::applicationDirPath()
        + "/data/experiment_reports.db";

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
    SettingsDialog dialog(this);
    dialog.exec();
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
            "<p style='color: %4; font-size: %5px;'>%6</p>"
        ).arg(AppConstants::APP_DISPLAY_NAME)
         .arg(AppConstants::APP_VERSION)
         .arg(qVersion())
         .arg(AppTheme::Color::TextSecondary)
         .arg(AppTheme::FontSize::ExtraSmall)
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

void MainWindow::onPluginManager()
{
    if (!m_pluginManager) {
        QMessageBox::warning(this, tr("提示"), tr("插件管理器未初始化"));
        return;
    }

    PluginManagerDialog dialog(m_pluginManager, this);
    dialog.exec();
}

void MainWindow::onUserManager()
{
    // 仅管理员可访问
    if (!UserSession::instance().isAdmin()) {
        QMessageBox::warning(this, tr("权限不足"), tr("只有管理员可以管理用户"));
        return;
    }

    UserManagerDialog dialog(this);
    dialog.exec();
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
    updatePropertyPanel();
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
    // 检查是否已经打开了该报告的编辑器
    for (const QPointer<ReportEditorWindow>& winPtr : m_editorWindows) {
        if (winPtr && winPtr->currentReport() && winPtr->currentReport()->id() == reportId) {
            winPtr->raise();
            winPtr->activateWindow();
            return;
        }
    }

    // 创建新的报告编辑器窗口
    ReportEditorWindow* editorWindow = new ReportEditorWindow(report, this);
    editorWindow->setAttribute(Qt::WA_DeleteOnClose);
    // 传递插件管理器，使报告编辑器能集成报告级工具插件（附件管理、版本历史等）
    editorWindow->setPluginManager(m_pluginManager);
    connect(editorWindow, &ReportEditorWindow::reportSaved,
            this, &MainWindow::onReportEditorSaved);
    connect(editorWindow, &ReportEditorWindow::windowClosed,
            this, &MainWindow::onReportEditorClosed);

    m_editorWindows.append(QPointer<ReportEditorWindow>(editorWindow));
    editorWindow->show();

    showStatusMessage(tr("已打开报告: %1").arg(report->title()));
    m_statusReportLabel->setText(tr("报告: %1").arg(report->title()));
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
    updatePropertyPanel();
}

// ===========================================================================
// 报告编辑器窗口信号槽
// ===========================================================================

void MainWindow::onReportEditorSaved(qint64 reportId)
{
    Q_UNUSED(reportId);
    // 报告保存后刷新列表和属性面板
    m_reportList->refreshList();
    updatePropertyPanel();
    updateStatusBar();
    showStatusMessage(tr("报告已保存"));
}

void MainWindow::onReportEditorClosed(qint64 reportId)
{
    Q_UNUSED(reportId);
    // 从列表中移除已关闭的窗口（QPointer 会自动检测对象是否已删除）
    for (int i = m_editorWindows.size() - 1; i >= 0; --i) {
        if (m_editorWindows.at(i).isNull()) {
            m_editorWindows.removeAt(i);
        }
    }
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

bool MainWindow::canModifyReport(qint64 reportId) const
{
    // 管理员可以修改所有
    if (UserSession::instance().isAdmin()) return true;

    Report::Ptr report = ReportRepository::findById(reportId);
    if (!report) return false;

    // 未分配创建者的旧数据，所有人都可以修改（兼容旧数据）
    if (report->createdBy() <= 0) return true;

    // 只有创建者可以修改
    return report->createdBy() == UserSession::instance().userId();
}

bool MainWindow::canModifyProject(qint64 projectId) const
{
    // 管理员可以修改所有
    if (UserSession::instance().isAdmin()) return true;

    Project::Ptr project = ProjectRepository::findById(projectId);
    if (!project) return false;

    // 未分配创建者的旧数据，所有人都可以修改（兼容旧数据）
    if (project->createdBy() <= 0) return true;

    // 只有创建者可以修改
    return project->createdBy() == UserSession::instance().userId();
}

void MainWindow::showPermissionDenied() const
{
    QMessageBox::warning(nullptr, tr("权限不足"),
        tr("您没有权限修改此数据。\n只有创建者或管理员可以修改。"));
}

void MainWindow::showStatusMessage(const QString& message, int timeout)
{
    // 如果未指定超时时间，使用配置中的默认值
    const int actualTimeout = (timeout > 0) ? timeout
                                            : AppConfig::instance().statusMessageDuration();
    statusBar()->showMessage(message, actualTimeout);
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
    m_actionExportReport->setEnabled(hasProject);  // 导出项目需要选中项目
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

void MainWindow::updatePropertyPanel()
{
    if (!m_propertyContentLabel) return;

    // 优先显示当前选中的报告属性
    const qint64 reportId = currentReportId();
    if (reportId > 0) {
        Report::Ptr report = ReportRepository::findById(reportId);
        if (report) {
            // 状态字符串
            QString statusStr;
            switch (report->status()) {
                case ReportStatus::Draft:     statusStr = tr("草稿"); break;
                case ReportStatus::Submitted: statusStr = tr("已提交"); break;
                case ReportStatus::Reviewed:  statusStr = tr("已审核"); break;
                default:                       statusStr = tr("未知"); break;
            }

            // 项目名称
            QString projectName = tr("未分类");
            if (report->projectId() > 0) {
                Project::Ptr project = ProjectRepository::findById(report->projectId());
                if (project) projectName = project->name();
            }

            // 标签
            const Tag::List tags = TagRepository::findByReport(report->id());
            QString tagsHtml;
            if (tags.isEmpty()) {
                tagsHtml = tr("无标签");
            } else {
                for (int i = 0; i < tags.size(); ++i) {
                    const QColor color = tags[i]->effectiveColor();
                    if (i > 0) tagsHtml += "&nbsp;&nbsp;";
                    tagsHtml += QString("<span style='color: %1;'>■</span>&nbsp;%2")
                                    .arg(color.name())
                                    .arg(tags[i]->name().toHtmlEscaped());
                }
            }

            // 创建者
            QString creatorName = tr("未分配");
            if (report->createdBy() > 0) {
                User::Ptr creator = UserRepository::findById(report->createdBy());
                if (creator) creatorName = creator->displayNameOrUsername();
                else creatorName = tr("未知用户");
            }

            const QString html = QString(
                "<div style='font-size: %1px; line-height: 1.8;'>"
                "<p><b style='color: %2;'>报告属性</b></p>"
                "<p><b>标题：</b>%3</p>"
                "<p><b>状态：</b>%4</p>"
                "<p><b>项目：</b>%5</p>"
                "<p><b>创建者：</b>%6</p>"
                "<p><b>实验日期：</b>%7</p>"
                "<p><b>更新时间：</b>%8</p>"
                "<p><b>字数：</b>%9 字</p>"
                "<p><b>内容块：</b>%10 个</p>"
                "<p><b>标签：</b>%11</p>"
                "<p><b>报告ID：</b>%12</p>"
                "</div>"
            ).arg(AppTheme::FontSize::Small)
             .arg(AppTheme::Color::Primary)
             .arg(report->title().isEmpty() ? tr("未命名") : report->title().toHtmlEscaped())
             .arg(statusStr)
             .arg(projectName.toHtmlEscaped())
             .arg(creatorName.toHtmlEscaped())
             .arg(report->experimentDate().isValid() ? report->experimentDate().toString("yyyy-MM-dd") : tr("未设置"))
             .arg(report->updatedAt().isValid() ? report->updatedAt().toString("yyyy-MM-dd HH:mm") : tr("未知"))
             .arg(QString::number(report->wordCount()))
             .arg(QString::number(report->blockCount()))
             .arg(tagsHtml)
             .arg(QString::number(report->id()));

            m_propertyContentLabel->setText(html);
            return;
        }
    }

    // 如果没有选中报告，显示当前项目属性
    const qint64 projectId = currentProjectId();
    if (projectId > 0) {
        Project::Ptr project = ProjectRepository::findById(projectId);
        if (project) {
            const int reportCount = ReportRepository::countByProject(projectId);
            const QString html = QString(
                "<div style='font-size: %1px; line-height: 1.8;'>"
                "<p><b style='color: %2;'>项目属性</b></p>"
                "<p><b>名称：</b>%3</p>"
                "<p><b>描述：</b>%4</p>"
                "<p><b>报告数：</b>%5 份</p>"
                "<p><b>创建时间：</b>%6</p>"
                "<p><b>项目ID：</b>%7</p>"
                "</div>"
            ).arg(AppTheme::FontSize::Small)
             .arg(AppTheme::Color::Success)
             .arg(project->name().toHtmlEscaped())
             .arg(project->description().isEmpty() ? tr("无描述") : project->description().toHtmlEscaped())
             .arg(QString::number(reportCount))
             .arg(project->createdAt().isValid() ? project->createdAt().toString("yyyy-MM-dd HH:mm") : tr("未知"))
             .arg(QString::number(project->id()));

            m_propertyContentLabel->setText(html);
            return;
        }
    }

    // 都没有选中，显示提示
    m_propertyContentLabel->setText(QString(
        "<div style='color: %1; font-size: %2px; margin-top: %3px;'>"
        "选择项目或报告后，<br>此处将显示其属性信息。"
        "</div>").arg(AppTheme::Color::TextSecondary)
                  .arg(AppTheme::FontSize::Small)
                  .arg(AppTheme::Spacing::Large));
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
