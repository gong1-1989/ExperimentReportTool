/**
 * @file ProjectRepository.h
 * @brief 项目仓储类头文件
 *
 * 仓储模式（Repository Pattern）：封装数据访问逻辑，
 * 上层业务代码不直接操作 SQL，而是通过仓储类进行 CRUD 操作。
 *
 * ProjectRepository 负责 projects 表的所有数据操作。
 */

#ifndef PROJECT_REPOSITORY_H
#define PROJECT_REPOSITORY_H

#include <QList>
#include <QString>
#include <QDateTime>
#include <QSqlQuery>

#include "core/models/Project.h"

/**
 * @brief 项目查询条件结构体
 *
 * 用于 findAll() 的高级筛选。
 * 所有字段都是可选的，留空表示不筛选该条件。
 */
struct ProjectQuery {
    QString keyword;       ///< 名称/描述关键词模糊匹配
    QString type;          ///< 项目类型精确匹配
    ProjectStatus status;  ///< 状态筛选（默认所有状态）
    qint64 parentId;       ///< 父项目 ID（-1 表示所有，0 表示仅根项目）
    QString owner;         ///< 负责人
    QDateTime dateFrom;    ///< 创建时间起始
    QDateTime dateTo;      ///< 创建时间截止
    QString sortBy;        ///< 排序字段（name/created_at/updated_at）
    Qt::SortOrder sortOrder; ///< 排序方向

    ProjectQuery()
        : status(static_cast<ProjectStatus>(-1))  // -1 表示不筛选状态
        , parentId(-1)
        , sortBy("updated_at")
        , sortOrder(Qt::DescendingOrder)
    {}
};

/**
 * @brief 项目仓储类
 *
 * 所有方法都是静态的，因为数据库连接是全局共享的。
 * 如果未来需要支持多数据库实例，可以改为实例方法并注入连接。
 */
class ProjectRepository
{
public:
    // -----------------------------------------------------------------------
    // CRUD 操作
    // -----------------------------------------------------------------------

    /**
     * @brief 根据 ID 查找项目
     * @param id 项目 ID
     * @return 项目指针，未找到返回 nullptr
     */
    static Project::Ptr findById(qint64 id);

    /**
     * @brief 查找所有项目（支持筛选和排序）
     * @param query 查询条件
     * @return 项目列表
     */
    static Project::List findAll(const ProjectQuery& query = ProjectQuery());

    /**
     * @brief 查找根项目（parent_id <= 0）
     * @return 根项目列表
     */
    static Project::List findRootProjects();

    /**
     * @brief 查找指定项目的子项目
     * @param parentId 父项目 ID
     * @return 子项目列表
     */
    static Project::List findChildren(qint64 parentId);

    /**
     * @brief 插入新项目
     * @param project 项目对象（id 会被设置为数据库生成的 ID）
     * @return 成功返回 true，失败返回 false
     *
     * 插入成功后，project->id() 会被更新为数据库自增 ID。
     */
    static bool insert(Project::Ptr project);

    /**
     * @brief 更新项目
     * @param project 项目对象（必须有有效的 id）
     * @return 成功返回 true
     */
    static bool update(const Project::Ptr& project);

    /**
     * @brief 删除项目
     * @param id 项目 ID
     * @return 成功返回 true
     *
     * 注意：由于外键设置了 ON DELETE CASCADE，
     * 删除项目会同时删除其下的所有报告。
     */
    static bool remove(qint64 id);

    /**
     * @brief 检查项目名称是否已存在
     * @param name 项目名称
     * @param excludeId 排除的项目 ID（用于编辑时排除自身）
     * @return 存在返回 true
     */
    static bool existsByName(const QString& name, qint64 excludeId = -1);

    // -----------------------------------------------------------------------
    // 统计操作
    // -----------------------------------------------------------------------

    /**
     * @brief 获取项目总数
     * @return 项目数量
     */
    static int count();

    /**
     * @brief 获取指定状态的项目数量
     * @param status 项目状态
     * @return 数量
     */
    static int countByStatus(ProjectStatus status);

    /**
     * @brief 获取指定项目的子项目数量
     * @param parentId 父项目 ID
     * @return 子项目数量
     */
    static int countChildren(qint64 parentId);

    // -----------------------------------------------------------------------
    // 树状结构操作
    // -----------------------------------------------------------------------

    /**
     * @brief 获取项目的完整路径（从根到当前项目的名称链）
     * @param id 项目 ID
     * @return 路径字符串，如 "物理实验/力学/牛顿第二定律"
     */
    static QString getPath(qint64 id);

    /**
     * @brief 获取所有后代项目 ID（递归）
     * @param rootId 根项目 ID
     * @return 后代 ID 列表（包括 rootId 自身）
     */
    static QList<qint64> getAllDescendantIds(qint64 rootId);

private:
    /**
     * @brief 将 SQL 查询结果映射为 Project 对象
     * @param query SQL 查询对象（已定位到有效行）
     * @return Project 指针
     */
    static Project::Ptr mapToProject(const QSqlQuery& query);
};

#endif // PROJECT_REPOSITORY_H
