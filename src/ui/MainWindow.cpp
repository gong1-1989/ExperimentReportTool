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
#include "ui/dialogs/ProjectDialog.h"
#include "ui/dialogs/SettingsDialog.h"
#include "ui/dialogs/SearchResultDialog.h"
#include "ui/dialogs/TemplateEditorDialog.h"
#include "ui/dialogs/TagManagerDialog.h"
#include "ui/dialogs/ChangePasswordDialog.h"
#include "ui/dialogs/PluginManagerDialog.h"
#include "ui/dialogs/UserManagerDialog.h"
#include "core/plugin/PluginManager.h"
#include "service/ReportService.h"
#include "service/ProjectService.h"
#include "service/TagService.h"
#include "service/UserService.h"
#include "service/TemplateService.h"
#include "service/DataTableService.h"
#include "utils/CsvImporter.h"
#include "data/repositories/ProjectRepository.h"
#include "data/repositories/ReportRepository.h"
#include "data/repositories/TemplateRepository.h"
#include "data/repositories/TagRepository.h"
#include "data/repositories/DataTableRepository.h"
#include "data/repositories/UserRepository.h"
#include "core/models/Tag.h"
#include "core/models/DataTable.h"
#include "export/ExportManager.h"
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
#include <QMessageBox>
#include "ui/UiHelper.h"
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
        if (ProjectService::save(project)) {
            m_projectTree->refreshTree();
            m_projectTree->selectProject(project->id());
            showStatusMessage(tr("项目「%1」已创建").arg(project->name()));
            LOG_INFO(QString("项目已创建: %1").arg(project->toString()));
        } else {
            UiHelper::error(this, tr("错误"), tr("创建项目失败，请查看日志"));
        }
    }
}

void MainWindow::onNewReport()
{
    const qint64 projectId = currentProjectId();
    if (projectId <= 0) {
        UiHelper::info(this, tr("提示"), tr("请先在左侧选择一个项目"));
        return;
    }

    // 选择模板
    const Template::List templates = TemplateService::listAll();
    if (templates.isEmpty()) {
        UiHelper::warning(this, tr("提示"), tr("没有可用的报告模板"));
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

    if (ReportService::save(report)) {
        m_reportList->refreshList();
        showStatusMessage(tr("报告「%1」已创建").arg(report->title()));
        LOG_INFO(QString("报告已创建: %1").arg(report->toString()));
    } else {
        UiHelper::error(this, tr("错误"), tr("创建报告失败"));
    }
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
    // 文件选择过滤器（内置 CSV 导入）
    QStringList filters;
    filters.append(CsvImporter::fileFilter());
    filters.append(tr("所有文件 (*.*)"));

    // 让用户选择文件
    const QString filePath = QFileDialog::getOpenFileName(this,
        tr("导入数据"), QDir::homePath(), filters.join(";;"));

    if (filePath.isEmpty()) return;

    const QFileInfo fileInfo(filePath);
    const QString suffix = fileInfo.suffix().toLower();

    // 仅支持 csv / txt 格式
    if (!CsvImporter::supportedFormats().contains(suffix)) {
        UiHelper::warning(this, tr("错误"),
            tr("不支持的文件格式：%1\n仅支持 %2 格式。")
                .arg(suffix.isEmpty() ? tr("未知") : suffix)
                .arg(CsvImporter::supportedFormats().join(", ")));
        return;
    }

    // 执行导入
    QString errorMessage;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    DataTable::Ptr table = CsvImporter::importFile(filePath, &errorMessage);
    QApplication::restoreOverrideCursor();

    if (!table) {
        UiHelper::warning(this, tr("导入失败"),
            errorMessage.isEmpty() ? tr("数据导入失败，请检查文件格式") : errorMessage);
        return;
    }

    // 设置数据表名称（使用文件名）
    if (table->name().isEmpty()) {
        table->setName(fileInfo.baseName());
    }
    // 导入的数据表为全局数据表，不关联到特定报告
    table->setReportId(0);

    // 保存到数据库
    if (!DataTableService::save(table)) {
        UiHelper::warning(this, tr("保存失败"), tr("数据表保存到数据库失败"));
        return;
    }

    UiHelper::info(this, tr("导入成功"),
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
        UiHelper::info(this, tr("提示"), tr("请先在左侧选择要导出的项目"));
        return;
    }

    // 加载项目信息
    Project::Ptr project = ProjectService::getById(projectId);
    if (!project) {
        UiHelper::warning(this, tr("错误"), tr("无法加载项目信息"));
        return;
    }

    // 查询该项目下的所有报告
    QList<Report::Ptr> reports = ReportService::listByProject(projectId);
    if (reports.isEmpty()) {
        UiHelper::info(this, tr("提示"), tr("该项目下没有报告可导出"));
        return;
    }

    // 让用户选择导出格式（传 &ok 准确区分"确定"与"取消"）
    bool ok = false;
    const QString formatStr = QInputDialog::getItem(this, tr("导出项目"),
        tr("选择导出格式："),
        QStringList() << tr("PDF 文档") << tr("HTML 网页") << tr("Word 文档") << tr("纯文本"),
        0, false, &ok);

    if (!ok || formatStr.isEmpty()) {
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
        const QString filePath = QDir(projectDir).filePath(QString("%1.%2").arg(fileName).arg(ext));

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
        UiHelper::info(this, tr("导出成功"), message);
    } else {
        UiHelper::warning(this, tr("导出完成（部分失败）"), message);
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

    Project::Ptr project = ProjectService::getById(projectId);
    if (!project) return;

    ProjectDialog dialog(this);
    dialog.setWindowTitle(tr("编辑项目"));
    dialog.setProjectData(project);

    if (dialog.exec() == QDialog::Accepted) {
        Project::Ptr updated = dialog.projectData();
        updated->setId(projectId);
        if (ProjectService::update(updated)) {
            m_projectTree->refreshTree();
            m_projectTree->selectProject(projectId);
            showStatusMessage(tr("项目已更新"));
        } else {
            UiHelper::error(this, tr("错误"), tr("更新项目失败"));
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

    Project::Ptr project = ProjectService::getById(projectId);
    if (!project) return;

    if (UiHelper::confirm(this,
                       tr("确认删除"),
                       tr("确定要删除项目「%1」吗？\n该项目下的所有报告将被同时删除，此操作不可恢复！") .arg(project->name()))) {
        if (ProjectService::remove(projectId)) {
            m_projectTree->refreshTree();
            m_reportList->setProjectId(-1);
            showStatusMessage(tr("项目已删除"));
        } else {
            UiHelper::error(this, tr("错误"), tr("删除项目失败"));
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

    Report::Ptr report = ReportService::getById(reportId);
    if (!report) return;

    if (UiHelper::confirm(this,
                       tr("确认删除"),
                       tr("确定要删除报告「%1」吗？此操作不可恢复！").arg(report->title()))) {
        if (ReportService::remove(reportId)) {
            m_reportList->refreshList();
            showStatusMessage(tr("报告已删除"));
        } else {
            UiHelper::error(this, tr("错误"), tr("删除报告失败"));
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
    const Template::List templates = TemplateService::listAll();

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
                if (!UiHelper::confirm(this,
                                       tr("内置模板"),
                                       tr("「%1」是内置模板，不能直接修改。\n是否创建一个副本进行编辑？")
                                           .arg(temp->name()))) return;

                // 创建副本
                Template::Ptr copy = Template::create();
                copy->setName(temp->name() + tr(" (副本)"));
                copy->setCategory(temp->category());
                copy->setDescription(temp->description());
                copy->setBlocks(temp->blocks());
                TemplateService::save(copy);
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
    User::Ptr user = UserService::authenticate(username, dialog.oldPassword());
    if (!user) {
        UiHelper::warning(this, tr("修改失败"), tr("原密码不正确"));
        return;
    }

    // 修改密码
    if (UserService::changePassword(user->id(), dialog.newPassword())) {
        UiHelper::info(this, tr("修改成功"), tr("密码已修改成功，下次登录请使用新密码"));
    } else {
        UiHelper::error(this, tr("修改失败"), tr("修改密码时发生错误"));
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
        UiHelper::info(this, tr("备份成功"),
            tr("数据已备份到:\n%1").arg(filePath));
        showStatusMessage(tr("数据备份完成"));
    } else {
        UiHelper::error(this, tr("备份失败"),
            tr("无法复制数据库文件。\n请确保目标路径可写。"));
    }
}

void MainWindow::onDataRestore()
{
    UiHelper::warning(this, tr("数据恢复"),
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
    UiHelper::info(this, tr("检查更新"),
        tr("当前已是最新版本: %1\n\n"
           "更新检查功能将在后续版本中实现。").arg(AppConstants::APP_VERSION));
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
    // 仅管理员可访问
    if (!UserSession::instance().isAdmin()) {
        UiHelper::warning(this, tr("权限不足"), tr("只有管理员可以管理用户"));
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
    Report::Ptr report = ReportService::getById(reportId);
    if (!report) {
        UiHelper::warning(this, tr("错误"), tr("未找到报告"));
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
    // 当前项目
    if (m_currentProjectId > 0) {
        Project::Ptr project = ProjectService::getById(m_currentProjectId);
        if (project) {
            m_statusProjectLabel->setText(tr("项目: %1").arg(project->name()));
        }
    } else {
        m_statusProjectLabel->setText(tr("项目: 全部"));
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
    // 检查是否有未保存的更改（编辑器实现后需要检查）
    // 当前版本直接保存设置并关闭

    saveSettings();
    LOG_INFO("主窗口关闭");
    event->accept();
}
