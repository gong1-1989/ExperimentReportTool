/**
 * @file tst_report.cpp
 * @brief Report 模型单元测试（纯逻辑，不依赖数据库/UI）
 *
 * 覆盖「连续文档 + 结构化对象」新模型（version 2）：
 * - 基础属性
 * - 对象增删查
 * - 文档序列化往返
 * - 旧版块 JSON 自动迁移
 */

#include <QtTest>

#include "core/models/Report.h"

class TestReport : public QObject
{
    Q_OBJECT

private slots:
    void testBasicAttributes();
    void testObjectAddRemove();
    void testObjectById();
    void testContentJsonRoundTrip();
    void testLegacyMigration();
    void testWordCountCached();
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
    QCOMPARE(report->objectCount(), 0);
    QVERIFY(report->document().isEmpty());
}

void TestReport::testObjectAddRemove()
{
    Report::Ptr report = makeReport();

    ContentBlock img(BlockType::Image);
    img.id = QStringLiteral("obj-1");
    img.data["caption"] = QStringLiteral("实验装置图");
    report->addObject(img);
    QCOMPARE(report->objectCount(), 1);

    ContentBlock table(BlockType::Table);
    table.id = QStringLiteral("obj-2");
    table.data["tableId"] = 5;
    report->addObject(table);
    QCOMPARE(report->objectCount(), 2);

    // 按 ID 删除
    QVERIFY(report->removeObject(QStringLiteral("obj-1")));
    QCOMPARE(report->objectCount(), 1);
    QVERIFY(!report->removeObject(QStringLiteral("obj-99")));
    QCOMPARE(report->objectCount(), 1);
}

void TestReport::testObjectById()
{
    Report::Ptr report = makeReport();
    ContentBlock chart(BlockType::Chart);
    chart.id = QStringLiteral("chart-1");
    chart.data["title"] = QStringLiteral("温度曲线");
    report->addObject(chart);

    const ContentBlock found = report->objectById(QStringLiteral("chart-1"));
    QCOMPARE(found.type, BlockType::Chart);
    QCOMPARE(found.data["title"].toString(), QStringLiteral("温度曲线"));

    // 找不到 → 空对象
    const ContentBlock missing = report->objectById(QStringLiteral("nope"));
    QVERIFY(missing.id.isEmpty());

    // 更新
    QJsonObject data;
    data["title"] = QStringLiteral("新标题");
    QVERIFY(report->updateObject(QStringLiteral("chart-1"), data));
    QCOMPARE(report->objectById(QStringLiteral("chart-1")).data["title"].toString(),
             QStringLiteral("新标题"));
}

void TestReport::testContentJsonRoundTrip()
{
    Report::Ptr report = makeReport();
    report->setDocument(QStringLiteral(
        "<html><body><h1>标题</h1><p>正文</p>"
        "<img src=\"object://table/obj-t1\" width=\"560\" height=\"120\"/>"
        "<p>结尾</p></body></html>"));

    ContentBlock table(BlockType::Table);
    table.id = QStringLiteral("obj-t1");
    table.data["tableId"] = 3;
    report->addObject(table);

    const QString json = report->contentToJson();

    Report::Ptr back = Report::create();
    back->contentFromJson(json);
    QCOMPARE(back->document(), report->document());
    QCOMPARE(back->objectCount(), 1);
    QCOMPARE(back->objectById(QStringLiteral("obj-t1")).data["tableId"].toLongLong(),
             qint64(3));
}

void TestReport::testLegacyMigration()
{
    // 旧版块数组（version 1）→ 自动迁移到 document + objects
    const QString legacyJson = QStringLiteral(
        "["
        "  {\"id\":\"b1\",\"type\":\"paragraph\","
        "   \"data\":{\"text\":\"<html><body><p>第一段</p></body></html>\"}},"
        "  {\"id\":\"b2\",\"type\":\"heading1\","
        "   \"data\":{\"text\":\"<html><body><h1>标题一</h1></body></html>\"}},"
        "  {\"id\":\"b3\",\"type\":\"image\","
        "   \"data\":{\"path\":\"C:/x.png\",\"caption\":\"图1\"}}"
        "]");

    Report::Ptr report = Report::create();
    report->contentFromJson(legacyJson);

    // 文本块拼入 document
    QVERIFY(report->document().contains(QStringLiteral("第一段")));
    QVERIFY(report->document().contains(QStringLiteral("标题一")));
    // 图片对象迁移为对象 + 锚点
    QCOMPARE(report->objectCount(), 1);
    QCOMPARE(report->objectById(QStringLiteral("b3")).type, BlockType::Image);
    QVERIFY(report->document().contains(QStringLiteral("object://image/b3")));
}

void TestReport::testWordCountCached()
{
    Report::Ptr report = makeReport();
    // 未设置缓存（m_wordCount=-1）→ 实时计算
    const int counted = report->wordCount();
    QCOMPARE(counted, 0);

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
