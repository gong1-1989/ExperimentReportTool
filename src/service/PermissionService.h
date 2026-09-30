/**
 * @file PermissionService.h
 * @brief 权限服务头文件
 *
 * 集中管理四级角色（超管/总管/组长/组员）的权限判断，
 * 包括报告可见性、编辑、删除、工作流操作、用户管理等。
 * 所有权限判断都应通过本服务，避免散落在 UI 层。
 *
 * 防越权四规则：角色天花板、同级不可管、超管免疫、组隔离。
 */

#ifndef PERMISSION_SERVICE_H
#define PERMISSION_SERVICE_H

#include "core/models/User.h"
#include "core/models/Report.h"

class PermissionService
{
public:
    // -----------------------------------------------------------------------
    // 报告可见性（优化1：草稿仅创建者+组长+总管可见）
    // -----------------------------------------------------------------------

    /// 能否查看该报告（草稿可见性规则）
    static bool canViewReport(const Report::Ptr& report, const User::Ptr& user);

    // -----------------------------------------------------------------------
    // 报告编辑/删除（优化2：删除权限组合规则）
    // -----------------------------------------------------------------------

    /// 能否编辑该报告（草稿可编辑，已提交及以上仅超管可编辑）
    static bool canEditReport(const Report::Ptr& report, const User::Ptr& user);

    /// 能否删除该报告（组合规则：创建者+超管；创建者禁用后组长/总管可删草稿；已提交仅超管）
    static bool canDeleteReport(const Report::Ptr& report, const User::Ptr& user);

    // -----------------------------------------------------------------------
    // 工作流操作
    // -----------------------------------------------------------------------

    /// 能否提交（创建者本人，且状态为草稿）
    static bool canSubmitReport(const Report::Ptr& report, const User::Ptr& user);

    /// 能否审核（组长审核本组报告，状态为已提交）
    static bool canReviewReport(const Report::Ptr& report, const User::Ptr& user);

    /// 能否审批（总管审批，状态为已审核）
    static bool canApproveReport(const Report::Ptr& report, const User::Ptr& user);

    /// 能否归档（总管/超管，状态为已审批）
    static bool canArchiveReport(const Report::Ptr& report, const User::Ptr& user);

    /// 能否撤回提交（已提交且为创建者本人，撤回后回到草稿）
    static bool canRecallReport(const Report::Ptr& report, const User::Ptr& user);

    // -----------------------------------------------------------------------
    // 用户管理（防越权四规则）
    // -----------------------------------------------------------------------

    /// 能否管理目标用户（创建/编辑/禁用）
    static bool canManageUser(const User::Ptr& currentUser, const User::Ptr& targetUser);

    /// 能否删除用户（仅超管）
    static bool canDeleteUser(const User::Ptr& currentUser);

    // -----------------------------------------------------------------------
    // 模板操作（优化5）
    // -----------------------------------------------------------------------

    /// 能否保存为模板（创建者+组长+总管+超管）
    static bool canSaveAsTemplate(const User::Ptr& user);

    /// 能否将模板提升为全局（总管+超管）
    static bool canPromoteTemplate(const User::Ptr& user);

    // -----------------------------------------------------------------------
    // 组管理
    // -----------------------------------------------------------------------

    /// 能否管理组（总管+超管）
    static bool canManageGroups(const User::Ptr& user);

private:
    /// 是否同组（或目标用户无组时视为可管理）
    static bool isSameGroup(const User::Ptr& a, const User::Ptr& b);
};

#endif // PERMISSION_SERVICE_H
