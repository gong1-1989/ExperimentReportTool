/**
 * @file TableBlockEditor.cpp
 * @brief 表格块编辑器实现文件
 */

#include "TableBlockEditor.h"
#include "core/utils/Logger.h"
#include "core/utils/AppTheme.h"
#include "core/utils/AppDimensions.h"

#include <QHeaderView>
#include <QJsonArray>
#include <QVBoxLayout>
#include <QHBoxLayout>

// ===========================================================================
// 构造函数
// ===========================================================================

TableBlockEditor::TableBlockEditor(const ContentBlock& block, QWidget* parent)
    : BlockEditor(block, parent)
    , m_table(nullptr)
    , m_addRowBtn(nullptr)
    , m_addColBtn(nullptr)
    , m_removeRowBtn(nullptr)
    , m_removeColBtn(nullptr)
{
    setupEditor();

    QVBoxLayout* layout = contentContainer();

    // 工具栏
    QHBoxLayout* toolbar = new QHBoxLayout();
    toolbar->setSpacing(AppTheme::Spacing::Small);

    m_addRowBtn = new QPushButton(tr("+ 行"), this);
    m_addColBtn = new QPushButton(tr("+ 列"), this);
    m_removeRowBtn = new QPushButton(tr("- 行"), this);
    m_removeColBtn = new QPushButton(tr("- 列"), this);

    for (QPushButton* btn : {m_addRowBtn, m_addColBtn, m_removeRowBtn, m_removeColBtn}) {
        btn->setStyleSheet(QString("QPushButton { padding: %1px %2px; font-size: %3px; }")
                               .arg(AppTheme::Spacing::Tiny)
                               .arg(AppTheme::Spacing::Medium)
                               .arg(AppTheme::FontSize::Small));
        toolbar->addWidget(btn);
    }
    toolbar->addStretch();
    layout->addLayout(toolbar);

    // 表格
    m_table = new QTableWidget(3, 3, this);
    m_table->setHorizontalHeaderLabels({tr("列1"), tr("列2"), tr("列3")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(true);
    m_table->verticalHeader()->setDefaultSectionSize(32);
    m_table->verticalHeader()->setMinimumSectionSize(32);
    m_table->setStyleSheet(
        QString("QTableWidget { border: 1px solid %1; gridline-color: %1; }")
            .arg(AppTheme::Color::BorderLight));
    m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_table->setMinimumHeight(AppDimensions::Widget::TableMinHeight);
    layout->addWidget(m_table);

    // 连接信号
    connect(m_addRowBtn, &QPushButton::clicked, this, &TableBlockEditor::onAddRow);
    connect(m_addColBtn, &QPushButton::clicked, this, &TableBlockEditor::onAddColumn);
    connect(m_removeRowBtn, &QPushButton::clicked, this, &TableBlockEditor::onRemoveRow);
    connect(m_removeColBtn, &QPushButton::clicked, this, &TableBlockEditor::onRemoveColumn);
    connect(m_table, &QTableWidget::cellChanged, this, &TableBlockEditor::onCellChanged);

    // 加载数据
    if (!block.data.isEmpty()) {
        setBlockData(block.data);
    }

    // 初始更新表格高度
    updateTableHeight();
}

// ===========================================================================
// 数据存取
// ===========================================================================

QJsonObject TableBlockEditor::blockData() const
{
    QJsonObject data;
    data["rows"] = m_table->rowCount();
    data["cols"] = m_table->columnCount();

    // 表头
    QJsonArray headers;
    for (int col = 0; col < m_table->columnCount(); ++col) {
        headers.append(m_table->horizontalHeaderItem(col)
                           ? m_table->horizontalHeaderItem(col)->text()
                           : QString("列%1").arg(col + 1));
    }
    data["headers"] = headers;

    // 单元格数据
    QJsonArray cells;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        QJsonArray rowData;
        for (int col = 0; col < m_table->columnCount(); ++col) {
            QTableWidgetItem* item = m_table->item(row, col);
            rowData.append(item ? item->text() : QString());
        }
        cells.append(rowData);
    }
    data["cells"] = cells;

    return data;
}

void TableBlockEditor::setBlockData(const QJsonObject& data)
{
    const int rows = data.value("rows").toInt(3);
    const int cols = data.value("cols").toInt(3);

    m_table->setRowCount(rows);
    m_table->setColumnCount(cols);

    // 表头
    if (data.value("headers").isArray()) {
        const QJsonArray headers = data.value("headers").toArray();
        for (int i = 0; i < headers.size() && i < cols; ++i) {
            m_table->setHorizontalHeaderItem(i, new QTableWidgetItem(headers[i].toString()));
        }
    }

    // 单元格数据
    if (data.value("cells").isArray()) {
        const QJsonArray cells = data.value("cells").toArray();
        for (int r = 0; r < cells.size() && r < rows; ++r) {
            const QJsonArray rowData = cells[r].toArray();
            for (int c = 0; c < rowData.size() && c < cols; ++c) {
                m_table->setItem(r, c, new QTableWidgetItem(rowData[c].toString()));
            }
        }
    }

    updateTableHeight();
}

QString TableBlockEditor::plainText() const
{
    QStringList texts;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        QStringList rowTexts;
        for (int col = 0; col < m_table->columnCount(); ++col) {
            QTableWidgetItem* item = m_table->item(row, col);
            if (item) rowTexts.append(item->text());
        }
        texts.append(rowTexts.join(" | "));
    }
    return texts.join("\n");
}

// ===========================================================================
// 槽函数
// ===========================================================================

void TableBlockEditor::onAddRow()
{
    m_table->insertRow(m_table->rowCount());
    updateTableHeight();
    notifyContentChanged();
}

void TableBlockEditor::onAddColumn()
{
    const int col = m_table->columnCount();
    m_table->insertColumn(col);
    m_table->setHorizontalHeaderItem(col, new QTableWidgetItem(QString("列%1").arg(col + 1)));
    notifyContentChanged();
}

void TableBlockEditor::onRemoveRow()
{
    if (m_table->rowCount() > 1) {
        m_table->removeRow(m_table->currentRow() >= 0 ? m_table->currentRow() : m_table->rowCount() - 1);
        updateTableHeight();
        notifyContentChanged();
    }
}

void TableBlockEditor::onRemoveColumn()
{
    if (m_table->columnCount() > 1) {
        m_table->removeColumn(m_table->currentColumn() >= 0 ? m_table->currentColumn() : m_table->columnCount() - 1);
        notifyContentChanged();
    }
}

void TableBlockEditor::onCellChanged(int row, int col)
{
    Q_UNUSED(row);
    Q_UNUSED(col);
    notifyContentChanged();
}

// ===========================================================================
// 内部方法
// ===========================================================================

void TableBlockEditor::updateTableHeight()
{
    for (int row = 0; row < m_table->rowCount(); ++row) {
        m_table->setRowHeight(row, 32);
    }

    const int headerHeight = m_table->horizontalHeader()->height();
    const int rowHeight = 32;
    const int margins = 10;
    const int height = headerHeight + m_table->rowCount() * rowHeight + margins;
    m_table->setFixedHeight(qMax(150, height));

    updateHeight();
}
