/**
 * @file WorkflowService.h
 * @brief 报告工作流服务头文件
 *
 * 管理报告的状态流转：草稿→已提交→已审核→已审批。
 * 每个操作都记录 last_action（操作类型+操作人+时间+意见）。
 * 退回时必须填写意见（非空校验），通过时可选填。
 *
 * 状态流转：
 * - 提交（submit）：Draft → Submitted（创建者本人）
 * - 审核通过（reviewApprove）：Submitted → Reviewed（组长）
 * - 审核退回（reviewReject）：Submitted → Draft（组长，意见必填）
 * - 审批通过（approveApprove）：Reviewed → Approved（总管）
 * - 审批退回（approveReject）：Reviewed → Draft（总管，意见必填）
 */

#ifndef WORKFLOW_SERVICE_H
#define WORKFLOW_SERVICE_H

#include <QString>
#include "core/models/Report.h"
#include "core/models/User.h"

class WorkflowService
{
public:
    /// 操作结果
    struct Result {
        bool success = false;
        QString errorMessage;
    };

    // -----------------------------------------------------------------------
    // 工作流操作
    // -----------------------------------------------------------------------

    /// 提交报告（Draft→Submitted）
    static Result submit(qint64 reportId, qint64 userId);

    /// 审核通过（Submitted→Reviewed），comment 可选
    static Result reviewApprove(qint64 reportId, qint64 userId, const QString& comment = QString());

    /// 审核退回（Submitted→Draft），comment 必填（非空校验）
    static Result reviewReject(qint64 reportId, qint64 userId, const QString& comment);

    /// 审批通过（Reviewed→Approved），comment 可选
    static Result approveApprove(qint64 reportId, qint64 userId, const QString& comment = QString());

    /// 审批退回（Reviewed→Draft），comment 必填
    static Result approveReject(qint64 reportId, qint64 userId, const QString& comment);

    /** 撤回提交：创建者主动收回（Submitted→Draft，无需意见） */
    static Result recall(qint64 reportId, qint64 userId);

private:
    /// 通用状态流转+last_action 记录
    static Result transition(qint64 reportId, qint64 userId,
                             ReportStatus fromStatus, ReportStatus toStatus,
                             const QString& actionName, const QString& comment,
                             bool commentRequired);
};

#endif // WORKFLOW_SERVICE_H
