/**
 * @file MainWindow.h
 * @brief 主窗口头文件
 *
 * 应用程序主窗口，采用三栏布局：
 * - 左侧：项目树（ProjectTreeWidget）
 * - 中间：报告列表（ReportListWidget）
 * - 右侧：属性/大纲面板（占位，后续实现）
 *
 * 包含菜单栏、工具栏、状态栏，负责全局操作的调度。
 */

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QAction>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QStackedWidget>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>
#include <QPointer>
#include <QList>

#include "core/models/Project.h"
#include "core/models/Report.h"

// 前向声明 UI 类（由 uic 工具从 .ui 文件自动生成）
namespace Ui {
class MainWindow;
}

// 前向声明
class ProjectTreeWidget;
class ReportListWidget;
class ReportEditorWindow;
class PluginManager;
class MainWindowDialogs;

/**
 * @brief 主窗口类
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    /**
     * @brief 设置插件管理器（用于插件管理对话框）
     * @param manager 插件管理器指针
     */
    void setPluginManager(PluginManager* manager) { m_pluginManager = manager; }

protected:
    /// 窗口关闭事件（保存窗口状态）
    void closeEvent(QCloseEvent* event) override;

private slots:
    // -----------------------------------------------------------------------
    // 文件菜单
    // -----------------------------------------------------------------------
    void onNewProject();
    void onNewReport();
    void onOpenReport();
    void onImportData();
    void onExportProject();
    void onExit();
    void onLogout();

    // -----------------------------------------------------------------------
    // 编辑菜单
    // -----------------------------------------------------------------------
    void onEditProject();
    void onDeleteProject();
    void onDeleteReport();
    void onFind();

    // -----------------------------------------------------------------------
    // 视图菜单
    // -----------------------------------------------------------------------
    void onToggleProjectPanel(bool visible);
    void onTogglePropertyPanel(bool visible);
    void onToggleFullscreen();
    void onZoomIn();
    void onZoomOut();
    void onResetZoom();

    // -----------------------------------------------------------------------
    // 工具菜单
    // -----------------------------------------------------------------------
    void onTemplateManager();
    void onTagManager();
    void onChangePassword();
    void onDataBackup();
    void onDataRestore();
    void onSettings();

    // -----------------------------------------------------------------------
    // 帮助菜单
    // -----------------------------------------------------------------------
    void onAbout();
    void onAboutQt();
    void onCheckUpdate();
    void onPluginManager();
    void onUserManager();

    // -----------------------------------------------------------------------
    // 项目树信号
    // -----------------------------------------------------------------------
    void onProjectSelected(qint64 projectId);
    void onProjectTreeChanged();

    // -----------------------------------------------------------------------
    // 报告列表信号
    // -----------------------------------------------------------------------
    void onReportOpenRequested(qint64 reportId);
    void onReportNewRequested(qint64 projectId);
    void onReportEditRequested(qint64 reportId);
    void onReportDeleteRequested(qint64 reportId);
    void onReportListChanged();

    // -----------------------------------------------------------------------
    // 报告编辑器窗口信号
    // -----------------------------------------------------------------------
    void onReportEditorSaved(qint64 reportId);
    void onReportEditorClosed(qint64 reportId);

    // -----------------------------------------------------------------------
    // 全局搜索
    // -----------------------------------------------------------------------
    void onGlobalSearch();
    void onGlobalSearchTextChanged(const QString& text);

private:
    // -----------------------------------------------------------------------
    // 初始化方法
    // -----------------------------------------------------------------------
    void createActions();
    void createToolBar();
    void createStatusBar();
    void createDockWidgets();
    void connectSignals();
    void loadSettings();
    void saveSettings();
    void updateWindowTitle();
    void updateActionsState();

    // -----------------------------------------------------------------------
    // 内部方法
    // -----------------------------------------------------------------------

    /// 初始化 UI（动态创建复杂控件）
    void setupUi();

    /// 更新 UI 状态
    void updateReportList();
    void updateProjectTree();

    // -----------------------------------------------------------------------
    // 辅助方法
    // -----------------------------------------------------------------------
    void showStatusMessage(const QString& message, int timeout = 0);
    void updateStatusBar();  ///< 更新状态栏显示
    void updatePropertyPanel();  ///< 更新属性面板显示
    qint64 currentProjectId() const;
    qint64 currentReportId() const;

    // -----------------------------------------------------------------------
    // 权限检查
    // -----------------------------------------------------------------------

    /// 检查当前用户是否有权限修改指定报告（管理员或创建者）
    bool canModifyReport(qint64 reportId) const;

    /// 检查当前用户是否有权限修改指定项目（管理员或创建者）
    bool canModifyProject(qint64 projectId) const;

    /// 显示无权限提示
    void showPermissionDenied() const;

    Ui::MainWindow* ui;                  ///< UI 界面对象（从 .ui 文件自动生成）

    // 中央区域分割器（动态创建）
    QSplitter* m_mainSplitter;           ///< 主分割器（左-中-右）

    // 核心组件
    ProjectTreeWidget* m_projectTree;    ///< 左侧项目树
    ReportListWidget* m_reportList;      ///< 中间报告列表
    QWidget* m_propertyPanel;            ///< 右侧属性面板
    QLabel* m_propertyContentLabel;      ///< 属性面板内容标签

    // 工具栏控件（动态创建）
    QLineEdit* m_globalSearchEdit;       ///< 工具栏全局搜索框

    // 状态栏控件（动态创建）
    QLabel* m_statusUserLabel;           ///< 当前登录用户
    QLabel* m_statusProjectLabel;        ///< 当前项目
    QLabel* m_statusReportLabel;         ///< 当前报告
    QLabel* m_statusCountLabel;          ///< 统计信息

    // -----------------------------------------------------------------------
    // 成员变量 - 动作
    // -----------------------------------------------------------------------

    // 文件菜单
    QAction* m_actionNewProject;
    QAction* m_actionNewReport;
    QAction* m_actionOpenReport;
    QAction* m_actionImportData;
    QAction* m_actionExportReport;
    QAction* m_actionExit;

    // 编辑菜单
    QAction* m_actionEditProject;
    QAction* m_actionDeleteProject;
    QAction* m_actionDeleteReport;
    QAction* m_actionFind;

    // 视图菜单
    QAction* m_actionToggleProjectPanel;
    QAction* m_actionTogglePropertyPanel;
    QAction* m_actionFullscreen;
    QAction* m_actionZoomIn;
    QAction* m_actionZoomOut;
    QAction* m_actionResetZoom;

    // 工具菜单
    QAction* m_actionTemplateManager;
    QAction* m_actionTagManager;       ///< 标签管理
    QAction* m_actionChangePassword;   ///< 修改密码
    QAction* m_actionBackup;
    QAction* m_actionRestore;
    QAction* m_actionSettings;

    // 帮助菜单
    QAction* m_actionAbout;
    QAction* m_actionAboutQt;
    QAction* m_actionCheckUpdate;
    QAction* m_actionPluginManager;
    QAction* m_actionUserManager;  ///< 用户管理（仅管理员可见）

    // -----------------------------------------------------------------------
    // 成员变量 - 数据
    // -----------------------------------------------------------------------

    qint64 m_currentProjectId;  ///< 当前选中的项目 ID
    qint64 m_currentReportId;   ///< 当前打开的报告 ID
    double m_zoomFactor;        ///< 缩放因子
    PluginManager* m_pluginManager;  ///< 插件管理器（用于插件管理对话框）
    MainWindowDialogs* m_dialogs;    ///< 对话框操作控制器

    /// 打开的报告编辑器窗口列表（使用 QPointer 安全管理，自动检测对象是否已删除）
    QList<QPointer<ReportEditorWindow>> m_editorWindows;
};

#endif // MAIN_WINDOW_H
