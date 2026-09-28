/**
 * @file tst_report.cpp
 * @brief Report 模型单元测试（纯逻辑，不依赖数据库/UI）
 */

#include <QtTest>

#include "core/models/Report.h"

class TestReport : public QObject
{
    Q_OBJECT

private slots:
    void testBasicAttributes();
    void testBlockAppendInsert();
    void testBlockRemoveMove();
    void testBlockOutOfRange();        // 越界访问安全
    void testWordCountCached();        // 缓存优先，实时计算兜底
    void testContentBlockJson();       // 块序列化往返

private:
    static Report::Ptr makeReport();
};

Report::Ptr TestReport::makeReport()
{
    Report::Ptr report = Report::create();
    report->setTitle(QStringLiteral("测试报告"));
    report->setProjectId(1);
    return report;
}

void TestReport::testBasicAttributes()
{
    Report::Ptr report = makeReport();
    QCOMPARE(report->title(), QStringLiteral("测试报告"));
    QCOMPARE(report->projectId(), 1);
    QCOMPARE(report->blockCount(), 0);
}

void TestReport::testBlockAppendInsert()
{
    Report::Ptr report = makeReport();

    ContentBlock b1(BlockType::Paragraph);
    b1.data["text"] = QStringLiteral("第一段");
    report->appendBlock(b1);
    QCOMPARE(report->blockCount(), 1);
    QCOMPARE(report->blockAt(0).data["text"].toString(), QStringLiteral("第一段"));

    ContentBlock b2(BlockType::Heading1);
    b2.data["text"] = QStringLiteral("标题");
    report->insertBlock(0, b2);  // 插到开头
    QCOMPARE(report->blockCount(), 2);
    QCOMPARE(report->blockAt(0).type, BlockType::Heading1);
    QCOMPARE(report->blockAt(1).type, BlockType::Paragraph);
}

void TestReport::testBlockRemoveMove()
{
    Report::Ptr report = makeReport();
    for (int i = 0; i < 3; ++i) {
        ContentBlock b(BlockType::Paragraph);
        b.data["text"] = QStringLiteral("块%1").arg(i + 1);
        report->appendBlock(b);
    }

    // 移动 0→2：A B C → B C A
    report->moveBlock(0, 2);
    QCOMPARE(report->blockAt(0).data["text"].toString(), QStringLiteral("块2"));
    QCOMPARE(report->blockAt(2).data["text"].toString(), QStringLiteral("块1"));

    // 删除中间：B C A → B A
    report->removeBlock(1);
    QCOMPARE(report->blockCount(), 2);
    QCOMPARE(report->blockAt(1).data["text"].toString(), QStringLiteral("块1"));

    // 清空
    report->clearBlocks();
    QCOMPARE(report->blockCount(), 0);
}

void TestReport::testBlockOutOfRange()
{
    Report::Ptr report = makeReport();
    // 空报告访问越界块必须安全
    const ContentBlock& b = report->blockAt(0);
    QVERIFY(b.data.isEmpty());  // 返回空块（需检查 blockAt 越界返回空块语义）

    report->appendBlock(ContentBlock(BlockType::Paragraph));
    const ContentBlock& b2 = report->blockAt(99);
    QVERIFY(b2.data.isEmpty());
}

void TestReport::testWordCountCached()
{
    Report::Ptr report = makeReport();
    ContentBlock b(BlockType::Paragraph);
    b.data["text"] = QStringLiteral("三个汉字");
    report->appendBlock(b);

    // 未设置缓存（m_wordCount=-1）→ 实时计算（3 个汉字）
    const int counted = report->wordCount();
    QCOMPARE(counted, 3);

    // 缓存优先：手动设置后返回缓存值
    report->setWordCount(100);
    QCOMPARE(report->wordCount(), 100);
}

void TestReport::testContentBlockJson()
{
    ContentBlock b(BlockType::CodeBlock);
    b.id = QStringLiteral("blk-1");
    b.data["language"] = QStringLiteral("cpp");
    b.data["code"] = QStringLiteral("int x = 1;");

    const QJsonObject json = b.toJson();
    ContentBlock back = ContentBlock::fromJson(json);
    QCOMPARE(back.type, BlockType::CodeBlock);
    QCOMPARE(back.id, QStringLiteral("blk-1"));
    QCOMPARE(back.data["language"].toString(), QStringLiteral("cpp"));
    QCOMPARE(back.data["code"].toString(), QStringLiteral("int x = 1;"));
}

QTEST_APPLESS_MAIN(TestReport)
#include "tst_report.moc"
