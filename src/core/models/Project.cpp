/**
 * @file Project.cpp
 * @brief 实验项目实体类实现文件
 */

#include "Project.h"
#include "core/utils/AppConstants.h"

#include <QStringList>

// ===========================================================================
// 工厂方法
// ===========================================================================

Project::Ptr Project::create()
{
    // 使用 QSharedPointer 管理生命周期
    // 自定义删除器不是必需的，默认 delete 即可
    return Ptr(new Project());
}

// ===========================================================================
// 构造与析构
// ===========================================================================

Project::Project()
    : m_id(-1)                    // 未保存到数据库时 ID 为 -1
    , m_status(ProjectStatus::Active)  // 新项目默认为进行中
    , m_parentId(-1)              // 默认为根项目
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
{
    // 构造函数中初始化默认值
    // 名称、类型等留空，由调用方设置
}

Project::~Project()
{
    // 无需手动释放资源，成员变量都是值类型
}

// ===========================================================================
// 状态转换方法
// ===========================================================================

QString Project::statusDisplayName() const
{
    switch (m_status) {
    case ProjectStatus::Active:    return QStringLiteral("进行中");
    case ProjectStatus::Completed: return QStringLiteral("已完成");
    case ProjectStatus::Archived:  return QStringLiteral("已归档");
    }
    return QStringLiteral("未知");
}

QString Project::statusToString() const
{
    switch (m_status) {
    case ProjectStatus::Active:    return AppConstants::PROJECT_STATUS_ACTIVE;
    case ProjectStatus::Completed: return AppConstants::PROJECT_STATUS_COMPLETED;
    case ProjectStatus::Archived:  return AppConstants::PROJECT_STATUS_ARCHIVED;
    }
    return AppConstants::PROJECT_STATUS_ACTIVE;
}

ProjectStatus Project::statusFromString(const QString& str)
{
    if (str == AppConstants::PROJECT_STATUS_COMPLETED) {
        return ProjectStatus::Completed;
    }
    if (str == AppConstants::PROJECT_STATUS_ARCHIVED) {
        return ProjectStatus::Archived;
    }
    // 默认返回 Active（包括空字符串和未知值）
    return ProjectStatus::Active;
}

QStringList Project::allStatusDisplayNames()
{
    return QStringList{
        QStringLiteral("进行中"),
        QStringLiteral("已完成"),
        QStringLiteral("已归档")
    };
}

// ===========================================================================
// 调试输出
// ===========================================================================

QString Project::toString() const
{
    return QString("Project(id=%1, name='%2', type='%3', status=%4, parent=%5)")
        .arg(m_id)
        .arg(m_name)
        .arg(m_type)
        .arg(statusToString())
        .arg(m_parentId);
}
