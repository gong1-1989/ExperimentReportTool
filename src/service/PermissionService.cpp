/**
 * @file PermissionService.cpp
 * @brief 权限服务实现
 */

#include "PermissionService.h"
#include "data/repositories/UserRepository.h"

// ===========================================================================
// 报告可见性（优化1）
// ===========================================================================

bool PermissionService::canViewReport(const Report::Ptr& report, const User::Ptr& user)
{
    if (!report || !user) return false;
    // 超管/总管可看所有
    if (user->isSuperAdmin() || user->isManager()) return true;
    // 草稿：仅创建者+本组组长可见
    if (report->status() == ReportStatus::Draft) {
        if (report->createdBy() == user->id()) return true;
        if (user->isLeader() && user->groupId() > 0) {
            // 组长能看本组组员的草稿：需要查创建者的组
            User::Ptr creator = UserRepository::findById(report->createdBy());
            if (creator && creator->groupId() == user->groupId()) return true;
        }
        return false;
    }
    // 已提交及以上：本组可见（组长/组员）
    if (user->groupId() > 0) {
        User::Ptr creator = UserRepository::findById(report->createdBy());
        if (creator && creator->groupId() == user->groupId()) return true;
    }
    // 创建者本人总能看自己的
    return report->createdBy() == user->id();
}

// ===========================================================================
// 报告编辑/删除（优化2）
// ===========================================================================

bool PermissionService::canEditReport(const Report::Ptr& report, const User::Ptr& user)
{
    if (!report || !user) return false;
    if (user->isSuperAdmin()) return true;
    // 草稿：创建者可编辑
    if (report->status() == ReportStatus::Draft && report->createdBy() == user->id()) return true;
    return false;
}

bool PermissionService::canDeleteReport(const Report::Ptr& report, const User::Ptr& user)
{
    if (!report || !user) return false;
    // 超管可删所有
    if (user->isSuperAdmin()) return true;
    // 已提交及以上：仅超管可删（上面已 return）
    if (report->status() != ReportStatus::Draft) return false;
    // 草稿：创建者（账户未禁用）可删
    if (report->createdBy() == user->id() && !user->isDisabled()) return true;
    // 创建者账户禁用后：组长（本组）+总管可删其草稿
    User::Ptr creator = UserRepository::findById(report->createdBy());
    if (creator && creator->isDisabled()) {
        if (user->isManager()) return true;
        if (user->isLeader() && user->groupId() > 0 && creator->groupId() == user->groupId()) return true;
    }
    return false;
}

// ===========================================================================
// 工作流操作
// ===========================================================================

bool PermissionService::canSubmitReport(const Report::Ptr& report, const User::Ptr& user)
{
    if (!report || !user) return false;
    if (report->status() != ReportStatus::Draft) return false;
    return report->createdBy() == user->id();
}

bool PermissionService::canReviewReport(const Report::Ptr& report, const User::Ptr& user)
{
    if (!report || !user) return false;
    if (report->status() != ReportStatus::Submitted) return false;
    if (user->isSuperAdmin() || user->isManager()) return true;
    // 组长审核本组报告
    if (user->isLeader() && user->groupId() > 0) {
        User::Ptr creator = UserRepository::findById(report->createdBy());
        if (creator && creator->groupId() == user->groupId()) return true;
    }
    return false;
}

bool PermissionService::canApproveReport(const Report::Ptr& report, const User::Ptr& user)
{
    if (!report || !user) return false;
    if (report->status() != ReportStatus::Reviewed) return false;
    return user->isSuperAdmin() || user->isManager();
}

bool PermissionService::canArchiveReport(const Report::Ptr& report, const User::Ptr& user)
{
    if (!report || !user) return false;
    if (report->status() != ReportStatus::Approved) return false;
    return user->isSuperAdmin() || user->isManager();
}

// ===========================================================================
// 用户管理（防越权四规则）
// ===========================================================================

bool PermissionService::canManageUser(const User::Ptr& currentUser, const User::Ptr& targetUser)
{
    if (!currentUser || !targetUser) return false;
    // 超管可管所有
    if (currentUser->isSuperAdmin()) return true;
    // 超管免疫：任何人不能管理超管
    if (targetUser->isSuperAdmin()) return false;
    // 同级不可管
    if (currentUser->role() == targetUser->role()) return false;
    // 总管：可管组长+组员（不可管超管，上面已排除）
    if (currentUser->isManager()) {
        return targetUser->isLeader() || targetUser->isMember();
    }
    // 组长：可管本组组员
    if (currentUser->isLeader() && currentUser->groupId() > 0) {
        return targetUser->isMember() && targetUser->groupId() == currentUser->groupId();
    }
    return false;
}

bool PermissionService::canDeleteUser(const User::Ptr& currentUser)
{
    return currentUser && currentUser->isSuperAdmin();
}

// ===========================================================================
// 模板操作（优化5）
// ===========================================================================

bool PermissionService::canRecallReport(const Report::Ptr& report, const User::Ptr& user)
{
    if (!report || !user) return false;
    if (user->isDisabled()) return false;
    // 仅"已提交"且为创建者本人可撤回（组长对 Submitted 用"审核退回"处理）
    return report->status() == ReportStatus::Submitted
           && report->createdBy() == user->id();
}

bool PermissionService::canSaveAsTemplate(const User::Ptr& user)
{
    return user && !user->isDisabled();
}

bool PermissionService::canPromoteTemplate(const User::Ptr& user)
{
    return user && (user->isSuperAdmin() || user->isManager());
}

// ===========================================================================
// 组管理
// ===========================================================================

bool PermissionService::canManageGroups(const User::Ptr& user)
{
    return user && (user->isSuperAdmin() || user->isManager());
}

bool PermissionService::isSameGroup(const User::Ptr& a, const User::Ptr& b)
{
    if (!a || !b) return false;
    return a->groupId() > 0 && a->groupId() == b->groupId();
}
