/**
 * @file tst_workflow.cpp
 * @brief 报告工作流状态机单元测试（WorkflowService + 归档，查库逻辑层）
 *
 * 覆盖：正常四段流、组长退回、总管审批退回、创建者撤回、
 *       越权操作、禁用账户、归档锁定、last_action 记录。
 */

#include <QtTest>

#include "tst_testbase.h"

#include "service/WorkflowService.h"

class TestWorkflow : public QObject, public TestDbBase
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase() { closeDb(); }

    void testHappyPath();
    void testLeaderReject();
    void testManagerReject();
    void testRecall();
    void testRecallUnauthorized();
    void testCrossGroupReview();
    void testDisabledUser();
    void testArchivedLocked();
    void testLastActionRecorded();

private:
    User::Ptr admin;
    User::Ptr manager;
    User::Ptr leader1;
    User::Ptr member1;
    User::Ptr member2;
    User::Ptr disabledUser;

    Report::Ptr makeDraft(qint64 createdBy = -1);
};

void TestWorkflow::initTestCase()
{
    initDb();

    admin   = addUser(QStringLiteral("admin"),   UserRole::SuperAdmin);
    manager = addUser(QStringLiteral("manager"), UserRole::Manager);
    leader1 = addUser(QStringLiteral("leader1"), UserRole::Leader, 1);
    member1 = addUser(QStringLiteral("member1"), UserRole::Member, 1);
    member2 = addUser(QStringLiteral("member2"), UserRole::Member, 2);
    disabledUser = addUser(QStringLiteral("disabled"), UserRole::Member, 1, true);

    addGroup(QStringLiteral("组1"), leader1->id());
}

Report::Ptr TestWorkflow::makeDraft(qint64 createdBy)
{
    if (createdBy < 0) createdBy = member1->id();
    return addReport(QStringLiteral("实验报告"), createdBy, ReportStatus::Draft);
}

// ---------------------------------------------------------------------------
// 正常流：草稿 → 提交 → 审核通过 → 审批通过 → 归档
// ---------------------------------------------------------------------------
void TestWorkflow::testHappyPath()
{
    Report::Ptr r = makeDraft();

    WorkflowService::Result s1 = WorkflowService::submit(r->id(), member1->id());
    QVERIFY2(s1.success, qPrintable(s1.errorMessage));
    QCOMPARE(fresh(r->id())->status(), ReportStatus::Submitted);

    WorkflowService::Result s2 = WorkflowService::reviewApprove(r->id(), leader1->id());
    QVERIFY2(s2.success, qPrintable(s2.errorMessage));
    QCOMPARE(fresh(r->id())->status(), ReportStatus::Reviewed);

    WorkflowService::Result s3 = WorkflowService::approveApprove(r->id(), manager->id());
    QVERIFY2(s3.success, qPrintable(s3.errorMessage));
    QCOMPARE(fresh(r->id())->status(), ReportStatus::Approved);

    // 归档（updateStatus 通道）
    QVERIFY(ReportService::updateStatus(r->id(), ReportStatus::Archived));
    QCOMPARE(fresh(r->id())->status(), ReportStatus::Archived);
}

// ---------------------------------------------------------------------------
// 组长退回：意见必填；退回后回草稿
// ---------------------------------------------------------------------------
void TestWorkflow::testLeaderReject()
{
    Report::Ptr r = makeDraft();
    QVERIFY(WorkflowService::submit(r->id(), member1->id()).success);

    // 空意见 → 失败
    WorkflowService::Result bad = WorkflowService::reviewReject(r->id(), leader1->id(), QString());
    QVERIFY(!bad.success);
    QCOMPARE(fresh(r->id())->status(), ReportStatus::Submitted);   // 状态不变

    // 带意见 → 成功，回到草稿
    WorkflowService::Result ok = WorkflowService::reviewReject(r->id(), leader1->id(), QStringLiteral("数据不完整，请补充"));
    QVERIFY2(ok.success, qPrintable(ok.errorMessage));
    QCOMPARE(fresh(r->id())->status(), ReportStatus::Draft);
}

// ---------------------------------------------------------------------------
// 总管审批退回：意见必填；退回后回草稿
// ---------------------------------------------------------------------------
void TestWorkflow::testManagerReject()
{
    Report::Ptr r = makeDraft();
    QVERIFY(WorkflowService::submit(r->id(), member1->id()).success);
    QVERIFY(WorkflowService::reviewApprove(r->id(), leader1->id()).success);

    WorkflowService::Result bad = WorkflowService::approveReject(r->id(), manager->id(), QString());
    QVERIFY(!bad.success);
    QCOMPARE(fresh(r->id())->status(), ReportStatus::Reviewed);

    WorkflowService::Result ok = WorkflowService::approveReject(r->id(), manager->id(), QStringLiteral("格式不规范"));
    QVERIFY2(ok.success, qPrintable(ok.errorMessage));
    QCOMPARE(fresh(r->id())->status(), ReportStatus::Draft);
}

// ---------------------------------------------------------------------------
// 撤回：创建者主动收回（Submitted→Draft）；状态不符/他人不可
// ---------------------------------------------------------------------------
void TestWorkflow::testRecall()
{
    Report::Ptr r = makeDraft();
    QVERIFY(WorkflowService::submit(r->id(), member1->id()).success);

    WorkflowService::Result ok = WorkflowService::recall(r->id(), member1->id());
    QVERIFY2(ok.success, qPrintable(ok.errorMessage));
    QCOMPARE(fresh(r->id())->status(), ReportStatus::Draft);

    // 撤回后再次撤回 → 状态不匹配（已回草稿）
    WorkflowService::Result again = WorkflowService::recall(r->id(), member1->id());
    QVERIFY(!again.success);
}

// ---------------------------------------------------------------------------
// 越权撤回：组长/总管/超管/别组成员撤回他人提交
// ---------------------------------------------------------------------------
void TestWorkflow::testRecallUnauthorized()
{
    Report::Ptr r = makeDraft();
    QVERIFY(WorkflowService::submit(r->id(), member1->id()).success);

    QVERIFY(!WorkflowService::recall(r->id(), leader1->id()).success);
    QVERIFY(!WorkflowService::recall(r->id(), manager->id()).success);
    QVERIFY(!WorkflowService::recall(r->id(), admin->id()).success);
    QVERIFY(!WorkflowService::recall(r->id(), member2->id()).success);
    QCOMPARE(fresh(r->id())->status(), ReportStatus::Submitted);   // 仍为已提交
}

// ---------------------------------------------------------------------------
// 跨组越权审核
// ---------------------------------------------------------------------------
void TestWorkflow::testCrossGroupReview()
{
    Report::Ptr r = makeDraft();
    QVERIFY(WorkflowService::submit(r->id(), member1->id()).success);

    WorkflowService::Result s = WorkflowService::reviewApprove(r->id(), member2->id());
    QVERIFY(!s.success);   // 组2 组员无权审核组1 报告
    QCOMPARE(fresh(r->id())->status(), ReportStatus::Submitted);
}

// ---------------------------------------------------------------------------
// 禁用账户不可提交
// ---------------------------------------------------------------------------
void TestWorkflow::testDisabledUser()
{
    Report::Ptr r = makeDraft(disabledUser->id());
    WorkflowService::Result s = WorkflowService::submit(r->id(), disabledUser->id());
    QVERIFY(!s.success);
    QCOMPARE(fresh(r->id())->status(), ReportStatus::Draft);
}

// ---------------------------------------------------------------------------
// 归档锁定：归档后任何流转（提交/审核/审批/撤回）都拒绝
// ---------------------------------------------------------------------------
void TestWorkflow::testArchivedLocked()
{
    Report::Ptr r = makeDraft();
    QVERIFY(WorkflowService::submit(r->id(), member1->id()).success);
    QVERIFY(WorkflowService::reviewApprove(r->id(), leader1->id()).success);
    QVERIFY(WorkflowService::approveApprove(r->id(), manager->id()).success);
    QVERIFY(ReportService::updateStatus(r->id(), ReportStatus::Archived));
    QCOMPARE(fresh(r->id())->status(), ReportStatus::Archived);

    QVERIFY(!WorkflowService::submit(r->id(), member1->id()).success);
    QVERIFY(!WorkflowService::recall(r->id(), member1->id()).success);
    QVERIFY(!WorkflowService::reviewApprove(r->id(), leader1->id()).success);
    QVERIFY(!WorkflowService::approveApprove(r->id(), manager->id()).success);
    QCOMPARE(fresh(r->id())->status(), ReportStatus::Archived);
}

// ---------------------------------------------------------------------------
// last_action 记录
// ---------------------------------------------------------------------------
void TestWorkflow::testLastActionRecorded()
{
    Report::Ptr r = makeDraft();
    QVERIFY(WorkflowService::submit(r->id(), member1->id()).success);

    Report::Ptr submitted = fresh(r->id());
    QCOMPARE(submitted->lastAction(), QStringLiteral("submit"));
    QCOMPARE(submitted->lastActionBy(), member1->id());
    QVERIFY(submitted->lastActionAt().isValid());

    // 撤回后记录 recall
    QVERIFY(WorkflowService::recall(r->id(), member1->id()).success);
    Report::Ptr recalled = fresh(r->id());
    QCOMPARE(recalled->lastAction(), QStringLiteral("recall"));
    QCOMPARE(recalled->lastActionBy(), member1->id());
}

QTEST_APPLESS_MAIN(TestWorkflow)
#include "tst_workflow.moc"
