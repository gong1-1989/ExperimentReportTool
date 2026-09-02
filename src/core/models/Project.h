/**
 * @file Project.h
 * @brief 实验项目实体类头文件
 *
 * Project 表示一个实验项目，是报告的顶层容器。
 * 支持树状结构（parent_id），可包含子项目和多个报告。
 */

#ifndef PROJECT_H
#define PROJECT_H

#include <QString>
#include <QDateTime>
#include <QList>
#include <QSharedPointer>

/**
 * @brief 项目状态枚举
 */
enum class ProjectStatus {
    Active,     ///< 进行中
    Completed,  ///< 已完成
    Archived    ///< 已归档
};

/**
 * @brief 实验项目实体类
 *
 * 对应数据库中的 projects 表。
 * 使用 QSharedPointer 管理生命周期，避免拷贝开销。
 *
 * 使用示例：
 * @code
 *   auto project = Project::create();
 *   project->setName("物理实验 - 牛顿第二定律验证");
 *   project->setType("物理");
 *   project->setStatus(ProjectStatus::Active);
 * @endcode
 */
class Project
{
public:
    /// 智能指针类型别名
    using Ptr = QSharedPointer<Project>;
    using List = QList<Ptr>;

    /**
     * @brief 创建一个新的 Project 实例（工厂方法）
     * @return 指向新项目的智能指针
     *
     * 使用工厂方法确保所有对象都通过智能指针管理。
     * 新创建的对象 id 为 -1，表示尚未保存到数据库。
     */
    static Ptr create();

    // -----------------------------------------------------------------------
    // 构造与析构
    // -----------------------------------------------------------------------

    /// 默认构造函数
    Project();

    /// 析构函数
    ~Project();

    // -----------------------------------------------------------------------
    // 属性访问器（Getter / Setter）
    // -----------------------------------------------------------------------

    /// 获取项目 ID（数据库主键，未保存时为 -1）
    qint64 id() const { return m_id; }
    /// 设置项目 ID（通常由数据库层在插入后设置）
    void setId(qint64 id) { m_id = id; }

    /// 获取项目名称
    QString name() const { return m_name; }
    /// 设置项目名称
    void setName(const QString& name) { m_name = name; }

    /// 获取项目类型（如"物理"、"化学"、"生物"等）
    QString type() const { return m_type; }
    /// 设置项目类型
    void setType(const QString& type) { m_type = type; }

    /// 获取项目描述
    QString description() const { return m_description; }
    /// 设置项目描述
    void setDescription(const QString& description) { m_description = description; }

    /// 获取项目状态
    ProjectStatus status() const { return m_status; }
    /// 设置项目状态
    void setStatus(ProjectStatus status) { m_status = status; }

    /// 获取负责人
    QString owner() const { return m_owner; }
    /// 设置负责人
    void setOwner(const QString& owner) { m_owner = owner; }

    /// 获取父项目 ID（根项目为 -1）
    qint64 parentId() const { return m_parentId; }
    /// 设置父项目 ID
    void setParentId(qint64 parentId) { m_parentId = parentId; }

    /// 获取创建时间
    QDateTime createdAt() const { return m_createdAt; }
    /// 设置创建时间（通常由数据库层设置）
    void setCreatedAt(const QDateTime& createdAt) { m_createdAt = createdAt; }

    /// 获取最后更新时间
    QDateTime updatedAt() const { return m_updatedAt; }
    /// 设置最后更新时间
    void setUpdatedAt(const QDateTime& updatedAt) { m_updatedAt = updatedAt; }

    // -----------------------------------------------------------------------
    // 便利方法
    // -----------------------------------------------------------------------

    /**
     * @brief 判断是否为根项目（无父项目）
     * @return 是根项目返回 true
     */
    bool isRoot() const { return m_parentId <= 0; }

    /**
     * @brief 判断项目是否已保存到数据库
     * @return 已保存返回 true
     */
    bool isPersisted() const { return m_id > 0; }

    /**
     * @brief 获取状态的字符串表示（用于显示）
     * @return 状态字符串，如"进行中"
     */
    QString statusDisplayName() const;

    /**
     * @brief 将状态转换为数据库存储字符串
     * @return 状态字符串，如"active"
     */
    QString statusToString() const;

    /**
     * @brief 从字符串解析状态
     * @param str 状态字符串（"active"/"completed"/"archived"）
     * @return 对应的 ProjectStatus 枚举值
     */
    static ProjectStatus statusFromString(const QString& str);

    /**
     * @brief 获取所有可用状态的显示名称列表
     * @return 状态显示名称列表（用于下拉框等）
     */
    static QStringList allStatusDisplayNames();

    /**
     * @brief 简单的字符串表示（用于调试输出）
     * @return 项目信息字符串
     */
    QString toString() const;

private:
    // -----------------------------------------------------------------------
    // 数据成员（对应数据库字段）
    // -----------------------------------------------------------------------

    qint64       m_id;           ///< 主键 ID
    QString      m_name;         ///< 项目名称
    QString      m_type;         ///< 项目类型
    QString      m_description;  ///< 项目描述
    ProjectStatus m_status;      ///< 项目状态
    QString      m_owner;        ///< 负责人
    qint64       m_parentId;     ///< 父项目 ID（-1 表示根）
    QDateTime    m_createdAt;    ///< 创建时间
    QDateTime    m_updatedAt;    ///< 最后更新时间
};

#endif // PROJECT_H
