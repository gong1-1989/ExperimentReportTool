/**
 * @file ProjectTreeWidget.h
 * @brief 项目树组件头文件
 *
 * 左侧栏的项目树，以树状结构展示所有项目。
 * 支持：
 * - 项目的增删改查（通过右键菜单）
 * - 拖拽调整层级（后续实现）
 * - 点击项目时发出信号，通知主窗口刷新报告列表
 */

#ifndef PROJECT_TREE_WIDGET_H
#define PROJECT_TREE_WIDGET_H

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QAction>
#include <QMenu>

#include "core/models/Project.h"

/**
 * @brief 项目树组件
 *
 * 使用 QTreeWidget 展示项目的树状结构。
 * 每个 QTreeWidgetItem 的 Data(0, Qt::UserRole) 存储项目 ID。
 */
class ProjectTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit ProjectTreeWidget(QWidget* parent = nullptr);

    /**
     * @brief 刷新项目树（从数据库重新加载）
     */
    void refreshTree();

    /**
     * @brief 获取当前选中的项目 ID
     * @return 项目 ID，未选中返回 -1
     */
    qint64 currentProjectId() const;

    /**
     * @brief 选中指定项目
     * @param projectId 项目 ID
     */
    void selectProject(qint64 projectId);

signals:
    /**
     * @brief 项目选中信号
     * @param projectId 选中的项目 ID
     */
    void projectSelected(qint64 projectId);

    /**
     * @brief 项目树变化信号（增删改后发出）
     */
    void projectTreeChanged();

private slots:
    /// 处理树节点点击
    void onItemClicked(QTreeWidgetItem* item, int column);
    /// 处理右键菜单
    void onCustomContextMenu(const QPoint& pos);
    /// 新建项目
    void onNewProject();
    /// 新建子项目
    void onNewSubProject();
    /// 编辑项目
    void onEditProject();
    /// 删除项目
    void onDeleteProject();
    /// 展开/折叠所有
    void onExpandAll();
    void onCollapseAll();

private:
    /**
     * @brief 递归构建项目树
     * @param parentItem 父节点（nullptr 表示根节点）
     * @param parentId 父项目 ID（-1 表示根项目）
     */
    void buildTree(QTreeWidgetItem* parentItem, qint64 parentId);

    /**
     * @brief 根据项目 ID 查找树节点
     * @param projectId 项目 ID
     * @return 树节点，未找到返回 nullptr
     */
    QTreeWidgetItem* findItemByProjectId(qint64 projectId);

    /**
     * @brief 递归查找节点
     * @param item 起始节点
     * @param projectId 项目 ID
     * @return 匹配的节点
     */
    QTreeWidgetItem* findItemRecursive(QTreeWidgetItem* item, qint64 projectId);

    /**
     * @brief 创建项目项（设置图标、文字等）
     * @param project 项目对象
     * @return 树节点
     */
    QTreeWidgetItem* createProjectItem(const Project::Ptr& project);

    // -----------------------------------------------------------------------
    // 成员变量
    // -----------------------------------------------------------------------

    QAction* m_actionNewProject;     ///< 新建项目
    QAction* m_actionNewSubProject;  ///< 新建子项目
    QAction* m_actionEditProject;    ///< 编辑项目
    QAction* m_actionDeleteProject;  ///< 删除项目
    QAction* m_actionExpandAll;      ///< 展开所有
    QAction* m_actionCollapseAll;    ///< 折叠所有

    qint64 m_contextProjectId;  ///< 右键菜单时的项目 ID
};

#endif // PROJECT_TREE_WIDGET_H
