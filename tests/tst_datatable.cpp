/**
 * @file tst_datatable.cpp
 * @brief DataTable 模型单元测试（纯逻辑，不依赖数据库/UI）
 */

#include <QtTest>

#include "core/models/DataTable.h"

class TestDataTable : public QObject
{
    Q_OBJECT

private slots:
    void init();                       // 每个用例前重置表格
    void testAppendColumn();
    void testInsertRemoveColumn();
    void testRowOperations();
    void testCellAccessOutOfRange();   // 越界访问必须安全返回
    void testValidateRequired();       // 必填项校验
    void testNumericColumn();

private:
    DataTable::Ptr m_table;
};

void TestDataTable::init()
{
    m_table = DataTable::create();
}

void TestDataTable::testAppendColumn()
{
    ColumnDefinition col;
    col.name = QStringLiteral("温度");
    col.type = ColumnType::Number;
    m_table->appendColumn(col);

    QCOMPARE(m_table->columnCount(), 1);
    QCOMPARE(m_table->columnAt(0).name, QStringLiteral("温度"));
    QCOMPARE(m_table->columnAt(0).type, ColumnType::Number);
}

void TestDataTable::testInsertRemoveColumn()
{
    ColumnDefinition c1;
    c1.name = QStringLiteral("A");
    ColumnDefinition c2;
    c2.name = QStringLiteral("B");
    ColumnDefinition c3;
    c3.name = QStringLiteral("C");
    m_table->appendColumn(c1);
    m_table->appendColumn(c2);
    m_table->appendColumn(c3);

    // 插入到中间
    ColumnDefinition cX;
    cX.name = QStringLiteral("X");
    m_table->insertColumn(1, cX);
    QCOMPARE(m_table->columnCount(), 4);
    QCOMPARE(m_table->columnAt(1).name, QStringLiteral("X"));

    // 删除
    m_table->removeColumn(1);
    QCOMPARE(m_table->columnCount(), 3);
    QCOMPARE(m_table->columnAt(1).name, QStringLiteral("B"));
}

void TestDataTable::testRowOperations()
{
    ColumnDefinition col;
    col.name = QStringLiteral("值");
    m_table->appendColumn(col);

    // 追加行并写值
    m_table->appendRow();
    m_table->setCellValue(0, 0, 42);
    QCOMPARE(m_table->rowCount(), 1);
    QCOMPARE(m_table->cellValue(0, 0).toInt(), 42);

    // 再追加带数据的行
    QVariantList row;
    row << 99;
    m_table->appendRow(row);
    QCOMPARE(m_table->rowCount(), 2);
    QCOMPARE(m_table->cellValue(1, 0).toInt(), 99);

    // 删除行
    m_table->removeRow(0);
    QCOMPARE(m_table->rowCount(), 1);
    QCOMPARE(m_table->cellValue(0, 0).toInt(), 99);
}

void TestDataTable::testCellAccessOutOfRange()
{
    ColumnDefinition col;
    col.name = QStringLiteral("值");
    m_table->appendColumn(col);
    m_table->appendRow();

    // 越界行列必须安全返回（不崩溃、返回空值/空列）
    QVERIFY(!m_table->cellValue(5, 0).isValid());
    QVERIFY(!m_table->cellValue(0, 5).isValid());
    QCOMPARE(m_table->rowAt(99).size(), 0);
    QVERIFY(m_table->columnAt(99).name.isEmpty());
}

void TestDataTable::testValidateRequired()
{
    ColumnDefinition requiredCol;
    requiredCol.name = QStringLiteral("必填");
    requiredCol.required = true;
    m_table->appendColumn(requiredCol);
    m_table->appendRow();

    // 未填必填项 → 有错误
    QStringList errors = m_table->validate();
    QVERIFY(!errors.isEmpty());

    // 填上后 → 无错误
    m_table->setCellValue(0, 0, QStringLiteral("已填"));
    QVERIFY(m_table->validate().isEmpty());
}

void TestDataTable::testNumericColumn()
{
    ColumnDefinition col;
    col.name = QStringLiteral("数值");
    col.type = ColumnType::Number;
    m_table->appendColumn(col);

    m_table->appendRow();
    m_table->setCellValue(0, 0, 1.5);
    m_table->appendRow();
    m_table->setCellValue(1, 0, 2.5);
    m_table->appendRow();
    m_table->setCellValue(2, 0, QStringLiteral("非数值"));  // 应被跳过

    const QVector<double> nums = m_table->numericColumn(0);
    QCOMPARE(nums.size(), 2);
    QCOMPARE(nums.at(0), 1.5);
    QCOMPARE(nums.at(1), 2.5);
}

QTEST_APPLESS_MAIN(TestDataTable)
#include "tst_datatable.moc"
