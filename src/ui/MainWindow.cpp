/**
 * @file MainWindow.cpp
 * @brief 主窗口实现文件
 */

#include "MainWindow.h"
#include <QStyle>
#include "ui_MainWindow.h"  // 由 uic 工具从 .ui 文件自动生成
#include "ui/widgets/ProjectTreeWidget.h"
#include "ui/widgets/ReportListWidget.h"
#include "ui/widgets/PropertyPanelHelper.h"
#include "ui/ReportEditorWindow.h"
#include "ui/MainWindowDialogs.h"
#include "ui/dialogs/SearchResultDialog.h"
#include "ui/dialogs/PluginManagerDialog.h"
#include "core/plugin/PluginManager.h"
#include "service/ReportService.h"
#include "service/ProjectService.h"
#include "core/models/Tag.h"
#include "core/models/DataTable.h"
#include "core/utils/Logger.h"
#include "core/utils/AppConstants.h"
#include "core/utils/AppTheme.h"
#include "core/utils/AppDimensions.h"
#include "core/utils/AppConfig.h"
#include "core/utils/UserSession.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include "ui/UiHelper.h"
#include <QStandardPaths>
#include <QCoreApplication>
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

    // 对话框操作控制器（hooks 注入窗口能力）
    m_dialogs = new MainWindowDialogs(this, MainWindowDialogs::Hooks{
        [this]() { return currentProjectId(); },
        [this]() { return currentReportId(); },
        [this](qint64 id) { m_projectTree->selectProject(id); },
        [this](qint64 id) { m_reportList->setProjectId(id); },
        [this]() { m_projectTree->refreshTree(); },
        [this]() { m_reportList->refreshList(); },
        [this]() { updatePropertyPanel(); },
        [this]() { updateStatusBar(); },
        [this]() { updateActionsState(); },
        [this](const QString& msg) { showStatusMessage(msg); },
        [this]() -> bool {
            // 关闭所有已打开的报告编辑器窗口（数据恢复用）
            for (int i = m_editorWindows.size() - 1; i >= 0; --i) {
                if (!m_editorWindows.at(i).isNull()) {
                    if (!m_editorWindows.at(i)->close()) return false;
                }
            }
            m_editorWindows.clear();
            m_currentReportId = -1;
            return true;
        },
    });

    createActions();
    createToolBar();
    createStatusBar();
    connectSignals();
    loadSettings();
    updateWindowTitle();
    updateActionsState();

    // 根据用户角色显示/隐藏用户管理菜单
    m_actionUserManager->setVisible(UserSession::instance().isAdmin());

    LOG_DEBUG("主窗口初始化完成");
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete m_dialogs;
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

    // 中央三栏布局已在 MainWindow.ui 中定义（含 ProjectTreeWidget/ReportListWidget 提升），
    // 此处仅取出成员引用，不再动态创建
    m_mainSplitter = ui->m_mainSplitter;
    m_projectTree = ui->m_projectTree;
    m_reportList = ui->m_reportList;
    m_propertyPanel = ui->m_propertyPanel;
    m_propertyContentLabel = ui->m_propertyContentLabel;

    // 设置主分割器比例（.ui 无法表达 splitter 初始比例，保留代码设置）
    m_mainSplitter->setStretchFactor(0, 1);  // 项目树
    m_mainSplitter->setStretchFactor(1, 3);  // 中间区域
    m_mainSplitter->setStretchFactor(2, 1);  // 属性面板
    m_mainSplitter->setSizes({250, 600, 250});
}

void MainWindow::createActions()
{
    // -----------------------------------------------------------------------
    // 文件菜单动作
    // -----------------------------------------------------------------------
    m_actionNewProject = ui->m_actionNewProject;
    m_actionNewProject->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));

    m_actionNewReport = ui->m_actionNewReport;
    m_actionNewReport->setIcon(style()->standardIcon(QStyle::SP_FileIcon));

    m_actionOpenReport = ui->m_actionOpenReport;
    m_actionOpenReport->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));

    m_actionImportData = ui->m_actionImportData;
    m_actionImportData->setIcon(style()->standardIcon(QStyle::SP_FileDialogContentsView));

    m_actionExportReport = ui->m_actionExportReport;
    m_actionExportReport->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));

    m_actionExit = ui->m_actionExit;
    m_actionExit->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    // -----------------------------------------------------------------------
    // 编辑菜单动作
    // -----------------------------------------------------------------------
    m_actionEditProject = ui->m_actionEditProject;
    m_actionEditProject->setIcon(style()->standardIcon(QStyle::SP_FileDialogContentsView));

    m_actionDeleteProject = ui->m_actionDeleteProject;
    m_actionDeleteProject->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));

    m_actionDeleteReport = ui->m_actionDeleteReport;
    m_actionDeleteReport->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));

    m_actionFind = ui->m_actionFind;
    m_actionFind->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    // -----------------------------------------------------------------------
    // 视图菜单动作
    // -----------------------------------------------------------------------
    m_actionToggleProjectPanel = ui->m_actionToggleProjectPanel;
    m_actionToggleProjectPanel->setIcon(style()->standardIcon(QStyle::SP_FileDialogListView));

    m_actionTogglePropertyPanel = ui->m_actionTogglePropertyPanel;
    m_actionTogglePropertyPanel->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));

    m_actionFullscreen = ui->m_actionFullscreen;
    m_actionFullscreen->setIcon(style()->standardIcon(QStyle::SP_DesktopIcon));

    m_actionZoomIn = ui->m_actionZoomIn;
    m_actionZoomIn->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));

    m_actionZoomOut = ui->m_actionZoomOut;
    m_actionZoomOut->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));

    m_actionResetZoom = ui->m_actionResetZoom;
    m_actionResetZoom->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    // -----------------------------------------------------------------------
    // 工具菜单动作
    // -----------------------------------------------------------------------
    m_actionTemplateManager = ui->m_actionTemplateManager;
    m_actionTemplateManager->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));

    m_actionTagManager = ui->m_actionTagManager;
    m_actionTagManager->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));

    m_actionChangePassword = ui->m_actionChangePassword;
    m_actionChangePassword->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));

    m_actionBackup = ui->m_actionBackup;
    m_actionBackup->setIcon(style()->standardIcon(QStyle::SP_DriveHDIcon));

    m_actionRestore = ui->m_actionRestore;
    m_actionRestore->setIcon(style()->standardIcon(QStyle::SP_DriveFDIcon));

    m_actionSettings = ui->m_actionSettings;
    m_actionSettings->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    // -----------------------------------------------------------------------
    // 帮助菜单动作
    // -----------------------------------------------------------------------
    m_actionAbout = ui->m_actionAbout;
    m_actionAbout->setIcon(style()->standardIcon(QStyle::SP_DialogHelpButton));

    m_actionAboutQt = ui->m_actionAboutQt;

    m_actionCheckUpdate = ui->m_actionCheckUpdate;
    m_actionCheckUpdate->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));

    m_actionPluginManager = ui->m_actionPluginManager;
    m_actionPluginManager->setIcon(style()->standardIcon(QStyle::SP_FileDialogListView));

    m_actionUserManager = ui->m_actionUserManager;
    m_actionUserManager->setIcon(style()->standardIcon(QStyle::SP_DirHomeIcon));
}

void MainWindow::createToolBar()
{
    QToolBar* toolBar = ui->mainToolBar;  // 使用 .ui 中定义的工具栏，避免重复创建
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

    // 全局搜索框（已在 MainWindow.ui 的工具栏中定义）
    m_globalSearchEdit = ui->m_globalSearchEdit;

    toolBar->addSeparator();
    toolBar->addAction(m_actionSettings);
}

void MainWindow::createStatusBar()
{
    QStatusBar* statusBar = this->statusBar();

    // 状态栏标签用 addWidget/addPermanentWidget 显式添加
    // （QStatusBar 子 widget 若仅写在 .ui 中，uic 不会生成 addWidget，导致控件堆叠重叠）
    const QString statusLabelStyle =
        QString("padding: 0 %1px;").arg(AppTheme::Spacing::Normal);

    // 主界面状态栏：显示当前登录用户
    m_statusUserLabel = new QLabel(this);
    m_statusUserLabel->setStyleSheet(
        QString("padding: 0 %1px; color: %2; font-weight: bold;")
            .arg(AppTheme::Spacing::Normal).arg(AppTheme::Color::Primary));
    statusBar->addWidget(m_statusUserLabel);

    m_statusProjectLabel = new QLabel(tr("项目: 全部"), this);
    m_statusProjectLabel->setStyleSheet(statusLabelStyle);
    statusBar->addWidget(m_statusProjectLabel);

    m_statusReportLabel = new QLabel(tr("报告: 无"), this);
    m_statusReportLabel->setStyleSheet(statusLabelStyle);
    statusBar->addWidget(m_statusReportLabel);

    // QStatusBar 没有 addStretch 方法，用一个空 QWidget 作为弹簧
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

    // 文件菜单：登出（用户名显示在主界面状态栏）
    connect(ui->m_actionLogout, &QAction::triggered, this, &MainWindow::onLogout);

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
    if (m_dialogs) m_dialogs->newProject();
}

void MainWindow::onNewReport()
{
    if (m_dialogs) m_dialogs->newReport();
}

void MainWindow::onOpenReport()
{
    const qint64 reportId = currentReportId();
    if (reportId > 0) {
        onReportOpenRequested(reportId);
    } else {
        UiHelper::info(this, tr("提示"), tr("请先在报告列表中选择一份报告"));
    }
}

// ===========================================================================
// 导入数据
// ===========================================================================

void MainWindow::onImportData()
{
    if (m_dialogs) m_dialogs->importData();
}

void MainWindow::onExportProject()
{
    if (m_dialogs) m_dialogs->exportProject();
}

void MainWindow::onExit()
{
    close();
}

void MainWindow::onLogout()
{
    const bool confirmed = UiHelper::confirm(
        this, tr("登出"), tr("确定要退出当前账号并返回登录界面吗？"));
    if (!confirmed) return;

    // 先逐个关闭报告编辑器窗口（close() 会触发 closeEvent 询问未保存）
    // 若用户在某个窗口点"取消"，中止登出，避免会话已清但窗口残留
    for (int i = m_editorWindows.size() - 1; i >= 0; --i) {
        if (m_editorWindows.at(i).isNull()) continue;
        if (!m_editorWindows.at(i)->close()) {
            showStatusMessage(tr("登出已取消：有未保存的报告"));
            return;
        }
    }

    // 清除登录会话
    UserSession::instance().clear();
    // 关闭主窗口，由 main.cpp 回到登录流程
    close();
}

// ===========================================================================
// 编辑菜单槽函数
// ===========================================================================

void MainWindow::onEditProject()
{
    if (m_dialogs) m_dialogs->editProject();
}

void MainWindow::onDeleteProject()
{
    const qint64 projectId = currentProjectId();
    if (projectId <= 0) return;

    // 权限检查留在窗口槽
    if (!canModifyProject(projectId)) {
        showPermissionDenied();
        return;
    }

    if (m_dialogs) m_dialogs->deleteProject(projectId);
}

void MainWindow::onDeleteReport()
{
    const qint64 reportId = currentReportId();
    if (reportId <= 0) return;

    // 权限检查留在窗口槽
    if (!canModifyReport(reportId)) {
        showPermissionDenied();
        return;
    }

    if (m_dialogs) m_dialogs->deleteReport(reportId);
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
    if (m_dialogs) m_dialogs->templateManager();
}

void MainWindow::onTagManager()
{
    if (m_dialogs) m_dialogs->tagManager();
}

void MainWindow::onChangePassword()
{
    if (m_dialogs) m_dialogs->changePassword();
}

void MainWindow::onDataBackup()
{
    if (m_dialogs) m_dialogs->dataBackup();
}

void MainWindow::onDataRestore()
{
    if (m_dialogs) m_dialogs->dataRestore();
}

void MainWindow::onSettings()
{
    if (m_dialogs) m_dialogs->settings();
}

// ===========================================================================
// 帮助菜单槽函数
// ===========================================================================

void MainWindow::onAbout()
{
    if (m_dialogs) m_dialogs->about();
}

void MainWindow::onAboutQt()
{
    if (m_dialogs) m_dialogs->aboutQt();
}

void MainWindow::onCheckUpdate()
{
    if (m_dialogs) m_dialogs->checkUpdate();
}

void MainWindow::onPluginManager()
{
    if (!m_pluginManager) {
        UiHelper::warning(this, tr("提示"), tr("插件管理器未初始化"));
        return;
    }

    PluginManagerDialog dialog(m_pluginManager, this);
    dialog.exec();
}

void MainWindow::onUserManager()
{
    if (m_dialogs) m_dialogs->userManager();
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
    Report::Ptr report = ReportService::getById(reportId);
    if (!report) {
        UiHelper::warning(this, tr("错误"), tr("未找到报告"));
        return;
    }
    m_currentReportId = reportId;

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
    updateStatusBar();
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

    Report::Ptr report = ReportService::getById(reportId);
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

    Project::Ptr project = ProjectService::getById(projectId);
    if (!project) return false;

    // 未分配创建者的旧数据，所有人都可以修改（兼容旧数据）
    if (project->createdBy() <= 0) return true;

    // 只有创建者可以修改
    return project->createdBy() == UserSession::instance().userId();
}

void MainWindow::showPermissionDenied() const
{
    UiHelper::warning(nullptr, tr("权限不足"),
        tr("您没有权限修改此数据。\n只有创建者或管理员可以修改。"));
}

void MainWindow::showStatusMessage(const QString& message, int timeout)
{
    // 如果未指定超时时间，使用配置中的默认值
    const int actualTimeout = (timeout > 0) ? timeout
                                            : AppConfig::instance().statusMessageDuration();
    statusBar()->showMessage(message, actualTimeout);
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
        Project::Ptr project = ProjectService::getById(m_currentProjectId);
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
    // 当前登录用户
    m_statusUserLabel->setText(tr("当前用户: %1")
        .arg(UserSession::instance().displayName()));

    // 当前项目
    if (m_currentProjectId > 0) {
        Project::Ptr project = ProjectService::getById(m_currentProjectId);
        if (project) {
            m_statusProjectLabel->setText(tr("项目: %1").arg(project->name()));
        }
    } else {
        m_statusProjectLabel->setText(tr("项目: 全部"));
    }

    // 当前报告：优先显示最后打开且仍存在的编辑器窗口，全部关闭则显示"无"
    m_statusReportLabel->setText(tr("报告: 无"));
    for (auto it = m_editorWindows.rbegin(); it != m_editorWindows.rend(); ++it) {
        if (!it->isNull()) {
            const QString title = (*it)->reportTitle();
            if (!title.isEmpty()) {
                m_statusReportLabel->setText(tr("报告: %1").arg(title));
            }
            break;
        }
    }

    // 统计信息
    const int projectCount = ProjectService::count();
    const int reportCount = ReportService::count();
    m_statusCountLabel->setText(
        tr("项目: %1 | 报告: %2").arg(projectCount).arg(reportCount));
}

void MainWindow::updatePropertyPanel()
{
    if (!m_propertyContentLabel) return;

    // 优先显示当前选中的报告属性
    const qint64 reportId = currentReportId();
    if (reportId > 0) {
        const Report::Ptr report = ReportService::getById(reportId);
        if (report) {
            m_propertyContentLabel->setText(PropertyPanelHelper::reportHtml(report));
            return;
        }
    }

    // 没有选中报告时，显示当前项目属性
    const qint64 projectId = currentProjectId();
    if (projectId > 0) {
        const Project::Ptr project = ProjectService::getById(projectId);
        if (project) {
            m_propertyContentLabel->setText(PropertyPanelHelper::projectHtml(project));
            return;
        }
    }

    // 都没有选中，显示占位提示
    m_propertyContentLabel->setText(PropertyPanelHelper::emptyHtml());
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
    // 先关闭所有报告编辑器窗口：close() 会触发各自的 closeEvent 询问未保存内容
    // 若用户在某个窗口点"取消"，中止主窗口关闭，避免未保存内容静默丢失
    for (int i = m_editorWindows.size() - 1; i >= 0; --i) {
        if (m_editorWindows.at(i).isNull()) continue;
        if (!m_editorWindows.at(i)->close()) {
            event->ignore();
            return;
        }
    }

    saveSettings();
    LOG_DEBUG("主窗口关闭");
    event->accept();
}
