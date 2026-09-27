/**
 * @file ProjectTreeWidget.cpp
 * @brief 项目树组件实现文件
 */

#include "ProjectTreeWidget.h"
#include "data/repositories/ProjectRepository.h"
#include "data/repositories/ReportRepository.h"
#include "data/repositories/UserRepository.h"
#include "core/utils/Logger.h"
#include "core/utils/UserSession.h"
#include "ui/dialogs/ProjectDialog.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QIcon>

// ===========================================================================
// 构造函数
// ===========================================================================

ProjectTreeWidget::ProjectTreeWidget(QWidget* parent)
    : QTreeWidget(parent)
    , m_contextProjectId(-1)
{
    // 设置列标题
    setHeaderLabel(tr("实验项目"));
    header()->setStretchLastSection(true);
    header()->setVisible(true);

    // 设置样式
    setRootIsDecorated(true);       // 显示展开/折叠箭头
    setAlternatingRowColors(true);   // 交替行颜色
    setAnimated(true);               // 展开/折叠动画
    setIndentation(16);              // 缩进宽度

    // 启用右键菜单
    setContextMenuPolicy(Qt::CustomContextMenu);

    // -----------------------------------------------------------------------
    // 创建动作
    // -----------------------------------------------------------------------

    m_actionNewProject = new QAction(tr("新建项目"), this);
    m_actionNewProject->setShortcut(QKeySequence("Ctrl+Shift+N"));
    connect(m_actionNewProject, &QAction::triggered, this, &ProjectTreeWidget::onNewProject);

    m_actionNewSubProject = new QAction(tr("新建子项目"), this);
    connect(m_actionNewSubProject, &QAction::triggered, this, &ProjectTreeWidget::onNewSubProject);

    m_actionEditProject = new QAction(tr("编辑项目"), this);
    m_actionEditProject->setShortcut(QKeySequence("F2"));
    connect(m_actionEditProject, &QAction::triggered, this, &ProjectTreeWidget::onEditProject);

    m_actionDeleteProject = new QAction(tr("删除项目"), this);
    m_actionDeleteProject->setShortcut(QKeySequence("Delete"));
    connect(m_actionDeleteProject, &QAction::triggered, this, &ProjectTreeWidget::onDeleteProject);

    m_actionExpandAll = new QAction(tr("全部展开"), this);
    connect(m_actionExpandAll, &QAction::triggered, this, &QTreeWidget::expandAll);

    m_actionCollapseAll = new QAction(tr("全部折叠"), this);
    connect(m_actionCollapseAll, &QAction::triggered, this, &QTreeWidget::collapseAll);

    // -----------------------------------------------------------------------
    // 连接信号
    // -----------------------------------------------------------------------

    connect(this, &QTreeWidget::itemClicked,
            this, &ProjectTreeWidget::onItemClicked);
    connect(this, &QTreeWidget::customContextMenuRequested,
            this, &ProjectTreeWidget::onCustomContextMenu);

    // 初始加载
    refreshTree();
}

// ===========================================================================
// 公共方法
// ===========================================================================

void ProjectTreeWidget::refreshTree()
{
    clear();
    m_userNodeCache.clear();

    // 获取所有根项目，按创建者分组
    Project::List rootProjects = ProjectRepository::findChildren(-1);

    // 按 created_by 分组
    QMap<qint64, Project::List> grouped;
    for (const Project::Ptr& project : rootProjects) {
        grouped[project->createdBy()].append(project);
    }

    // 按用户名排序显示
    QList<qint64> userIds = grouped.keys();
    // 排序：已分配用户在前，未分配(-1)在后
    std::sort(userIds.begin(), userIds.end(), [](qint64 a, qint64 b) {
        if (a == -1) return false;
        if (b == -1) return true;
        return a < b;
    });

    for (qint64 userId : userIds) {
        QTreeWidgetItem* userNode = createUserNode(userId);
        if (!userNode) continue;

        for (const Project::Ptr& project : grouped[userId]) {
            QTreeWidgetItem* item = createProjectItem(project);
            userNode->addChild(item);
            // 递归构建子项目
            buildTree(item, project->id());
        }
    }

    // 默认展开第一层
    expandToDepth(0);
}

qint64 ProjectTreeWidget::currentProjectId() const
{
    QTreeWidgetItem* item = currentItem();
    if (!item) return -1;
    return item->data(0, Qt::UserRole).toLongLong();
}

void ProjectTreeWidget::selectProject(qint64 projectId)
{
    QTreeWidgetItem* item = findItemByProjectId(projectId);
    if (item) {
        setCurrentItem(item);
        // 确保父节点都展开
        QTreeWidgetItem* parent = item->parent();
        while (parent) {
            parent->setExpanded(true);
            parent = parent->parent();
        }
    }
}

// ===========================================================================
// 私有槽函数
// ===========================================================================

void ProjectTreeWidget::onItemClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    if (!item) return;

    const qint64 projectId = item->data(0, Qt::UserRole).toLongLong();
    // 用户分组节点（-2）不发出项目选中信号
    if (projectId != -2) {
        emit projectSelected(projectId);
    }
}

void ProjectTreeWidget::onCustomContextMenu(const QPoint& pos)
{
    QTreeWidgetItem* item = itemAt(pos);
    m_contextProjectId = item ? item->data(0, Qt::UserRole).toLongLong() : -1;

    QMenu menu(this);

    // 总是可用的操作
    menu.addAction(m_actionNewProject);

    // 选中有效项目时的操作
    if (m_contextProjectId > 0) {
        menu.addAction(m_actionNewSubProject);
        menu.addAction(m_actionEditProject);
        menu.addSeparator();
        menu.addAction(m_actionDeleteProject);
    }

    menu.addSeparator();
    menu.addAction(m_actionExpandAll);
    menu.addAction(m_actionCollapseAll);

    menu.exec(viewport()->mapToGlobal(pos));
}

void ProjectTreeWidget::onNewProject()
{
    ProjectDialog dialog(this);
    dialog.setWindowTitle(tr("新建项目"));

    if (dialog.exec() == QDialog::Accepted) {
        Project::Ptr project = dialog.projectData();
        // 自动设置创建者为当前登录用户
        project->setCreatedBy(UserSession::instance().userId());
        if (ProjectRepository::insert(project)) {
            refreshTree();
            selectProject(project->id());
            emit projectTreeChanged();
        } else {
            QMessageBox::critical(this, tr("错误"), tr("创建项目失败"));
        }
    }
}

void ProjectTreeWidget::onNewSubProject()
{
    if (m_contextProjectId <= 0) return;

    ProjectDialog dialog(this);
    dialog.setWindowTitle(tr("新建子项目"));
    dialog.setParentProjectId(m_contextProjectId);

    if (dialog.exec() == QDialog::Accepted) {
        Project::Ptr project = dialog.projectData();
        project->setParentId(m_contextProjectId);
        // 自动设置创建者为当前登录用户
        project->setCreatedBy(UserSession::instance().userId());
        if (ProjectRepository::insert(project)) {
            refreshTree();
            selectProject(project->id());
            emit projectTreeChanged();
        } else {
            QMessageBox::critical(this, tr("错误"), tr("创建子项目失败"));
        }
    }
}

void ProjectTreeWidget::onEditProject()
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
            refreshTree();
            selectProject(projectId);
            emit projectTreeChanged();
        } else {
            QMessageBox::critical(this, tr("错误"), tr("更新项目失败"));
        }
    }
}

void ProjectTreeWidget::onDeleteProject()
{
    const qint64 projectId = m_contextProjectId > 0 ? m_contextProjectId : currentProjectId();
    if (projectId <= 0) return;

    Project::Ptr project = ProjectRepository::findById(projectId);
    if (!project) return;

    // 检查是否有子项目
    const int childCount = ProjectRepository::countChildren(projectId);
    const int reportCount = ReportRepository::countByProject(projectId);

    QString warningText = tr("确定要删除项目「%1」吗？").arg(project->name());
    if (childCount > 0 || reportCount > 0) {
        warningText += tr("\n\n该项目包含 %1 个子项目和 %2 份报告，删除后将无法恢复！")
                          .arg(childCount).arg(reportCount);
    }

    const auto ret = QMessageBox::warning(
        this, tr("确认删除"), warningText,
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        if (ProjectRepository::remove(projectId)) {
            refreshTree();
            emit projectTreeChanged();
        } else {
            QMessageBox::critical(this, tr("错误"), tr("删除项目失败"));
        }
    }
}

void ProjectTreeWidget::onExpandAll()
{
    expandAll();
}

void ProjectTreeWidget::onCollapseAll()
{
    collapseAll();
}

// ===========================================================================
// 私有方法
// ===========================================================================

void ProjectTreeWidget::buildTree(QTreeWidgetItem* parentItem, qint64 parentId)
{
    Project::List children = ProjectRepository::findChildren(parentId);

    for (const Project::Ptr& project : children) {
        QTreeWidgetItem* item = createProjectItem(project);
        parentItem->addChild(item);
        // 递归构建子项目
        buildTree(item, project->id());
    }
}

QTreeWidgetItem* ProjectTreeWidget::createUserNode(qint64 userId)
{
    QTreeWidgetItem* node = new QTreeWidgetItem();
    node->setData(0, Qt::UserRole, -2);  // -2 表示用户分组节点，不是项目

    if (userId <= 0) {
        // 未分配（旧数据）
        node->setText(0, tr("未分配"));
        node->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
        node->setToolTip(0, tr("未分配创建者的项目（旧数据）"));
    } else {
        User::Ptr user = UserRepository::findById(userId);
        if (user) {
            node->setText(0, user->displayNameOrUsername());
            node->setToolTip(0, QString(tr("用户名: %1\n角色: %2"))
                .arg(user->username())
                .arg(user->roleName()));
        } else {
            node->setText(0, tr("未知用户(%1)").arg(userId));
        }
        node->setIcon(0, style()->standardIcon(QStyle::SP_DirHomeIcon));
    }

    addTopLevelItem(node);
    return node;
}

QTreeWidgetItem* ProjectTreeWidget::createProjectItem(const Project::Ptr& project)
{
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, project->name());
    item->setData(0, Qt::UserRole, project->id());
    item->setToolTip(0, QString("%1\n类型: %2\n状态: %3")
                           .arg(project->name())
                           .arg(project->type())
                           .arg(project->statusDisplayName()));

    // 根据状态设置图标
    QIcon icon;
    switch (project->status()) {
    case ProjectStatus::Active:
        icon = style()->standardIcon(QStyle::SP_DirOpenIcon);
        break;
    case ProjectStatus::Completed:
        icon = style()->standardIcon(QStyle::SP_DialogApplyButton);
        break;
    case ProjectStatus::Archived:
        icon = style()->standardIcon(QStyle::SP_DirClosedIcon);
        break;
    }
    item->setIcon(0, icon);

    return item;
}

QTreeWidgetItem* ProjectTreeWidget::findItemByProjectId(qint64 projectId)
{
    // 遍历所有顶层节点
    for (int i = 0; i < topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = findItemRecursive(topLevelItem(i), projectId);
        if (item) return item;
    }
    return nullptr;
}

QTreeWidgetItem* ProjectTreeWidget::findItemRecursive(QTreeWidgetItem* item, qint64 projectId)
{
    if (!item) return nullptr;

    if (item->data(0, Qt::UserRole).toLongLong() == projectId) {
        return item;
    }

    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem* found = findItemRecursive(item->child(i), projectId);
        if (found) return found;
    }

    return nullptr;
}
