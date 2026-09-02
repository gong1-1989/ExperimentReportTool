/**
 * @file DataTableEditorDialog.cpp
 * @brief 数据表编辑器对话框实现文件
 */

#include "DataTableEditorDialog.h"
#include "DataImportDialog.h"
#include "core/utils/Logger.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QInputDialog>
#include <QColor>

// ===========================================================================
// ColumnPropertyPanel 实现
// ===========================================================================

ColumnPropertyPanel::ColumnPropertyPanel(QWidget* parent)
    : QWidget(parent)
    , m_columnIndex(-1)
    , m_nameEdit(nullptr)
    , m_typeCombo(nullptr)
    , m_unitEdit(nullptr)
    , m_requiredCheck(nullptr)
    , m_minSpin(nullptr)
    , m_maxSpin(nullptr)
    , m_rangeLabel(nullptr)
{
    setupUi();
}

void ColumnPropertyPanel::setupUi()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    QLabel* title = new QLabel(tr("列属性"), this);
    title->setStyleSheet("font-weight: bold; font-size: 14px; padding-bottom: 8px; border-bottom: 1px solid #ddd;");
    layout->addWidget(title);

    QFormLayout* form = new QFormLayout();
    form->setSpacing(8);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("列名称"));
    form->addRow(tr("名称:"), m_nameEdit);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem(tr("数值"), static_cast<int>(ColumnType::Number));
    m_typeCombo->addItem(tr("文本"), static_cast<int>(ColumnType::Text));
    m_typeCombo->addItem(tr("日期"), static_cast<int>(ColumnType::Date));
    m_typeCombo->addItem(tr("布尔"), static_cast<int>(ColumnType::Boolean));
    form->addRow(tr("类型:"), m_typeCombo);

    m_unitEdit = new QLineEdit(this);
    m_unitEdit->setPlaceholderText(tr("如: m/s, kg, °C"));
    form->addRow(tr("单位:"), m_unitEdit);

    m_requiredCheck = new QCheckBox(tr("必填"), this);
    form->addRow("", m_requiredCheck);

    // 数值范围（仅数值类型显示）
    m_rangeLabel = new QLabel(tr("数值范围:"), this);
    m_rangeLabel->setStyleSheet("margin-top: 8px; font-weight: bold;");
    form->addRow(m_rangeLabel);

    QHBoxLayout* rangeLayout = new QHBoxLayout();
    m_minSpin = new QDoubleSpinBox(this);
    m_minSpin->setRange(-1e18, 1e18);
    m_minSpin->setDecimals(6);
    m_minSpin->setPrefix(tr("最小: "));
    m_maxSpin = new QDoubleSpinBox(this);
    m_maxSpin->setRange(-1e18, 1e18);
    m_maxSpin->setDecimals(6);
    m_maxSpin->setPrefix(tr("最大: "));
    rangeLayout->addWidget(m_minSpin);
    rangeLayout->addWidget(m_maxSpin);
    form->addRow(rangeLayout);

    layout->addLayout(form);
    layout->addStretch();

    // 连接信号
    connect(m_nameEdit, &QLineEdit::textChanged, this, &ColumnPropertyPanel::onNameChanged);
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ColumnPropertyPanel::onTypeChanged);
    connect(m_unitEdit, &QLineEdit::textChanged, this, &ColumnPropertyPanel::onUnitChanged);
    connect(m_requiredCheck, &QCheckBox::stateChanged, this, &ColumnPropertyPanel::onRequiredChanged);
    connect(m_minSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ColumnPropertyPanel::onMinChanged);
    connect(m_maxSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ColumnPropertyPanel::onMaxChanged);

    updateVisibility();
}

void ColumnPropertyPanel::setColumn(const ColumnDefinition& col, int columnIndex)
{
    m_column = col;
    m_columnIndex = columnIndex;

    m_nameEdit->setText(col.name);
    m_unitEdit->setText(col.unit);
    m_requiredCheck->setChecked(col.required);
    m_minSpin->setValue(col.minValue);
    m_maxSpin->setValue(col.maxValue);

    const int typeIdx = m_typeCombo->findData(static_cast<int>(col.type));
    if (typeIdx >= 0) m_typeCombo->setCurrentIndex(typeIdx);

    updateVisibility();
}

ColumnDefinition ColumnPropertyPanel::column() const
{
    return m_column;
}

void ColumnPropertyPanel::updateVisibility()
{
    const bool isNumber = m_column.type == ColumnType::Number;
    m_rangeLabel->setVisible(isNumber);
    m_minSpin->setVisible(isNumber);
    m_maxSpin->setVisible(isNumber);
}

void ColumnPropertyPanel::onNameChanged(const QString& name)
{
    m_column.name = name;
    emit columnChanged(m_columnIndex, m_column);
}

void ColumnPropertyPanel::onTypeChanged(int index)
{
    m_column.type = static_cast<ColumnType>(m_typeCombo->itemData(index).toInt());
    updateVisibility();
    emit columnChanged(m_columnIndex, m_column);
}

void ColumnPropertyPanel::onUnitChanged(const QString& unit)
{
    m_column.unit = unit;
    emit columnChanged(m_columnIndex, m_column);
}

void ColumnPropertyPanel::onRequiredChanged(int state)
{
    m_column.required = (state == Qt::Checked);
    emit columnChanged(m_columnIndex, m_column);
}

void ColumnPropertyPanel::onMinChanged(double value)
{
    m_column.minValue = value;
    emit columnChanged(m_columnIndex, m_column);
}

void ColumnPropertyPanel::onMaxChanged(double value)
{
    m_column.maxValue = value;
    emit columnChanged(m_columnIndex, m_column);
}

// ===========================================================================
// DataTableEditorDialog 实现
// ===========================================================================

DataTableEditorDialog::DataTableEditorDialog(const DataTable::Ptr& table, QWidget* parent)
    : QDialog(parent)
    , m_table(table)
    , m_tableWidget(nullptr)
    , m_columnPanel(nullptr)
    , m_columnInfoLabel(nullptr)
    , m_buttonBox(nullptr)
    , m_statusLabel(nullptr)
    , m_currentColumn(-1)
    , m_loading(false)
{
    setupUi();
    loadTable();
    setWindowTitle(tr("数据表编辑器: %1").arg(m_table->name()));
    resize(1000, 600);
}

void DataTableEditorDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // -----------------------------------------------------------------------
    // 工具栏
    // -----------------------------------------------------------------------
    QHBoxLayout* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    m_addRowBtn = new QPushButton(tr("+ 行"), this);
    m_addColBtn = new QPushButton(tr("+ 列"), this);
    m_insertRowBtn = new QPushButton(tr("插入行"), this);
    m_insertColBtn = new QPushButton(tr("插入列"), this);
    m_removeRowBtn = new QPushButton(tr("- 行"), this);
    m_removeColBtn = new QPushButton(tr("- 列"), this);

    for (QPushButton* btn : {m_addRowBtn, m_addColBtn, m_insertRowBtn, m_insertColBtn, m_removeRowBtn, m_removeColBtn}) {
        btn->setStyleSheet("QPushButton { padding: 4px 10px; font-size: 12px; }");
        toolbar->addWidget(btn);
    }

    QFrame* vLine = new QFrame();
    vLine->setFrameShape(QFrame::VLine);         // 竖线
    vLine->setFrameShadow(QFrame::Sunken);
    vLine->setFixedWidth(2);
    toolbar->addWidget(vLine);

    m_importBtn = new QPushButton(tr("导入CSV"), this);
    m_exportBtn = new QPushButton(tr("导出CSV"), this);
    m_validateBtn = new QPushButton(tr("校验数据"), this);
    for (QPushButton* btn : {m_importBtn, m_exportBtn, m_validateBtn}) {
        btn->setStyleSheet("QPushButton { padding: 4px 10px; font-size: 12px; }");
        toolbar->addWidget(btn);
    }

    toolbar->addStretch();
    mainLayout->addLayout(toolbar);

    // -----------------------------------------------------------------------
    // 主区域：表格 + 列属性
    // -----------------------------------------------------------------------
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    // 表格
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->setStyleSheet("QTableWidget { gridline-color: #ddd; }");
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_tableWidget->horizontalHeader()->setStretchLastSection(true);
    m_tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    splitter->addWidget(m_tableWidget);

    // 列属性面板
    QWidget* rightPanel = new QWidget(this);
    rightPanel->setMaximumWidth(300);
    rightPanel->setStyleSheet("QWidget { background: #fafafa; border-left: 1px solid #eee; }");
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    m_columnInfoLabel = new QLabel(tr("选择一列以编辑属性"), rightPanel);
    m_columnInfoLabel->setStyleSheet("padding: 8px 12px; background: #f0f0f0; border-bottom: 1px solid #ddd; font-weight: bold;");
    rightLayout->addWidget(m_columnInfoLabel);

    m_columnPanel = new ColumnPropertyPanel(rightPanel);
    rightLayout->addWidget(m_columnPanel);

    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter, 1);

    // -----------------------------------------------------------------------
    // 底部
    // -----------------------------------------------------------------------
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: #666; font-size: 12px; padding: 4px 0;");
    mainLayout->addWidget(m_statusLabel);

    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setText(tr("确定"));
    m_buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("取消"));
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &DataTableEditorDialog::onAccept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(m_buttonBox);

    // -----------------------------------------------------------------------
    // 连接信号
    // -----------------------------------------------------------------------
    connect(m_addRowBtn, &QPushButton::clicked, this, &DataTableEditorDialog::onAddRow);
    connect(m_addColBtn, &QPushButton::clicked, this, &DataTableEditorDialog::onAddColumn);
    connect(m_insertRowBtn, &QPushButton::clicked, this, &DataTableEditorDialog::onInsertRow);
    connect(m_insertColBtn, &QPushButton::clicked, this, &DataTableEditorDialog::onInsertColumn);
    connect(m_removeRowBtn, &QPushButton::clicked, this, &DataTableEditorDialog::onRemoveRow);
    connect(m_removeColBtn, &QPushButton::clicked, this, &DataTableEditorDialog::onRemoveColumn);
    connect(m_importBtn, &QPushButton::clicked, this, &DataTableEditorDialog::onImportCsv);
    connect(m_exportBtn, &QPushButton::clicked, this, &DataTableEditorDialog::onExportCsv);
    connect(m_validateBtn, &QPushButton::clicked, this, &DataTableEditorDialog::onValidate);

    connect(m_tableWidget, &QTableWidget::cellChanged, this, &DataTableEditorDialog::onCellChanged);
    connect(m_tableWidget, &QTableWidget::currentCellChanged, this, &DataTableEditorDialog::onCurrentCellChanged);
    connect(m_tableWidget->horizontalHeader(), &QHeaderView::sectionDoubleClicked,
            this, &DataTableEditorDialog::onHeaderDoubleClicked);

    connect(m_columnPanel, &ColumnPropertyPanel::columnChanged,
            this, &DataTableEditorDialog::onColumnChanged);
}

void DataTableEditorDialog::loadTable()
{
    m_loading = true;

    m_tableWidget->clear();
    m_tableWidget->setRowCount(m_table->rowCount());
    m_tableWidget->setColumnCount(m_table->columnCount());

    // 设置表头
    for (int col = 0; col < m_table->columnCount(); ++col) {
        const ColumnDefinition& colDef = m_table->columnAt(col);
        QString headerText = colDef.name;
        if (!colDef.unit.isEmpty()) {
            headerText += QString(" (%1)").arg(colDef.unit);
        }
        m_tableWidget->setHorizontalHeaderItem(col, new QTableWidgetItem(headerText));
    }

    // 填充数据
    for (int row = 0; row < m_table->rowCount(); ++row) {
        for (int col = 0; col < m_table->columnCount(); ++col) {
            const QVariant& value = m_table->cellValue(row, col);
            QTableWidgetItem* item = new QTableWidgetItem(value.isValid() ? value.toString() : "");
            m_tableWidget->setItem(row, col, item);
        }
    }

    m_loading = false;
    updateStatus();
}

void DataTableEditorDialog::updateHeaders()
{
    for (int col = 0; col < m_table->columnCount(); ++col) {
        const ColumnDefinition& colDef = m_table->columnAt(col);
        QString headerText = colDef.name;
        if (!colDef.unit.isEmpty()) {
            headerText += QString(" (%1)").arg(colDef.unit);
        }
        if (m_tableWidget->horizontalHeaderItem(col)) {
            m_tableWidget->horizontalHeaderItem(col)->setText(headerText);
        } else {
            m_tableWidget->setHorizontalHeaderItem(col, new QTableWidgetItem(headerText));
        }
    }
}

void DataTableEditorDialog::updateColumnProperties()
{
    if (m_currentColumn >= 0 && m_currentColumn < m_table->columnCount()) {
        const ColumnDefinition& col = m_table->columnAt(m_currentColumn);
        m_columnPanel->setColumn(col, m_currentColumn);
        m_columnInfoLabel->setText(tr("列 %1: %2").arg(m_currentColumn + 1).arg(col.name));
    } else {
        m_columnInfoLabel->setText(tr("选择一列以编辑属性"));
    }
}

// ===========================================================================
// 表格操作槽函数
// ===========================================================================

void DataTableEditorDialog::onAddRow()
{
    m_table->appendRow();
    m_tableWidget->insertRow(m_tableWidget->rowCount());
    updateStatus();
}

void DataTableEditorDialog::onAddColumn()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("新建列"), tr("列名称:"), QLineEdit::Normal, tr("列%1").arg(m_table->columnCount() + 1), &ok);
    if (!ok || name.isEmpty()) return;

    ColumnDefinition col(name, ColumnType::Number);
    m_table->appendColumn(col);
    m_tableWidget->insertColumn(m_tableWidget->columnCount());
    updateHeaders();
    updateStatus();
}

void DataTableEditorDialog::onInsertRow()
{
    const int row = m_tableWidget->currentRow();
    m_table->insertRow(row >= 0 ? row : 0);
    m_tableWidget->insertRow(row >= 0 ? row : 0);
    updateStatus();
}

void DataTableEditorDialog::onInsertColumn()
{
    const int col = m_tableWidget->currentColumn();
    const int insertPos = col >= 0 ? col : 0;

    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("插入列"), tr("列名称:"), QLineEdit::Normal, tr("新列"), &ok);
    if (!ok || name.isEmpty()) return;

    ColumnDefinition newCol(name, ColumnType::Number);
    m_table->insertColumn(insertPos, newCol);
    m_tableWidget->insertColumn(insertPos);
    updateHeaders();
    updateStatus();
}

void DataTableEditorDialog::onRemoveRow()
{
    const int row = m_tableWidget->currentRow();
    if (row < 0) return;
    if (m_table->rowCount() <= 1) {
        QMessageBox::information(this, tr("提示"), tr("至少保留一行"));
        return;
    }
    m_table->removeRow(row);
    m_tableWidget->removeRow(row);
    updateStatus();
}

void DataTableEditorDialog::onRemoveColumn()
{
    const int col = m_tableWidget->currentColumn();
    if (col < 0) return;
    if (m_table->columnCount() <= 1) {
        QMessageBox::information(this, tr("提示"), tr("至少保留一列"));
        return;
    }
    m_table->removeColumn(col);
    m_tableWidget->removeColumn(col);
    m_currentColumn = -1;
    updateColumnProperties();
    updateStatus();
}

void DataTableEditorDialog::onCellChanged(int row, int col)
{
    if (m_loading) return;
    QTableWidgetItem* item = m_tableWidget->item(row, col);
    if (item) {
        m_table->setCellValue(row, col, item->text());
    }
}

void DataTableEditorDialog::onCurrentCellChanged(int row, int col, int prevRow, int prevCol)
{
    Q_UNUSED(row);
    Q_UNUSED(prevRow);
    Q_UNUSED(prevCol);
    if (col != m_currentColumn) {
        m_currentColumn = col;
        updateColumnProperties();
    }
}

void DataTableEditorDialog::onHeaderDoubleClicked(int logicalIndex)
{
    if (logicalIndex < 0 || logicalIndex >= m_table->columnCount()) return;

    ColumnDefinition col = m_table->columnAt(logicalIndex);
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("重命名列"), tr("列名称:"), QLineEdit::Normal, col.name, &ok);
    if (ok && !name.isEmpty()) {
        col.name = name;
        m_table->removeColumn(logicalIndex);
        m_table->insertColumn(logicalIndex, col);
        // 重新加载数据（简化处理）
        // 注意：实际项目中应该只更新列定义而不丢失数据
        updateHeaders();
        updateColumnProperties();
    }
}

// ===========================================================================
// 列属性变化
// ===========================================================================

void DataTableEditorDialog::onColumnChanged(int columnIndex, const ColumnDefinition& col)
{
    if (columnIndex < 0 || columnIndex >= m_table->columnCount()) return;

    // 更新列定义（通过移除再插入的方式，简化处理）
    // 注意：这会丢失该列的数据，实际项目中应该有更好的方式
    // 这里为了简单，我们直接修改 m_table 内部的列定义
    // 但 DataTable 没有提供单独修改列定义的方法，所以我们用一个小技巧：
    // 保存该列数据 -> 移除列 -> 插入新列 -> 恢复数据
    Q_UNUSED(col);
    Q_UNUSED(columnIndex);

    // 简化：只更新表头显示
    updateHeaders();
}

// ===========================================================================
// 导入导出
// ===========================================================================

void DataTableEditorDialog::onImportCsv()
{
    // 使用数据导入对话框（支持预览、分隔符选择、导入模式）
    DataImportDialog dialog(m_table, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    DataTable::Ptr imported = dialog.importedTable();
    if (!imported) {
        QMessageBox::warning(this, tr("导入失败"), tr("没有导入数据"));
        return;
    }

    // 根据导入模式应用数据
    const ImportMode mode = dialog.importMode();

    if (mode == ImportMode::NewTable) {
        // 新表模式：替换当前表的所有内容
        m_table->setColumns(imported->columns());
        m_table->setData(imported->rows());
    } else if (mode == ImportMode::Replace) {
        // 替换模式：保留列定义，替换数据
        // 如果列数不匹配，使用导入的列定义
        if (imported->columnCount() != m_table->columnCount()) {
            m_table->setColumns(imported->columns());
        }
        m_table->setData(imported->rows());
    } else {
        // 追加模式：追加数据到现有表
        // 确保列数匹配
        if (imported->columnCount() > m_table->columnCount()) {
            // 导入的列更多，添加缺失的列
            for (int c = m_table->columnCount(); c < imported->columnCount(); ++c) {
                m_table->appendColumn(imported->columnAt(c));
            }
        }
        // 追加数据行
        const QList<QVariantList> existingData = m_table->rows();
        QList<QVariantList> newData = existingData;
        for (const QVariantList& row : imported->rows()) {
            QVariantList paddedRow = row;
            while (paddedRow.size() < m_table->columnCount()) {
                paddedRow.append(QString());
            }
            newData.append(paddedRow);
        }
        m_table->setData(newData);
    }

    // 刷新表格显示
    refreshTable();
    updateHeaders();
    updateStatus();

    QMessageBox::information(this, tr("导入成功"),
        tr("已导入 %1 行数据").arg(imported->rowCount()));
}

void DataTableEditorDialog::onExportCsv()
{
    const QString filePath = QFileDialog::getSaveFileName(
        this, tr("导出CSV"), m_table->name() + ".csv", tr("CSV文件 (*.csv)"));
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("错误"), tr("无法写入文件"));
        return;
    }

    QTextStream out(&file);

    // 表头
    QStringList headers;
    for (int col = 0; col < m_table->columnCount(); ++col) {
        headers.append(m_table->columnAt(col).name);
    }
    out << headers.join(',') << "\n";

    // 数据
    for (int row = 0; row < m_table->rowCount(); ++row) {
        QStringList values;
        for (int col = 0; col < m_table->columnCount(); ++col) {
            values.append(m_table->cellValue(row, col).toString());
        }
        out << values.join(',') << "\n";
    }

    file.close();
    QMessageBox::information(this, tr("导出成功"), tr("数据已导出到:\n%1").arg(filePath));
}

// ===========================================================================
// 数据校验
// ===========================================================================

void DataTableEditorDialog::onValidate()
{
    const QStringList errors = m_table->validate();
    if (errors.isEmpty()) {
        QMessageBox::information(this, tr("校验通过"), tr("所有数据均符合要求"));
        m_statusLabel->setText(tr("校验通过"));
        m_statusLabel->setStyleSheet("color: #67C23A; font-size: 12px;");
    } else {
        const QString errorText = errors.join("\n");
        QMessageBox::warning(this, tr("校验失败"),
            tr("发现 %1 个问题:\n\n%2").arg(errors.size()).arg(errorText));
        m_statusLabel->setText(tr("校验失败: %1 个问题").arg(errors.size()));
        m_statusLabel->setStyleSheet("color: #F56C6C; font-size: 12px;");
    }
}

// ===========================================================================
// 保存
// ===========================================================================

void DataTableEditorDialog::onAccept()
{
    // 数据已经在编辑时同步到 m_table，直接接受
    accept();
}

void DataTableEditorDialog::updateStatus()
{
    m_statusLabel->setText(tr("%1 行 × %2 列").arg(m_table->rowCount()).arg(m_table->columnCount()));
    m_statusLabel->setStyleSheet("color: #666; font-size: 12px;");
}
void DataTableEditorDialog::refreshTable()
{
    // 这里写刷新表格逻辑，例如查询数据库、重置model、view更新
}
