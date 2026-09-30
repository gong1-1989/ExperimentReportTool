/**
 * @file DataImportDialog.cpp
 * @brief 数据导入对话框实现文件
 */

#include "DataImportDialog.h"
#include "ui_DataImportDialog.h"  // 由 uic 工具从 .ui 文件自动生成
#include "core/utils/Logger.h"
#include "core/utils/AppDimensions.h"
#include "import/ImportAdapter.h"
#include "import/ImportRegistry.h"

#include <QFileDialog>
#include <QMessageBox>
#include "ui/UiHelper.h"
#include <QHeaderView>
#include <QApplication>
#include <QFileInfo>
#include <QColor>
#include <QFile>
#include <QSet>
#include <QPair>

// ===========================================================================
// 构造与析构
// ===========================================================================

DataImportDialog::DataImportDialog(const DataTable::Ptr& table, QWidget* parent)
    : BaseDialog(parent)
    , ui(new Ui::DataImportDialog)
    , m_targetTable(table)
    , m_importMode(ImportMode::Append)
{
    ui->setupUi(this);
    // 表格内容与表头居中
    UiHelper::centerTableWidget(ui->m_previewTable);
    setWindowTitle(tr("导入数据"));
    resize(AppDimensions::Window::DialogMediumWidth, AppDimensions::Window::DialogMediumHeight);

    // 后台解析监视器：解析在 QtConcurrent 线程池执行，完成后回主线程更新预览
    m_watcher = new QFutureWatcher<ParseOutcome>(this);
    connect(m_watcher, &QFutureWatcher<ParseOutcome>::finished,
            this, &DataImportDialog::onParseFinished);
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
        UiHelper::info(this, tr("提示"), tr("请先选择 CSV 文件"));
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
        UiHelper::warning(this, tr("错误"), tr("文件不存在: %1").arg(m_currentFilePath));
        return false;
    }

    // 按扩展名选择导入适配器（统一契约：内置 CSV，未来插件格式同样走此入口）
    ImportAdapter* adapter = ImportRegistry::adapterForFile(m_currentFilePath);
    if (!adapter) {
        UiHelper::warning(this, tr("错误"),
                          tr("不支持的文件格式: %1")
                              .arg(QFileInfo(m_currentFilePath).suffix()));
        return false;
    }

    // 异步解析：解析（纯函数，不碰 UI/数据库）在后台线程执行，
    // 大文件不再卡界面；setFuture 替换后旧任务完成信号不再触发，天然只响应最新一次
    const QString filePath = m_currentFilePath;                   // 快照（后台线程安全使用）
    const QChar delimiter = ui->m_delimiterCombo->currentData().toChar();

    ui->m_infoLabel->setText(tr("正在解析..."));
    ui->m_importBtn->setEnabled(false);
    ui->m_previewBtn->setEnabled(false);
    QApplication::setOverrideCursor(Qt::WaitCursor);

    m_watcher->setFuture(QtConcurrent::run(
        [filePath, delimiter, adapter]() -> ParseOutcome {
            ParseOutcome out;
            QHash<QString, QVariant> options;
            if (!delimiter.isNull()) {
                options.insert(QStringLiteral("delimiter"), delimiter);
            }
            out.data = adapter->parse(filePath, &out.errorMessage, options);
            return out;
        }));
    return true;
}

void DataImportDialog::onParseFinished()
{
    QApplication::restoreOverrideCursor();
    ui->m_previewBtn->setEnabled(true);

    const ParseOutcome outcome = m_watcher->result();
    m_importData = outcome.data;

    // 表头语义由界面复选框决定（适配器只负责解析，不替调用方判断）
    m_importData.hasHeader = ui->m_hasHeaderCheck->isChecked();
    if (m_importData.hasHeader && !m_importData.rows.isEmpty()) {
        m_importData.headers = m_importData.rows.first();
    }

    if (m_importData.isEmpty()) {
        UiHelper::error(this, tr("解析失败"),
                        outcome.errorMessage.isEmpty()
                            ? tr("文件为空或格式无法识别")
                            : outcome.errorMessage);
        ui->m_infoLabel->setText(tr("解析失败"));
        ui->m_importBtn->setEnabled(false);
        return;
    }

    runValidation();
    updatePreviewTable();
    ui->m_importBtn->setEnabled(true);

    // 状态栏：行/列 + 校验汇总
    QString info = tr("共 %1 行，%2 列")
        .arg(m_importData.rowCount).arg(m_importData.columnCount);
    if (m_importMode == ImportMode::NewTable || !m_targetTable) {
        info += tr("（新表模式：不做类型校验，列默认文本类型）");
    } else {
        if (!m_missingRequired.isEmpty())
            info += tr("；缺少必填列: %1").arg(m_missingRequired.join(QStringLiteral(", ")));
        if (!m_unknownColumns.isEmpty())
            info += tr("；未知列已忽略: %1").arg(m_unknownColumns.join(QStringLiteral(", ")));
        int errorCount = 0;
        for (const CellIssue& issue : m_issues) {
            if (issue.level == CellIssue::Level::Error) ++errorCount;
        }
        if (errorCount > 0)
            info += tr("；%1 处错误（红色标出，导入被阻止）").arg(errorCount);
    }
    ui->m_infoLabel->setText(info);
}

void DataImportDialog::updatePreviewTable()
{
    ui->m_previewTable->clear();

    if (m_importData.rows.isEmpty()) {
        ui->m_previewTable->setRowCount(0);
        ui->m_previewTable->setColumnCount(0);
        return;
    }

    const bool hasHeader = ui->m_hasHeaderCheck->isChecked();
    const int startRow = hasHeader ? 1 : 0;
    const int totalDataRows = m_importData.rowCount - startRow;
    const int cols = m_importData.columnCount;
    const int displayRows = qMin(20, qMax(0, totalDataRows));

    ui->m_previewTable->setRowCount(displayRows);
    ui->m_previewTable->setColumnCount(cols);

    // 表头：文件表头行 或 列N
    if (hasHeader && !m_importData.rows.isEmpty()) {
        const QStringList& headerRow = m_importData.rows.first();
        for (int c = 0; c < cols; ++c) {
            const QString header = c < headerRow.size() && !headerRow.at(c).isEmpty()
                ? headerRow.at(c) : QString("列%1").arg(c + 1);
            ui->m_previewTable->setHorizontalHeaderItem(c, new QTableWidgetItem(header));
        }
    } else {
        for (int c = 0; c < cols; ++c) {
            ui->m_previewTable->setHorizontalHeaderItem(c, new QTableWidgetItem(QString("列%1").arg(c + 1)));
        }
    }

    // 错误单元格索引（预览行号、文件列号）→ 标红
    QSet<QPair<int, int>> errorCells;
    for (const CellIssue& issue : m_issues) {
        if (issue.level == CellIssue::Level::Error) {
            errorCells.insert(qMakePair(issue.row, issue.col));
        }
    }

    // 填充数据（最多 20 行）
    for (int r = 0; r < displayRows; ++r) {
        const QStringList& row = m_importData.rows.at(startRow + r);
        for (int c = 0; c < cols; ++c) {
            const QString cell = c < row.size() ? row.at(c) : QString();
            QTableWidgetItem* item = new QTableWidgetItem(cell);
            item->setToolTip(cell);
            if (errorCells.contains(qMakePair(r, c))) {
                item->setBackground(QColor(255, 200, 200));
                item->setForeground(QColor(160, 0, 0));
                QStringList msgs;
                for (const CellIssue& issue : m_issues) {
                    if (issue.row == r && issue.col == c) msgs << issue.message;
                }
                item->setToolTip(msgs.join(QStringLiteral("\n")));
            }
            ui->m_previewTable->setItem(r, c, item);
        }
    }

    ui->m_previewTable->resizeColumnsToContents();
}

// ===========================================================================
// 校验
// ===========================================================================

void DataImportDialog::runValidation()
{
    m_columnMapping.clear();
    m_issues.clear();
    m_missingRequired.clear();
    m_unknownColumns.clear();

    // 新表模式：无目标列定义 → 跳过强校验（界面已注明）
    if (m_importMode == ImportMode::NewTable || !m_targetTable) return;

    const bool hasHeader = ui->m_hasHeaderCheck->isChecked();
    const QList<ColumnDefinition> targetCols = m_targetTable->columns();

    if (hasHeader) {
        // 按列名匹配：乱序可导、未知列忽略、缺必填列报错
        m_columnMapping = ImportValidator::buildColumnMapping(
            m_importData.headers, targetCols, &m_missingRequired, &m_unknownColumns);
    } else {
        // 无表头：按顺序位置对应目标列
        const int n = qMin(m_importData.columnCount, targetCols.size());
        for (int c = 0; c < n; ++c) m_columnMapping.insert(c, c);
    }

    m_issues = ImportValidator::validate(m_importData, hasHeader, targetCols, m_columnMapping);
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
    if (m_importData.isEmpty()) {
        UiHelper::warning(this, tr("提示"), tr("没有可导入的数据"));
        return;
    }

    // 校验拦截：Error 级问题阻止导入（预览中已红色标出）
    if (m_importMode != ImportMode::NewTable && m_targetTable) {
        int errorCount = 0;
        for (const CellIssue& issue : m_issues) {
            if (issue.level == CellIssue::Level::Error) ++errorCount;
        }
        if (errorCount > 0) {
            UiHelper::warning(this, tr("导入被阻止"),
                              tr("文件存在 %1 处错误（预览中红色标出）\n"
                                 "请修正后重新预览导入，或点击“导出模板”对照填写。")
                                  .arg(errorCount));
            return;
        }
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
    const bool hasHeader = ui->m_hasHeaderCheck->isChecked();
    const int startRow = hasHeader ? 1 : 0;

    // 创建或更新数据表
    DataTable::Ptr table;
    QList<QVariantList> data;

    if (m_importMode == ImportMode::NewTable || !m_targetTable) {
        // 新表：按文件结构建列（默认 Text，无目标列定义 → 不做强校验）
        table = DataTable::create();
        table->setReportId(m_targetTable ? m_targetTable->reportId() : -1);
        table->setName(tr("导入的数据表"));

        QList<ColumnDefinition> columns;
        const QStringList& headerRow = hasHeader && !m_importData.rows.isEmpty()
            ? m_importData.rows.first() : QStringList();
        for (int c = 0; c < m_importData.columnCount; ++c) {
            ColumnDefinition col;
            col.name = hasHeader && c < headerRow.size() && !headerRow.at(c).trimmed().isEmpty()
                ? headerRow.at(c).trimmed() : QString("列%1").arg(c + 1);
            col.type = ColumnType::Text;
            columns.append(col);
        }
        table->setColumns(columns);

        // 数据按文件列顺序
        for (int r = startRow; r < m_importData.rows.size(); ++r) {
            const QStringList& row = m_importData.rows.at(r);
            QVariantList dataRow;
            for (int c = 0; c < m_importData.columnCount; ++c) {
                dataRow.append(c < row.size() ? row.at(c) : QString());
            }
            data.append(dataRow);
        }
    } else {
        // 追加/替换：按目标列定义顺序 + 列映射（按列名，乱序可导；未知列已忽略）
        table = m_targetTable;
        const QList<ColumnDefinition> targetCols = table->columns();
        for (int r = startRow; r < m_importData.rows.size(); ++r) {
            const QStringList& row = m_importData.rows.at(r);
            QVariantList dataRow;
            for (int t = 0; t < targetCols.size(); ++t) {
                int src = -1;
                for (auto it = m_columnMapping.constBegin(); it != m_columnMapping.constEnd(); ++it) {
                    if (it.value() == t) { src = it.key(); break; }
                }
                dataRow.append(src >= 0 && src < row.size() ? row.at(src) : QString());
            }
            data.append(dataRow);
        }
    }

    if (m_importMode == ImportMode::Append && m_targetTable) {
        // 追加模式：保留现有数据，添加新数据
        // DataTable 没有 setData() 方法，使用 appendRow() 逐行添加
        for (const QVariantList& dataRow : data) {
            table->appendRow(dataRow);
        }
    } else {
        // 替换或新表模式：先清空现有行，再添加新数据
        while (table->rowCount() > 0) {
            table->removeRow(table->rowCount() - 1);
        }
        for (const QVariantList& dataRow : data) {
            table->appendRow(dataRow);
        }
    }

    m_importedTable = table;
}

// ===========================================================================
// 标准模板导出
// ===========================================================================

void DataImportDialog::on_m_exportTemplateBtn_clicked()
{
    if (!m_targetTable) {
        UiHelper::info(this, tr("导出模板"),
                       tr("当前为“创建新表”模式，没有目标列定义可导出\n"
                          "请先在数据表编辑器中设置列定义，再导出标准模板。"));
        return;
    }

    // 获取 CSV 适配器（模板随适配器：未来其他格式由各自适配器生成模板）
    ImportAdapter* csvAdapter = nullptr;
    for (ImportAdapter* a : ImportRegistry::allAdapters()) {
        if (a->extensions().contains(QStringLiteral("csv"))) { csvAdapter = a; break; }
    }
    if (!csvAdapter) return;

    const QList<ColumnDefinition> columns = m_targetTable->columns();
    QString defaultName = tr("%1_导入模板.csv")
        .arg(m_targetTable->name().trimmed().isEmpty() ? QStringLiteral("数据表") : m_targetTable->name().trimmed());
    const QString path = QFileDialog::getSaveFileName(this, tr("导出标准模板"), defaultName, csvAdapter->fileFilter());
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        UiHelper::error(this, tr("导出失败"), file.errorString());
        return;
    }
    file.write(csvAdapter->buildTemplate(columns));
    file.close();

    // 填写规则说明
    QStringList tips;
    tips << tr("模板已导出：%1").arg(path);
    tips << tr("第一行为列名（含单位），第二行为合规示例。");
    tips << tr("填写规则：");
    for (const ColumnDefinition& col : columns) {
        QString rule = tr("· %1：%2").arg(col.name)
                          .arg(ColumnDefinition::typeToString(col.type));
        if (col.required) rule += tr("（必填）");
        if (col.type == ColumnType::Number) {
            rule += tr("，范围 [%1, %2]")
                .arg(col.minValue, 0, 'g', 6).arg(col.maxValue, 0, 'g', 6);
        }
        tips << rule;
    }
    UiHelper::info(this, tr("导出模板"), tips.join(QStringLiteral("\n")));
}
