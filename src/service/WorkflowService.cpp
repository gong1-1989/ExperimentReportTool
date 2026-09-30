/**
 * @file WorkflowService.cpp
 * @brief 报告工作流服务实现
 */

#include "WorkflowService.h"
#include <QObject>
#include "PermissionService.h"
#include "AuditService.h"
#include "data/repositories/ReportRepository.h"
#include "data/repositories/UserRepository.h"
#include "core/utils/Logger.h"

WorkflowService::Result WorkflowService::submit(qint64 reportId, qint64 userId)
{
    return transition(reportId, userId, ReportStatus::Draft, ReportStatus::Submitted,
                      "submit", QString(), false);
}

WorkflowService::Result WorkflowService::reviewApprove(qint64 reportId, qint64 userId, const QString& comment)
{
    return transition(reportId, userId, ReportStatus::Submitted, ReportStatus::Reviewed,
                      "review_approve", comment, false);
}

WorkflowService::Result WorkflowService::reviewReject(qint64 reportId, qint64 userId, const QString& comment)
{
    return transition(reportId, userId, ReportStatus::Submitted, ReportStatus::Draft,
                      "review_reject", comment, true);
}

WorkflowService::Result WorkflowService::approveApprove(qint64 reportId, qint64 userId, const QString& comment)
{
    return transition(reportId, userId, ReportStatus::Reviewed, ReportStatus::Approved,
                      "approve_approve", comment, false);
}

WorkflowService::Result WorkflowService::approveReject(qint64 reportId, qint64 userId, const QString& comment)
{
    return transition(reportId, userId, ReportStatus::Reviewed, ReportStatus::Draft,
                      "approve_reject", comment, true);
}

WorkflowService::Result WorkflowService::recall(qint64 reportId, qint64 userId)
{
    return transition(reportId, userId, ReportStatus::Submitted, ReportStatus::Draft,
                      "recall", QString(), false);
}

WorkflowService::Result WorkflowService::transition(qint64 reportId, qint64 userId,
                                                       ReportStatus fromStatus, ReportStatus toStatus,
                                                       const QString& actionName, const QString& comment,
                                                       bool commentRequired)
{
    Result result;

    // 1. 校验报告存在
    Report::Ptr report = ReportRepository::findById(reportId);
    if (!report) {
        result.errorMessage = "报告不存在";
        return result;
    }

    // 2. 校验状态
    if (report->status() == ReportStatus::Archived) {
        result.errorMessage = "报告已归档，不能进行状态流转";
        return result;
    }
    if (report->status() != fromStatus) {
        result.errorMessage = QString("报告状态不匹配（当前为%1，无法执行此操作）").arg(report->statusDisplayName());
        return result;
    }

    // 3. 校验用户存在
    User::Ptr user = UserRepository::findById(userId);
    if (!user) {
        result.errorMessage = "用户不存在";
        return result;
    }
    if (user->isDisabled()) {
        result.errorMessage = "账户已被禁用";
        return result;
    }

    // 4. 校验权限（根据操作类型）
    bool hasPermission = false;
    if (actionName == "submit") {
        hasPermission = PermissionService::canSubmitReport(report, user);
    } else if (actionName.startsWith("review_")) {
        hasPermission = PermissionService::canReviewReport(report, user);
    } else if (actionName.startsWith("approve_")) {
        hasPermission = PermissionService::canApproveReport(report, user);
    } else if (actionName == "recall") {
        hasPermission = PermissionService::canRecallReport(report, user);
    }
    if (!hasPermission) {
        result.errorMessage = "没有权限执行此操作";
        return result;
    }

    // 5. 退回意见必填校验
    if (commentRequired && comment.trimmed().isEmpty()) {
        result.errorMessage = "退回时必须填写意见";
        return result;
    }

    // 6. 执行状态流转 + last_action 记录
    report->setStatus(toStatus);
    report->setLastAction(actionName);
    report->setLastActionBy(userId);
    report->setLastActionAt(QDateTime::currentDateTime());
    report->setLastActionComment(comment.trimmed());
    report->setModifiedBy(userId);

    if (!ReportRepository::update(report)) {
        result.errorMessage = "保存失败";
        return result;
    }

    LOG_INFO(QString("工作流操作: report=%1, action=%2, by=%3")
                 .arg(reportId).arg(actionName).arg(userId));

    // 审计留痕：动作名转中文
    QString actionText = actionName;
    if (actionName == "submit")               actionText = QObject::tr("提交审核");
    else if (actionName == "review_approve")  actionText = QObject::tr("审核通过");
    else if (actionName == "review_reject")   actionText = QObject::tr("审核退回");
    else if (actionName == "approve_approve") actionText = QObject::tr("审批通过");
    else if (actionName == "approve_reject")  actionText = QObject::tr("审批退回");
    else if (actionName == "recall")          actionText = QObject::tr("撤回");
    else if (actionName == "archive")         actionText = QObject::tr("归档");
    AuditService::log(actionText,
                      report->title() + (comment.trimmed().isEmpty()
                                             ? QString()
                                             : QString(" | %1").arg(comment.trimmed())),
                      reportId);

    result.success = true;
    return result;
}
