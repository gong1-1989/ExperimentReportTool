/**
 * @file MainWindowDialogs.h
 * @brief 主窗口对话框操作控制器
 *
 * 自 MainWindow 提取：文件/编辑/工具/帮助菜单的对话框类操作。
 * 权限检查（canModifyReport/canModifyProject）与界面刷新由窗口负责，
 * 控制器通过 Hooks 依赖注入访问窗口能力，保持纯对话框逻辑。
 */

#ifndef MAIN_WINDOW_DIALOGS_H
#define MAIN_WINDOW_DIALOGS_H

#include <QString>
#include <functional>

class QWidget;

/**
 * @brief 主窗口对话框操作控制器
 */
class MainWindowDialogs
{
public:
    /// 窗口能力钩子（由 MainWindow 注入 lambda，捕获 this 访问私有成员）
    struct Hooks {
        std::function<qint64()> currentProjectId;    ///< 当前选中项目
        std::function<qint64()> currentReportId;     ///< 当前选中报告
        std::function<void(qint64)> selectProject;   ///< 项目树选中
        std::function<void(qint64)> setReportListProjectId;  ///< 设置报告列表项目过滤（-1=全部）
        std::function<void()> refreshProjectTree;    ///< 刷新项目树
        std::function<void()> refreshReportList;     ///< 刷新报告列表
        std::function<void()> updatePropertyPanel;   ///< 刷新属性面板
        std::function<void()> updateStatusBar;       ///< 刷新状态栏
        std::function<void()> updateActionsState;    ///< 刷新动作状态
        std::function<void(const QString&)> showStatus;  ///< 状态栏消息
        std::function<bool()> closeAllEditorWindows; ///< 关闭全部编辑器窗口（数据恢复）；返回是否全部成功
    };

    explicit MainWindowDialogs(QWidget* parent, Hooks hooks);

    // 文件菜单
    void newProject();
    void newReport();
    void importData();
    void exportProject();
    // 编辑菜单
    void editProject();
    /// 删除项目（调用方需已通过权限检查）
    void deleteProject(qint64 projectId);
    /// 删除报告（调用方需已通过权限检查）
    void deleteReport(qint64 reportId);
    // 工具菜单
    void templateManager();
    void tagManager();
    void changePassword();
    void dataBackup();
    void dataRestore();
    void settings();
    // 帮助菜单
    void about();
    void aboutQt();
    void checkUpdate();
    void userManager();

private:
    QWidget* m_parent;   ///< 对话框父窗口
    Hooks m_hooks;       ///< 窗口能力钩子
};

#endif // MAIN_WINDOW_DIALOGS_H
