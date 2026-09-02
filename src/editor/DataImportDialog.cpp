/**
 * @file DataImportDialog.cpp
 * @brief 数据导入对话框实现文件
 */

#include "DataImportDialog.h"
#include "core/utils/Logger.h"

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
    , m_filePathEdit(nullptr)
    , m_browseBtn(nullptr)
    , m_previewBtn(nullptr)
    , m_delimiterCombo(nullptr)
    , m_hasHeaderCheck(nullptr)
    , m_encodingCombo(nullptr)
    , m_previewTable(nullptr)
    , m_infoLabel(nullptr)
    , m_appendRadio(nullptr)
    , m_replaceRadio(nullptr)
    , m_newTableRadio(nullptr)
    , m_modeGroup(nullptr)
    , m_importBtn(nullptr)
    , m_cancelBtn(nullptr)
    , m_targetTable(table)
    , m_importMode(ImportMode::Append)
{
    setupUi();
    setWindowTitle(tr("导入数据"));
    resize(800, 600);
}

DataImportDialog::~DataImportDialog()
{
}

// ===========================================================================
// UI 初始化
// ===========================================================================

void DataImportDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    // -----------------------------------------------------------------------
    // 文件选择
    // -----------------------------------------------------------------------
    QGroupBox* fileGroup = new QGroupBox(tr("CSV 文件"), this);
    QVBoxLayout* fileLayout = new QVBoxLayout(fileGroup);

    QHBoxLayout* pathLayout = new QHBoxLayout();
    m_filePathEdit = new QLineEdit(this);
    m_filePathEdit->setPlaceholderText(tr("选择或输入 CSV 文件路径..."));
    pathLayout->addWidget(m_filePathEdit, 1);

    m_browseBtn = new QPushButton(tr("浏览..."), this);
    pathLayout->addWidget(m_browseBtn);

    m_previewBtn = new QPushButton(tr("预览"), this);
    pathLayout->addWidget(m_previewBtn);

    fileLayout->addLayout(pathLayout);

    // 导入选项
    QHBoxLayout* optionsLayout = new QHBoxLayout();

    optionsLayout->addWidget(new QLabel(tr("分隔符:"), this));
    m_delimiterCombo = new QComboBox(this);
    m_delimiterCombo->addItem(tr("自动检测"), QChar(0));
    m_delimiterCombo->addItem(tr("逗号 (,)"), QChar(','));
    m_delimiterCombo->addItem(tr("分号 (;)"), QChar(';'));
    m_delimiterCombo->addItem(tr("制表符 (Tab)"), QChar('\t'));
    m_delimiterCombo->addItem(tr("竖线 (|)"), QChar('|'));
    optionsLayout->addWidget(m_delimiterCombo);

    optionsLayout->addSpacing(15);
    m_hasHeaderCheck = new QCheckBox(tr("第一行是表头"), this);
    m_hasHeaderCheck->setChecked(true);
    optionsLayout->addWidget(m_hasHeaderCheck);

    optionsLayout->addStretch();
    fileLayout->addLayout(optionsLayout);

    mainLayout->addWidget(fileGroup);

    // -----------------------------------------------------------------------
    // 预览
    // -----------------------------------------------------------------------
    QGroupBox* previewGroup = new QGroupBox(tr("数据预览（前 20 行）"), this);
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);

    m_previewTable = new QTableWidget(this);
    m_previewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_previewTable->setAlternatingRowColors(true);
    m_previewTable->horizontalHeader()->setStretchLastSection(true);
    m_previewTable->setStyleSheet(
        "QTableWidget { border: 1px solid #ddd; border-radius: 4px; }"
        "QHeaderView::section { background: #f5f5f5; padding: 6px; border: none; "
        "border-bottom: 1px solid #ddd; font-weight: bold; }");
    previewLayout->addWidget(m_previewTable);

    m_infoLabel = new QLabel(tr("请选择 CSV 文件并点击预览"), this);
    m_infoLabel->setStyleSheet("color: #666; font-size: 12px; padding: 4px 0;");
    previewLayout->addWidget(m_infoLabel);

    mainLayout->addWidget(previewGroup, 1);

    // -----------------------------------------------------------------------
    // 导入模式
    // -----------------------------------------------------------------------
    QGroupBox* modeGroup = new QGroupBox(tr("导入模式"), this);
    QHBoxLayout* modeLayout = new QHBoxLayout(modeGroup);

    m_appendRadio = new QRadioButton(tr("追加到现有数据"), this);
    m_replaceRadio = new QRadioButton(tr("替换现有数据"), this);
    m_newTableRadio = new QRadioButton(tr("创建新数据表"), this);

    m_modeGroup = new QButtonGroup(this);
    m_modeGroup->addButton(m_appendRadio, 0);
    m_modeGroup->addButton(m_replaceRadio, 1);
    m_modeGroup->addButton(m_newTableRadio, 2);

    m_appendRadio->setChecked(true);

    // 如果没有目标表，只能创建新表
    if (!m_targetTable) {
        m_appendRadio->setEnabled(false);
        m_replaceRadio->setEnabled(false);
        m_newTableRadio->setChecked(true);
    }

    modeLayout->addWidget(m_appendRadio);
    modeLayout->addWidget(m_replaceRadio);
    modeLayout->addWidget(m_newTableRadio);
    modeLayout->addStretch();

    mainLayout->addWidget(modeGroup);

    // -----------------------------------------------------------------------
    // 底部按钮
    // -----------------------------------------------------------------------
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_cancelBtn = new QPushButton(tr("取消"), this);
    buttonLayout->addWidget(m_cancelBtn);

    m_importBtn = new QPushButton(tr("导入"), this);
    m_importBtn->setStyleSheet(
        "QPushButton { background: #4A90D9; color: white; padding: 6px 24px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #357ABD; }"
        "QPushButton:disabled { background: #ccc; }");
    m_importBtn->setEnabled(false);
    m_importBtn->setDefault(true);
    buttonLayout->addWidget(m_importBtn);

    mainLayout->addLayout(buttonLayout);

    // -----------------------------------------------------------------------
    // 连接信号
    // -----------------------------------------------------------------------
    connect(m_browseBtn, &QPushButton::clicked, this, &DataImportDialog::onBrowseFile);
    connect(m_previewBtn, &QPushButton::clicked, this, &DataImportDialog::onPreview);
    connect(m_delimiterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DataImportDialog::onDelimiterChanged);
    connect(m_hasHeaderCheck, &QCheckBox::stateChanged,
            this, &DataImportDialog::onHasHeaderChanged);
    connect(m_modeGroup, QOverload<int>::of(&QButtonGroup::idClicked),
            this, &DataImportDialog::onImportModeChanged);
    connect(m_importBtn, &QPushButton::clicked, this, &DataImportDialog::onImport);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

// ===========================================================================
// 文件浏览
// ===========================================================================

void DataImportDialog::onBrowseFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, tr("选择 CSV 文件"), QString(),
        tr("CSV 文件 (*.csv);;文本文件 (*.txt);;所有文件 (*)"));

    if (!filePath.isEmpty()) {
        m_filePathEdit->setText(filePath);
        m_currentFilePath = filePath;
        loadAndPreview();
    }
}

// ===========================================================================
// 预览
// ===========================================================================

void DataImportDialog::onPreview()
{
    if (m_filePathEdit->text().trimmed().isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请先选择 CSV 文件"));
        return;
    }
    m_currentFilePath = m_filePathEdit->text().trimmed();
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
    parser.setHasHeader(m_hasHeaderCheck->isChecked());

    // 设置分隔符
    const QChar delimiter = m_delimiterCombo->currentData().toChar();
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
        m_infoLabel->setText(tr("解析失败"));
        m_importBtn->setEnabled(false);
        return false;
    }

    updatePreviewTable();
    m_importBtn->setEnabled(true);

    m_infoLabel->setText(tr("共 %1 行，%2 列")
        .arg(m_parseResult.rowCount).arg(m_parseResult.columnCount));

    return true;
}

void DataImportDialog::updatePreviewTable()
{
    m_previewTable->clear();

    if (m_parseResult.rows.isEmpty()) {
        m_previewTable->setRowCount(0);
        m_previewTable->setColumnCount(0);
        return;
    }

    // 最多显示 20 行
    const int displayRows = qMin(20, m_parseResult.rowCount);
    const int cols = m_parseResult.columnCount;

    m_previewTable->setRowCount(displayRows);
    m_previewTable->setColumnCount(cols);

    // 设置表头
    if (m_hasHeaderCheck->isChecked() && m_parseResult.rowCount > 0) {
        const QStringList& headerRow = m_parseResult.rows.first();
        for (int c = 0; c < cols; ++c) {
            const QString header = c < headerRow.size() ? headerRow.at(c) : QString("列%1").arg(c + 1);
            m_previewTable->setHorizontalHeaderItem(c, new QTableWidgetItem(header));
        }
    } else {
        for (int c = 0; c < cols; ++c) {
            m_previewTable->setHorizontalHeaderItem(c, new QTableWidgetItem(QString("列%1").arg(c + 1)));
        }
    }

    // 填充数据
    const int startRow = m_hasHeaderCheck->isChecked() ? 1 : 0;
    for (int r = 0; r < displayRows && (r + startRow) < m_parseResult.rowCount; ++r) {
        const QStringList& row = m_parseResult.rows.at(r + startRow);
        for (int c = 0; c < cols; ++c) {
            const QString cell = c < row.size() ? row.at(c) : QString();
            QTableWidgetItem* item = new QTableWidgetItem(cell);
            item->setToolTip(cell);
            m_previewTable->setItem(r, c, item);
        }
    }

    m_previewTable->resizeColumnsToContents();
}

// ===========================================================================
// 选项变化
// ===========================================================================

void DataImportDialog::onDelimiterChanged(int index)
{
    Q_UNUSED(index);
    if (!m_currentFilePath.isEmpty()) {
        loadAndPreview();
    }
}

void DataImportDialog::onHasHeaderChanged(int state)
{
    Q_UNUSED(state);
    if (!m_currentFilePath.isEmpty()) {
        loadAndPreview();
    }
}

void DataImportDialog::onImportModeChanged(int id)
{
    m_importMode = static_cast<ImportMode>(id);
}

// ===========================================================================
// 导入
// ===========================================================================

void DataImportDialog::onImport()
{
    if (!m_parseResult.success || m_parseResult.rows.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("没有可导入的数据"));
        return;
    }

    applyImport();
    accept();
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

    if (m_hasHeaderCheck->isChecked() && m_parseResult.rowCount > 0) {
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
    const int startRow = m_hasHeaderCheck->isChecked() ? 1 : 0;

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
        QList<QVariantList> existingData = table->rows();
        existingData.append(data);
        table->setData(existingData);
    } else {
        // 替换或新表模式
        table->setData(data);
    }

    m_importedTable = table;

    LOG_INFO(QString("数据导入完成: %1 行, %2 列, 模式: %3")
        .arg(data.size()).arg(cols)
        .arg(m_importMode == ImportMode::Append ? "追加" :
             m_importMode == ImportMode::Replace ? "替换" : "新表"));
}
