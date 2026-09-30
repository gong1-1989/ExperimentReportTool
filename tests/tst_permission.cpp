/**
 * @file tst_permission.cpp
 * @brief 权限矩阵单元测试（PermissionService，查库逻辑层）
 *
 * 场景：超管/总管/组长(组1)/组员1(组1)/组员2(组2)/禁用组员
 * 报告：草稿/已提交/已审核/已审批/已归档 × 不同创建者
 */

#include <QtTest>

#include "tst_testbase.h"

#include "service/PermissionService.h"

class TestPermission : public QObject, public TestDbBase
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase() { closeDb(); }

    void testViewMatrix();
    void testEditDelete();
    void testReviewApproveArchive();
    void testRecall();
    void testManageUser();
    void testMiscPermissions();

private:
    User::Ptr admin;
    User::Ptr manager;
    User::Ptr leader1;
    User::Ptr member1;
    User::Ptr member2;
    User::Ptr disabledUser;

    Report::Ptr rDraft;     // member1 草稿
    Report::Ptr rSubmitted; // member1 已提交
    Report::Ptr rReviewed;  // member1 已审核
    Report::Ptr rApproved;  // member1 已审批
    Report::Ptr rArchived;  // member1 已归档
    Report::Ptr rOtherDraft;// member2(组2) 草稿
};

void TestPermission::initTestCase()
{
    initDb();

    admin   = addUser(QStringLiteral("admin"),   UserRole::SuperAdmin);
    manager = addUser(QStringLiteral("manager"), UserRole::Manager);
    leader1 = addUser(QStringLiteral("leader1"), UserRole::Leader, 1);
    member1 = addUser(QStringLiteral("member1"), UserRole::Member, 1);
    member2 = addUser(QStringLiteral("member2"), UserRole::Member, 2);
    disabledUser = addUser(QStringLiteral("disabled"), UserRole::Member, 1, true);

    addGroup(QStringLiteral("组1"), leader1->id());

    rDraft      = addReport(QStringLiteral("草稿"),     member1->id(), ReportStatus::Draft);
    rSubmitted  = addReport(QStringLiteral("已提交"),   member1->id(), ReportStatus::Submitted);
    rReviewed   = addReport(QStringLiteral("已审核"),   member1->id(), ReportStatus::Reviewed);
    rApproved   = addReport(QStringLiteral("已审批"),   member1->id(), ReportStatus::Approved);
    rArchived   = addReport(QStringLiteral("已归档"),   member1->id(), ReportStatus::Archived);
    rOtherDraft = addReport(QStringLiteral("别组草稿"), member2->id(), ReportStatus::Draft);
}

// ---------------------------------------------------------------------------
// 可见性：草稿仅创建者+本组组长(+超管/总管)；提交后本组可见
// ---------------------------------------------------------------------------
void TestPermission::testViewMatrix()
{
    // 超管/总管全可见
    QVERIFY(PermissionService::canViewReport(rDraft, admin));
    QVERIFY(PermissionService::canViewReport(rDraft, manager));
    QVERIFY(PermissionService::canViewReport(rOtherDraft, admin));
    QVERIFY(PermissionService::canViewReport(rOtherDraft, manager));

    // 草稿：创建者本人 ✓、本组组长 ✓、别组成员 ✗
    QVERIFY(PermissionService::canViewReport(rDraft, member1));
    QVERIFY(PermissionService::canViewReport(rDraft, leader1));
    QVERIFY(!PermissionService::canViewReport(rDraft, member2));

    // 已提交：本组成员可见（member1/leader1），别组不可见
    QVERIFY(PermissionService::canViewReport(rSubmitted, member1));
    QVERIFY(PermissionService::canViewReport(rSubmitted, leader1));
    QVERIFY(!PermissionService::canViewReport(rSubmitted, member2));

    // 别组草稿：组长1 不可见
    QVERIFY(!PermissionService::canViewReport(rOtherDraft, leader1));
    QVERIFY(PermissionService::canViewReport(rOtherDraft, member2));

    // 空对象防御
    QVERIFY(!PermissionService::canViewReport(nullptr, member1));
}

// ---------------------------------------------------------------------------
// 编辑/删除：草稿创建者可编辑；提交后仅超管可删
// ---------------------------------------------------------------------------
void TestPermission::testEditDelete()
{
    // 编辑：草稿创建者 ✓、超管 ✓；组长/总管 ✗；已提交 ✗
    QVERIFY(PermissionService::canEditReport(rDraft, member1));
    QVERIFY(PermissionService::canEditReport(rDraft, admin));
    QVERIFY(!PermissionService::canEditReport(rDraft, leader1));
    QVERIFY(!PermissionService::canEditReport(rDraft, manager));
    QVERIFY(!PermissionService::canEditReport(rSubmitted, member1));

    // 删除：草稿创建者 ✓、超管 ✓；组长 ✗
    QVERIFY(PermissionService::canDeleteReport(rDraft, member1));
    QVERIFY(PermissionService::canDeleteReport(rDraft, admin));
    QVERIFY(!PermissionService::canDeleteReport(rDraft, leader1));

    // 提交后：仅超管可删
    QVERIFY(!PermissionService::canDeleteReport(rSubmitted, member1));
    QVERIFY(PermissionService::canDeleteReport(rSubmitted, admin));
}

// ---------------------------------------------------------------------------
// 审核/审批/归档
// ---------------------------------------------------------------------------
void TestPermission::testReviewApproveArchive()
{
    // 审核：已提交 + 组长(本组)/总管/超管
    QVERIFY(PermissionService::canReviewReport(rSubmitted, leader1));
    QVERIFY(PermissionService::canReviewReport(rSubmitted, manager));
    QVERIFY(PermissionService::canReviewReport(rSubmitted, admin));
    QVERIFY(!PermissionService::canReviewReport(rSubmitted, member1));   // 组员不可
    QVERIFY(!PermissionService::canReviewReport(rDraft, leader1));       // 草稿不可

    // 审批：已审核 + 总管/超管
    QVERIFY(PermissionService::canApproveReport(rReviewed, manager));
    QVERIFY(PermissionService::canApproveReport(rReviewed, admin));
    QVERIFY(!PermissionService::canApproveReport(rReviewed, leader1));
    QVERIFY(!PermissionService::canApproveReport(rSubmitted, manager));   // 未到审核态

    // 归档：已审批 + 总管/超管；已归档不可再归档
    QVERIFY(PermissionService::canArchiveReport(rApproved, manager));
    QVERIFY(PermissionService::canArchiveReport(rApproved, admin));
    QVERIFY(!PermissionService::canArchiveReport(rApproved, leader1));
    QVERIFY(!PermissionService::canArchiveReport(rArchived, manager));
}

// ---------------------------------------------------------------------------
// 撤回：仅"已提交"且创建者本人
// ---------------------------------------------------------------------------
void TestPermission::testRecall()
{
    QVERIFY(PermissionService::canRecallReport(rSubmitted, member1));
    QVERIFY(!PermissionService::canRecallReport(rSubmitted, leader1));
    QVERIFY(!PermissionService::canRecallReport(rSubmitted, manager));
    QVERIFY(!PermissionService::canRecallReport(rSubmitted, admin));
    QVERIFY(!PermissionService::canRecallReport(rSubmitted, member2));   // 非创建者

    QVERIFY(!PermissionService::canRecallReport(rDraft, member1));      // 草稿不可
    QVERIFY(!PermissionService::canRecallReport(rArchived, member1));    // 归档不可
}

// ---------------------------------------------------------------------------
// 用户管理：超管全管；总管管组长+组员；组长管本组组员
// ---------------------------------------------------------------------------
void TestPermission::testManageUser()
{
    // 超管可管所有人
    QVERIFY(PermissionService::canManageUser(admin, member1));
    QVERIFY(PermissionService::canManageUser(admin, manager));
    QVERIFY(PermissionService::canManageUser(admin, admin));

    // 总管：可管组长/组员，不可管超管/总管
    QVERIFY(PermissionService::canManageUser(manager, leader1));
    QVERIFY(PermissionService::canManageUser(manager, member1));
    QVERIFY(!PermissionService::canManageUser(manager, admin));

    // 组长：仅本组组员
    QVERIFY(PermissionService::canManageUser(leader1, member1));
    QVERIFY(!PermissionService::canManageUser(leader1, member2));   // 别组不可
    QVERIFY(!PermissionService::canManageUser(leader1, leader1));

    // 组员：不可管任何人
    QVERIFY(!PermissionService::canManageUser(member1, member1));

    // 删除用户：仅超管
    QVERIFY(PermissionService::canDeleteUser(admin));
    QVERIFY(!PermissionService::canDeleteUser(manager));
    QVERIFY(!PermissionService::canDeleteUser(leader1));
}

// ---------------------------------------------------------------------------
// 其他权限：模板/组管理/禁用账户
// ---------------------------------------------------------------------------
void TestPermission::testMiscPermissions()
{
    // 保存为模板：非禁用即可
    QVERIFY(PermissionService::canSaveAsTemplate(member1));
    QVERIFY(!PermissionService::canSaveAsTemplate(disabledUser));

    // 模板推广：超管/总管
    QVERIFY(PermissionService::canPromoteTemplate(admin));
    QVERIFY(PermissionService::canPromoteTemplate(manager));
    QVERIFY(!PermissionService::canPromoteTemplate(leader1));

    // 组管理：超管/总管
    QVERIFY(PermissionService::canManageGroups(admin));
    QVERIFY(PermissionService::canManageGroups(manager));
    QVERIFY(!PermissionService::canManageGroups(leader1));

    // 同组判断
    QVERIFY(PermissionService::isSameGroup(member1, member1));
    QVERIFY(!PermissionService::isSameGroup(member1, member2));
}

QTEST_APPLESS_MAIN(TestPermission)
#include "tst_permission.moc"
