/**
 * @file tst_versiondiffer.cpp
 * @brief VersionDiffer 纯逻辑单元测试（块提取 / 相似度 / LCS 差异对齐）
 */
#include <QtTest>
#include "core/utils/VersionDiffer.h"

class TestVersionDiffer : public QObject
{
    Q_OBJECT

private slots:
    void testEmptyContent();
    void testBlockSimilarity();
    void testBuildBlockDiffSame();
    void testBuildBlockDiffAddDelete();
    void testBuildBlockDiffModify();
};

void TestVersionDiffer::testEmptyContent()
{
    QVERIFY(VersionDiffer::extractBlocks(QString()).isEmpty());
    QVERIFY(VersionDiffer::extractBlocks("").isEmpty());
}

void TestVersionDiffer::testBlockSimilarity()
{
    // 完全相同
    QCOMPARE(VersionDiffer::blockSimilarity("hello", "hello"), 1.0);
    // 空串
    QCOMPARE(VersionDiffer::blockSimilarity("", "abc"), 0.0);
    // 完全不同（无共享 bigram）
    QCOMPARE(VersionDiffer::blockSimilarity("abc", "xyz"), 0.0);
    // 部分相似
    const double sim = VersionDiffer::blockSimilarity("实验步骤", "实验目的");
    QVERIFY(sim > 0.0 && sim < 1.0);
}

void TestVersionDiffer::testBuildBlockDiffSame()
{
    const QStringList a = {"块1", "块2", "块3"};
    const QStringList b = {"块1", "块2", "块3"};
    const auto r = VersionDiffer::buildBlockDiff(a, b);
    QCOMPARE(r.sameCount, 3);
    QCOMPARE(r.diffCount, 0);
    QVERIFY(r.leftHtml.contains("块1"));
    QVERIFY(r.rightHtml.contains("块3"));
}

void TestVersionDiffer::testBuildBlockDiffAddDelete()
{
    const QStringList a = {"块1", "块2", "块3"};
    const QStringList b = {"块1", "块3"};  // 删除了"块2"
    const auto r = VersionDiffer::buildBlockDiff(a, b);
    QCOMPARE(r.sameCount, 2);
    QVERIFY(r.diffCount >= 1);
    QVERIFY(r.leftHtml.contains("del"));   // A 侧有删除标记
}

void TestVersionDiffer::testBuildBlockDiffModify()
{
    const QStringList a = {"实验目的", "程序测试"};
    const QStringList b = {"实验目的", "一步步调试"};  // 第二块修改
    const auto r = VersionDiffer::buildBlockDiff(a, b);
    QCOMPARE(r.sameCount, 1);
    QVERIFY(r.diffCount >= 1);
    QVERIFY(r.leftHtml.contains("mod") || r.leftHtml.contains("del"));
}

QTEST_APPLESS_MAIN(TestVersionDiffer)
#include "tst_versiondiffer.moc"
