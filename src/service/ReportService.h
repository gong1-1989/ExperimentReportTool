/**
 * @file ReportService.h
 * @brief 报告业务逻辑服务层
 *
 * 封装报告相关的业务逻辑，解耦 UI 层和数据访问层。
 * UI 层应通过本服务访问报告数据，而非直接调用 Repository。
 */

#ifndef REPORT_SERVICE_H
#define REPORT_SERVICE_H

#include <QList>
#include <QString>
#include <QSharedPointer>

#include "core/models/Report.h"
#include "data/repositories/ReportRepository.h"  // ReportQuery 结构体定义

// 前向声明
class Project;

/**
 * @brief 报告业务服务
 *
 * 提供报告的增删改查、搜索、统计等业务逻辑。
 * 所有方法均为静态方法，无需实例化。
 */
class ReportService
{
public:
    // ========================================================================
    // 基础 CRUD
    // ========================================================================

    /**
     * @brief 根据 ID 获取报告
     * @param id 报告 ID
     * @return 报告对象，不存在返回 nullptr
     */
    static Report::Ptr getById(qint64 id);

    /**
     * @brief 创建新报告
     * @param title 标题
     * @param projectId 所属项目 ID
     * @param author 作者
     * @return 创建的报告对象，失败返回 nullptr
     */
    static Report::Ptr create(const QString& title, qint64 projectId,
                               const QString& author = QString());

    /**
     * @brief 更新报告
     * @param report 报告对象
     * @return 是否成功
     */
    static bool update(const Report::Ptr& report);

    /**
     * @brief 删除报告
     * @param id 报告 ID
     * @return 是否成功
     */
    static bool remove(qint64 id);

    // ========================================================================
    // 查询
    // ========================================================================

    /**
     * @brief 按条件查询报告列表
     * @param query 查询条件
     * @return 报告列表
     */
    static Report::List query(const ReportQuery& query);

    /**
     * @brief 获取项目下的所有报告
     * @param projectId 项目 ID
     * @return 报告列表
     */
    static Report::List listByProject(qint64 projectId);

    /**
     * @brief 搜索报告（仅搜索元数据：标题/作者/标签/状态）
     * @param keyword 关键词
     * @param projectId 项目 ID（-1 表示全部）
     * @return 报告列表
     */
    static Report::List search(const QString& keyword, qint64 projectId = -1);

    // ========================================================================
    // 统计
    // ========================================================================

    /**
     * @brief 获取报告总数
     * @param projectId 项目 ID（-1 表示全部）
     * @return 报告数量
     */
    static int count(qint64 projectId = -1);

    /**
     * @brief 获取指定状态的报告数量
     * @param status 状态
     * @param projectId 项目 ID（-1 表示全部）
     * @return 报告数量
     */
    static int countByStatus(ReportStatus status, qint64 projectId = -1);

    // ========================================================================
    // 业务操作
    // ========================================================================

    /**
     * @brief 复制报告
     * @param id 源报告 ID
     * @param newTitle 新标题（空则自动生成）
     * @return 新报告对象，失败返回 nullptr
     */
    static Report::Ptr duplicate(qint64 id, const QString& newTitle = QString());

    /**
     * @brief 更新报告状态
     * @param id 报告 ID
     * @param status 新状态
     * @return 是否成功
     */
    static bool updateStatus(qint64 id, ReportStatus status);

    /**
     * @brief 更新报告标签
     * @param id 报告 ID
     * @param tagId 标签 ID（0 表示无标签）
     * @return 是否成功
     */
    static bool updateTag(qint64 id, qint64 tagId);

private:
    ReportService() = delete;  ///< 禁止实例化
    ~ReportService() = delete;
};

#endif // REPORT_SERVICE_H
