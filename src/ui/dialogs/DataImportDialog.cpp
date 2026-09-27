/**
 * @file DataImportDialog.cpp
 * @brief 数据导入对话框实现文件
 */

#include "DataImportDialog.h"
#include "ui_DataImportDialog.h"  // 由 uic 工具从 .ui 文件自动生成
#include "core/utils/Logger.h"
#include "core/utils/AppDimensions.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QApplication>
#include <QFileInfo>

// ===========================================================================
// 构造与析构
// ===========================================================================

DataImportDialog::DataImportDialog(const DataTable::Ptr& table, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::DataImportDialog)
    , m_targetTable(table)
    , m_importMode(ImportMode::Append)
{
    ui->setupUi(this);
    setWindowTitle(tr("导入数据"));
    resize(AppDimensions::Window::DialogMediumWidth, AppDimensions::Window::DialogMediumHeight);
}

DataImportDialog::~DataImportDialog()
{
    delete ui;
}

// ===========================================================================
// 文件浏览
// ===========================================================================

void DataImportDialog::on_m_browseBtn_clicked()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, tr("选择 CSV 文件"), QString(),
        tr("CSV 文件 (*.csv);;文本文件 (*.txt);;所有文件 (*)"));

    if (!filePath.isEmpty()) {
        ui->m_filePathEdit->setText(filePath);
        m_currentFilePath = filePath;
        loadAndPreview();
    }
}

// ===========================================================================
// 预览
// ===========================================================================

void DataImportDialog::on_m_previewBtn_clicked()
{
    if (ui->m_filePathEdit->text().trimmed().isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请先选择 CSV 文件"));
        return;
    }
    m_currentFilePath = ui->m_filePathEdit->text().trimmed();
    loadAndPreview();
}

bool DataImportDialog::loadAndPreview()
{
    if (m_currentFilePath.isEmpty()) return false;

    QFileInfo fileInfo(m_currentFilePath);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, tr("错误"), tr("文件不存在: %1").arg(m_currentFilePath));
        return false;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);

    CsvParser parser;
    parser.setHasHeader(ui->m_hasHeaderCheck->isChecked());

    // 设置分隔符
    const QChar delimiter = ui->m_delimiterCombo->currentData().toChar();
    if (delimiter != QChar(0)) {
        parser.setDelimiter(delimiter);
        parser.setAutoDetect(false);
    }

    m_parseResult = parser.parseFile(m_currentFilePath);

    QApplication::restoreOverrideCursor();

    if (!m_parseResult.success) {
        QMessageBox::critical(this, tr("解析失败"),
            tr("错误: %1\n行号: %2").arg(m_parseResult.errorMessage)
                                        .arg(m_parseResult.errorLine));
        ui->m_infoLabel->setText(tr("解析失败"));
        ui->m_importBtn->setEnabled(false);
        return false;
    }

    updatePreviewTable();
    ui->m_importBtn->setEnabled(true);

    ui->m_infoLabel->setText(tr("共 %1 行，%2 列")
        .arg(m_parseResult.rowCount).arg(m_parseResult.columnCount));

    return true;
}

void DataImportDialog::updatePreviewTable()
{
    ui->m_previewTable->clear();

    if (m_parseResult.rows.isEmpty()) {
        ui->m_previewTable->setRowCount(0);
        ui->m_previewTable->setColumnCount(0);
        return;
    }

    // 最多显示 20 行
    const int displayRows = qMin(20, m_parseResult.rowCount);
    const int cols = m_parseResult.columnCount;

    ui->m_previewTable->setRowCount(displayRows);
    ui->m_previewTable->setColumnCount(cols);

    // 设置表头
    if (ui->m_hasHeaderCheck->isChecked() && m_parseResult.rowCount > 0) {
        const QStringList& headerRow = m_parseResult.rows.first();
        for (int c = 0; c < cols; ++c) {
            const QString header = c < headerRow.size() ? headerRow.at(c) : QString("列%1").arg(c + 1);
            ui->m_previewTable->setHorizontalHeaderItem(c, new QTableWidgetItem(header));
        }
    } else {
        for (int c = 0; c < cols; ++c) {
            ui->m_previewTable->setHorizontalHeaderItem(c, new QTableWidgetItem(QString("列%1").arg(c + 1)));
        }
    }

    // 填充数据
    const int startRow = ui->m_hasHeaderCheck->isChecked() ? 1 : 0;
    for (int r = 0; r < displayRows && (r + startRow) < m_parseResult.rowCount; ++r) {
        const QStringList& row = m_parseResult.rows.at(r + startRow);
        for (int c = 0; c < cols; ++c) {
            const QString cell = c < row.size() ? row.at(c) : QString();
            QTableWidgetItem* item = new QTableWidgetItem(cell);
            item->setToolTip(cell);
            ui->m_previewTable->setItem(r, c, item);
        }
    }

    ui->m_previewTable->resizeColumnsToContents();
}

// ===========================================================================
// 选项变化
// ===========================================================================

void DataImportDialog::on_m_delimiterCombo_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    if (!m_currentFilePath.isEmpty()) {
        loadAndPreview();
    }
}

void DataImportDialog::on_m_hasHeaderCheck_stateChanged(int state)
{
    Q_UNUSED(state);
    if (!m_currentFilePath.isEmpty()) {
        loadAndPreview();
    }
}

void DataImportDialog::on_m_newTableRadio_toggled(bool checked)
{
    if (checked) m_importMode = ImportMode::NewTable;
}

void DataImportDialog::on_m_appendRadio_toggled(bool checked)
{
    if (checked) m_importMode = ImportMode::Append;
}

void DataImportDialog::on_m_replaceRadio_toggled(bool checked)
{
    if (checked) m_importMode = ImportMode::Replace;
}

// ===========================================================================
// 导入
// ===========================================================================

void DataImportDialog::on_m_importBtn_clicked()
{
    if (!m_parseResult.success || m_parseResult.rows.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("没有可导入的数据"));
        return;
    }

    applyImport();
    accept();
}

void DataImportDialog::on_m_cancelBtn_clicked()
{
    reject();
}

void DataImportDialog::applyImport()
{
    // 创建或更新数据表
    DataTable::Ptr table;

    if (m_importMode == ImportMode::NewTable || !m_targetTable) {
        table = DataTable::create();
        table->setName(tr("导入的数据表"));
    } else {
        table = m_targetTable;
    }

    // 设置列定义
    const int cols = m_parseResult.columnCount;
    QList<ColumnDefinition> columns;

    if (ui->m_hasHeaderCheck->isChecked() && m_parseResult.rowCount > 0) {
        const QStringList& headerRow = m_parseResult.rows.first();
        for (int c = 0; c < cols; ++c) {
            ColumnDefinition col;
            col.name = c < headerRow.size() && !headerRow.at(c).isEmpty()
                ? headerRow.at(c) : QString("列%1").arg(c + 1);
            col.type = ColumnType::Text;  // 默认文本类型，用户可后续修改
            col.unit = "";
            col.required = false;
            columns.append(col);
        }
    } else {
        for (int c = 0; c < cols; ++c) {
            ColumnDefinition col;
            col.name = QString("列%1").arg(c + 1);
            col.type = ColumnType::Text;
            columns.append(col);
        }
    }

    table->setColumns(columns);

    // 填充数据
    QList<QVariantList> data;
    const int startRow = ui->m_hasHeaderCheck->isChecked() ? 1 : 0;

    for (int r = startRow; r < m_parseResult.rowCount; ++r) {
        const QStringList& row = m_parseResult.rows.at(r);
        QVariantList dataRow;
        for (int c = 0; c < cols; ++c) {
            dataRow.append(c < row.size() ? row.at(c) : QString());
        }
        data.append(dataRow);
    }

    if (m_importMode == ImportMode::Append && m_targetTable) {
        // 追加模式：保留现有数据，添加新数据
        // DataTable 没有 setData() 方法，使用 appendRow() 逐行添加
        for (const QVariantList& dataRow : data) {
            table->appendRow(dataRow);
        }
    } else {
        // 替换或新表模式：先清空现有行，再添加新数据
        // 从后往前删除所有现有行
        while (table->rowCount() > 0) {
            table->removeRow(table->rowCount() - 1);
        }
        // 添加新数据
        for (const QVariantList& dataRow : data) {
            table->appendRow(dataRow);
        }
    }

    m_importedTable = table;

    LOG_INFO(QString("数据导入完成: %1 行, %2 列, 模式: %3")
        .arg(data.size()).arg(cols)
        .arg(m_importMode == ImportMode::Append ? "追加" :
             m_importMode == ImportMode::Replace ? "替换" : "新表"));
}
