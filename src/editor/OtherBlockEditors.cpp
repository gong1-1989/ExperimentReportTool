/**
 * @file OtherBlockEditors.cpp
 * @brief 其他块类型编辑器实现文件
 */

#include "OtherBlockEditors.h"
#include "core/utils/Logger.h"
#include "chart/ChartRenderer.h"
#include "chart/ChartConfigDialog.h"
#include "data/repositories/DataTableRepository.h"
#include "data/repositories/ReportRepository.h"
#include "editor/DataTableEditorDialog.h"
#include "editor/TextBlockEditor.h"  // 文本块编辑器（工厂中使用）
#include "formula/FormulaBlockEditor.h"

#include <QHeaderView>
#include <QFileInfo>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QJsonArray>
#include <QTimer>  // 用于复制按钮的延迟恢复

// ===========================================================================
// 表格块编辑器
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
    toolbar->setSpacing(4);

    m_addRowBtn = new QPushButton(tr("+ 行"), this);
    m_addColBtn = new QPushButton(tr("+ 列"), this);
    m_removeRowBtn = new QPushButton(tr("- 行"), this);
    m_removeColBtn = new QPushButton(tr("- 列"), this);

    for (QPushButton* btn : {m_addRowBtn, m_addColBtn, m_removeRowBtn, m_removeColBtn}) {
        btn->setStyleSheet("QPushButton { padding: 2px 8px; font-size: 12px; }");
        toolbar->addWidget(btn);
    }
    toolbar->addStretch();
    layout->addLayout(toolbar);

    // 表格
    m_table = new QTableWidget(3, 3, this);
    m_table->setHorizontalHeaderLabels({tr("列1"), tr("列2"), tr("列3")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(true);
    // 设置默认行高，确保数据行可见
    m_table->verticalHeader()->setDefaultSectionSize(32);
    m_table->verticalHeader()->setMinimumSectionSize(32);
    m_table->setStyleSheet("QTableWidget { border: 1px solid #ddd; gridline-color: #ddd; }");
    // 表格尺寸策略：固定高度，宽度扩展
    m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // 设置最小高度，确保表格至少显示表头和几行数据
    m_table->setMinimumHeight(150);
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

    // 加载数据后更新表格高度
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

void TableBlockEditor::updateTableHeight()
{
    // 确保所有行都有正确的高度
    for (int row = 0; row < m_table->rowCount(); ++row) {
        m_table->setRowHeight(row, 32);
    }

    // 根据行数动态计算表格高度
    // 表头高度约30px，每行32px，底部留一些边距
    const int headerHeight = m_table->horizontalHeader()->height();
    const int rowHeight = 32;
    const int margins = 10;
    const int height = headerHeight + m_table->rowCount() * rowHeight + margins;
    // 使用固定高度，确保表格完整显示
    m_table->setFixedHeight(qMax(150, height));

    // 表格高度变化后，更新块编辑器的高度
    updateHeight();
}

// ===========================================================================
// 图片块编辑器
// ===========================================================================

ImageBlockEditor::ImageBlockEditor(const ContentBlock& block, QWidget* parent)
    : BlockEditor(block, parent)
    , m_imageLabel(nullptr)
    , m_captionEdit(nullptr)
    , m_selectBtn(nullptr)
    , m_displayWidth(600)
{
    setupEditor();

    QVBoxLayout* layout = contentContainer();

    // 图片显示区域
    m_imageLabel = new QLabel(this);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    // 设置固定高度，确保图片区域可见
    m_imageLabel->setFixedHeight(200);
    m_imageLabel->setMinimumHeight(200);
    m_imageLabel->setStyleSheet("QLabel { border: 2px dashed #ccc; border-radius: 4px; "
                                  "background-color: #fafafa; color: #999; }");
    m_imageLabel->setText(tr("点击下方按钮选择图片"));
    layout->addWidget(m_imageLabel);

    // 工具栏
    QHBoxLayout* toolbar = new QHBoxLayout();
    m_selectBtn = new QPushButton(tr("选择图片"), this);
    m_captionEdit = new QLineEdit(this);
    m_captionEdit->setPlaceholderText(tr("图片说明（可选）"));
    toolbar->addWidget(m_selectBtn);
    toolbar->addWidget(m_captionEdit, 1);
    layout->addLayout(toolbar);

    connect(m_selectBtn, &QPushButton::clicked, this, &ImageBlockEditor::onSelectImage);
    connect(m_captionEdit, &QLineEdit::textChanged, this, &ImageBlockEditor::onCaptionChanged);

    if (!block.data.isEmpty()) {
        setBlockData(block.data);
    }

    // 初始更新高度
    updateHeight();
}

QJsonObject ImageBlockEditor::blockData() const
{
    QJsonObject data;
    data["path"] = m_imagePath;
    data["caption"] = m_captionEdit->text();
    data["width"] = m_displayWidth;
    return data;
}

void ImageBlockEditor::setBlockData(const QJsonObject& data)
{
    m_imagePath = data.value("path").toString();
    m_displayWidth = data.value("width").toInt(600);
    m_captionEdit->setText(data.value("caption").toString());
    updateImageDisplay();
}

void ImageBlockEditor::onSelectImage()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, tr("选择图片"), QString(),
        tr("图片文件 (*.png *.jpg *.jpeg *.gif *.bmp *.svg);;所有文件 (*)"));

    if (!filePath.isEmpty()) {
        m_imagePath = filePath;
        updateImageDisplay();
        notifyContentChanged();
    }
}

void ImageBlockEditor::onCaptionChanged()
{
    notifyContentChanged();
}

void ImageBlockEditor::updateImageDisplay()
{
    if (m_imagePath.isEmpty()) {
        m_imageLabel->setText(tr("点击下方按钮选择图片"));
        m_imageLabel->setPixmap(QPixmap());
        return;
    }

    QPixmap pixmap(m_imagePath);
    if (pixmap.isNull()) {
        m_imageLabel->setText(tr("图片加载失败: %1").arg(m_imagePath));
        return;
    }

    // 按宽度缩放
    const int maxWidth = qMin(m_displayWidth, width() - 40);
    if (pixmap.width() > maxWidth) {
        pixmap = pixmap.scaledToWidth(maxWidth, Qt::SmoothTransformation);
    }
    m_imageLabel->setPixmap(pixmap);
    m_imageLabel->setStyleSheet("QLabel { border: none; background: transparent; }");

    // 图片加载后更新块编辑器高度
    updateHeight();
}

// ===========================================================================
// 代码块编辑器
// ===========================================================================

CodeBlockEditor::CodeBlockEditor(const ContentBlock& block, QWidget* parent)
    : BlockEditor(block, parent)
    , m_codeEdit(nullptr)
    , m_languageCombo(nullptr)
    , m_copyBtn(nullptr)
    , m_language("plaintext")
{
    setupEditor();

    QVBoxLayout* layout = contentContainer();

    // 工具栏
    QHBoxLayout* toolbar = new QHBoxLayout();
    m_languageCombo = new QComboBox(this);
    m_languageCombo->addItems({
        "Plain Text", "C/C++", "Python", "Java", "JavaScript",
        "TypeScript", "Go", "Rust", "SQL", "Shell", "JSON", "XML", "HTML/CSS"
    });
    m_copyBtn = new QPushButton(tr("复制"), this);
    toolbar->addWidget(new QLabel(tr("语言:"), this));
    toolbar->addWidget(m_languageCombo);
    toolbar->addStretch();
    toolbar->addWidget(m_copyBtn);
    layout->addLayout(toolbar);

    // 代码编辑区
    m_codeEdit = new QPlainTextEdit(this);
    m_codeEdit->setFont(QFont("Consolas", 11));
    m_codeEdit->setStyleSheet(
        "QPlainTextEdit { background-color: #1e1e1e; color: #d4d4d4; "
        "border: 1px solid #333; border-radius: 4px; padding: 8px; "
        "selection-background-color: #264f78; }");
    m_codeEdit->setPlaceholderText(tr("在此输入代码..."));
    // 设置最小高度和最大高度，确保代码编辑区可见
    m_codeEdit->setMinimumHeight(120);
    m_codeEdit->setMaximumHeight(400);
    layout->addWidget(m_codeEdit);

    connect(m_copyBtn, &QPushButton::clicked, this, &CodeBlockEditor::onCopyCode);
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CodeBlockEditor::onLanguageChanged);
    connect(m_codeEdit, &QPlainTextEdit::textChanged, this, [this]() {
        notifyContentChanged();
    });

    if (!block.data.isEmpty()) {
        setBlockData(block.data);
    }

    // 初始更新高度
    updateHeight();
}

QJsonObject CodeBlockEditor::blockData() const
{
    QJsonObject data;
    data["code"] = m_codeEdit->toPlainText();
    data["language"] = m_language;
    return data;
}

void CodeBlockEditor::setBlockData(const QJsonObject& data)
{
    m_codeEdit->setPlainText(data.value("code").toString());
    m_language = data.value("language").toString("plaintext");

    // 设置语言下拉框
    const QStringList langs = {"plaintext", "cpp", "python", "java", "javascript",
                                 "typescript", "go", "rust", "sql", "shell", "json", "xml", "html"};
    const int idx = langs.indexOf(m_language);
    if (idx >= 0) m_languageCombo->setCurrentIndex(idx);
}

void CodeBlockEditor::onCopyCode()
{
    QApplication::clipboard()->setText(m_codeEdit->toPlainText());
    m_copyBtn->setText(tr("已复制!"));
    QTimer::singleShot(1500, this, [this]() { m_copyBtn->setText(tr("复制")); });
}

void CodeBlockEditor::onLanguageChanged(int index)
{
    const QStringList langs = {"plaintext", "cpp", "python", "java", "javascript",
                                 "typescript", "go", "rust", "sql", "shell", "json", "xml", "html"};
    if (index >= 0 && index < langs.size()) {
        m_language = langs[index];
    }
    notifyContentChanged();
}

// ===========================================================================
// 分割线块编辑器
// ===========================================================================

DividerBlockEditor::DividerBlockEditor(const ContentBlock& block, QWidget* parent)
    : BlockEditor(block, parent)
    , m_divider(nullptr)
{
    setupEditor();

    m_divider = new QFrame(this);
    m_divider->setFrameShape(QFrame::HLine);
    m_divider->setFrameShadow(QFrame::Sunken);
    m_divider->setStyleSheet("QFrame { color: #ddd; max-height: 2px; }");
    contentContainer()->addWidget(m_divider);

    // 分割线不需要焦点
    setFocusPolicy(Qt::NoFocus);
}

// ===========================================================================
// 图表块编辑器（占位）
// ===========================================================================

// ===========================================================================
// 图表块编辑器（使用 Qt Charts 渲染真实图表）
// ===========================================================================

ChartBlockEditor::ChartBlockEditor(const ContentBlock& block, QWidget* parent)
    : BlockEditor(block, parent)
    , m_chartContainer(nullptr)
    , m_placeholderLabel(nullptr)
    , m_configBtn(nullptr)
    , m_editDataBtn(nullptr)
    , m_renderer(nullptr)
    , m_reportId(-1)
{
    setupEditor();
    setupChartArea();

    if (!block.data.isEmpty()) {
        setBlockData(block.data);
    }

    // 初始更新高度
    updateHeight();
}

ChartBlockEditor::~ChartBlockEditor()
{
    if (m_renderer) {
        delete m_renderer;
    }
}

void ChartBlockEditor::setReportId(qint64 reportId)
{
    m_reportId = reportId;
    // 设置 reportId 后重新渲染图表
    // 因为引用表格块（负 ID）的图表需要 reportId 才能找到对应的表格块
    if (m_config.dataTableId != 0) {
        renderChart();
    }
}

void ChartBlockEditor::setupChartArea()
{
    // 图表容器
    m_chartContainer = new QWidget(this);
    // 设置固定高度，确保图表区域可见
    m_chartContainer->setFixedHeight(350);
    m_chartContainer->setMinimumHeight(350);
    m_chartContainer->setStyleSheet("QWidget { background: white; border: 1px solid #e0e0e0; border-radius: 6px; }");
    QVBoxLayout* containerLayout = new QVBoxLayout(m_chartContainer);
    containerLayout->setContentsMargins(8, 8, 8, 8);
    containerLayout->setSpacing(8);

    // 工具栏
    QHBoxLayout* toolbar = new QHBoxLayout();
    m_configBtn = new QPushButton(tr("⚙ 配置图表"), this);
    m_configBtn->setStyleSheet("QPushButton { padding: 4px 12px; font-size: 12px; background: #f5f5f5; border: 1px solid #ddd; border-radius: 4px; }"
                                 "QPushButton:hover { background: #e8e8e8; }");
    m_editDataBtn = new QPushButton(tr("📊 编辑数据"), this);
    m_editDataBtn->setStyleSheet("QPushButton { padding: 4px 12px; font-size: 12px; background: #f5f5f5; border: 1px solid #ddd; border-radius: 4px; }"
                                  "QPushButton:hover { background: #e8e8e8; }");
    toolbar->addWidget(m_configBtn);
    toolbar->addWidget(m_editDataBtn);
    toolbar->addStretch();
    containerLayout->addLayout(toolbar);

    // 占位标签（未配置时显示）
    m_placeholderLabel = new QLabel(this);
    m_placeholderLabel->setAlignment(Qt::AlignCenter);
    m_placeholderLabel->setMinimumHeight(200);
    m_placeholderLabel->setStyleSheet(
        "QLabel { background-color: #fafafa; border: 2px dashed #ddd; "
        "border-radius: 6px; color: #999; padding: 20px; font-size: 14px; }");
    m_placeholderLabel->setText(
        tr("📊 点击「配置图表」选择数据表和图表类型\n\n"
           "支持折线图、柱状图、饼图、散点图、面积图"));
    containerLayout->addWidget(m_placeholderLabel);

    contentContainer()->addWidget(m_chartContainer);

    // 连接信号
    connect(m_configBtn, &QPushButton::clicked, this, &ChartBlockEditor::onConfigureChart);
    connect(m_editDataBtn, &QPushButton::clicked, this, &ChartBlockEditor::onEditData);
}

DataTable::Ptr ChartBlockEditor::tableBlockToDataTable(const ContentBlock& block, int index) const
{
    // 创建临时 DataTable 对象
    DataTable::Ptr table = DataTable::create();
    // 使用负 ID 标识这是从表格块转换来的（-index-1，确保为负数）
    table->setId(-index - 1);
    table->setName(tr("表格块 #%1").arg(index + 1));
    table->setReportId(m_reportId);

    // 从表格块 JSON 中提取列定义
    const int cols = block.data.value("cols").toInt(0);
    QList<ColumnDefinition> columns;
    if (block.data.value("headers").isArray()) {
        const QJsonArray headers = block.data.value("headers").toArray();
        for (int col = 0; col < cols; ++col) {
            ColumnDefinition colDef;
            colDef.name = col < headers.size() ? headers[col].toString() : QString("列%1").arg(col + 1);
            colDef.type = ColumnType::Text;
            columns.append(colDef);
        }
    } else {
        for (int col = 0; col < cols; ++col) {
            ColumnDefinition colDef;
            colDef.name = QString("列%1").arg(col + 1);
            colDef.type = ColumnType::Text;
            columns.append(colDef);
        }
    }
    table->setColumns(columns);

    // 从表格块 JSON 中提取数据行
    if (block.data.value("cells").isArray()) {
        const QJsonArray cells = block.data.value("cells").toArray();
        for (int row = 0; row < cells.size(); ++row) {
            const QJsonArray rowData = cells[row].toArray();
            QVariantList variantRow;
            for (int col = 0; col < cols; ++col) {
                variantRow.append(col < rowData.size() ? rowData[col].toVariant() : QVariant());
            }
            table->appendRow(variantRow);
        }
    }

    return table;
}

DataTable::Ptr ChartBlockEditor::getDataTableById(qint64 id) const
{
    if (id > 0) {
        // 正 ID：从数据库获取数据表
        return DataTableRepository::findById(id);
    } else if (id < 0) {
        // 负 ID：从报告表格块获取
        if (m_reportId <= 0) return nullptr;

        Report::Ptr report = ReportRepository::findById(m_reportId);
        if (!report) return nullptr;

        // 计算表格块索引（id = -index-1，所以 index = -id-1）
        const int targetIndex = -id - 1;
        int tableBlockIndex = 0;

        for (const ContentBlock& block : report->blocks()) {
            if (block.type == BlockType::Table) {
                if (tableBlockIndex == targetIndex) {
                    return tableBlockToDataTable(block, targetIndex);
                }
                ++tableBlockIndex;
            }
        }
        return nullptr;
    }
}

void ChartBlockEditor::renderChart()
{
    if (m_config.dataTableId == 0) {
        m_placeholderLabel->show();
        return;
    }

    // 根据 ID 获取数据表（正 ID=数据库，负 ID=表格块）
    DataTable::Ptr table = getDataTableById(m_config.dataTableId);
    if (!table) {
        if (m_config.dataTableId > 0) {
            m_placeholderLabel->setText(tr("⚠ 数据表不存在 (ID: %1)").arg(m_config.dataTableId));
        } else {
            m_placeholderLabel->setText(tr("⚠ 表格块不存在"));
        }
        m_placeholderLabel->show();
        return;
    }

    // 创建渲染器并渲染
    if (!m_renderer) {
        m_renderer = new ChartRenderer(this);
    }

    m_renderer->setConfig(m_config);
    m_renderer->setDataTable(table);

    if (m_renderer->render()) {
        // 移除旧的图表视图
        QLayoutItem* item;
        while ((item = m_chartContainer->layout()->takeAt(2)) != nullptr) {
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }

        // 添加新的图表视图
        QChartView* view = m_renderer->chartView();
        view->setMinimumHeight(250);
        static_cast<QVBoxLayout*>(m_chartContainer->layout())->addWidget(view, 1);
        m_placeholderLabel->hide();
    } else {
        m_placeholderLabel->setText(tr("⚠ 图表渲染失败，请检查数据"));
        m_placeholderLabel->show();
    }
}

void ChartBlockEditor::onConfigureChart()
{
    // 收集所有可用的数据源（数据表 + 报告中的表格块）
    DataTable::List allDataSources;

    // 1. 从数据库获取该报告下的数据表
    if (m_reportId > 0) {
        DataTable::List dbTables = DataTableRepository::findByReport(m_reportId);
        allDataSources.append(dbTables);
    }

    // 2. 从报告中获取表格块，转换为临时 DataTable
    if (m_reportId > 0) {
        Report::Ptr report = ReportRepository::findById(m_reportId);
        if (report) {
            int tableBlockIndex = 0;
            for (const ContentBlock& block : report->blocks()) {
                if (block.type == BlockType::Table) {
                    DataTable::Ptr table = tableBlockToDataTable(block, tableBlockIndex);
                    allDataSources.append(table);
                    ++tableBlockIndex;
                }
            }
        }
    }

    if (allDataSources.isEmpty()) {
        QMessageBox::information(this, tr("提示"),
            tr("当前报告还没有可用的数据源。\n请先在报告中添加表格块，或创建数据表。"));
        return;
    }

    ChartConfigDialog dialog(allDataSources, m_config, this);
    if (dialog.exec() == QDialog::Accepted) {
        m_config = dialog.config();
        renderChart();
        notifyContentChanged();
    }
}

void ChartBlockEditor::onEditData()
{
    if (m_config.dataTableId == 0) {
        QMessageBox::information(this, tr("提示"), tr("请先配置图表，选择数据源"));
        return;
    }

    if (m_config.dataTableId > 0) {
        // 正 ID：数据库数据表，打开数据表编辑器
        DataTable::Ptr table = DataTableRepository::findById(m_config.dataTableId);
        if (!table) {
            QMessageBox::warning(this, tr("错误"), tr("数据表不存在"));
            return;
        }

        DataTableEditorDialog dialog(table, this);
        if (dialog.exec() == QDialog::Accepted) {
            // 保存数据表
            DataTableRepository::update(table);
            // 重新渲染图表
            renderChart();
            notifyContentChanged();
        }
    } else {
        // 负 ID：表格块，提示用户直接在报告中编辑
        QMessageBox::information(this, tr("提示"),
            tr("当前图表引用的是报告中的表格块。\n请直接在报告中编辑对应的表格块，图表会自动更新。"));
    }
}

QJsonObject ChartBlockEditor::blockData() const
{
    return m_config.toJson();
}

void ChartBlockEditor::setBlockData(const QJsonObject& data)
{
    m_config = ChartConfig::fromJson(data);
    renderChart();
}

// ===========================================================================
// 块编辑器工厂
// ===========================================================================

BlockEditor* BlockEditorFactory::createEditor(const ContentBlock& block, QWidget* parent)
{
    switch (block.type) {
    case BlockType::Heading1:
    case BlockType::Heading2:
    case BlockType::Heading3:
    case BlockType::Paragraph:
    case BlockType::BulletList:
    case BlockType::NumberedList:
    case BlockType::Quote:
        return new TextBlockEditor(block, parent);

    case BlockType::Table:
        return new TableBlockEditor(block, parent);

    case BlockType::Image:
        return new ImageBlockEditor(block, parent);

    case BlockType::CodeBlock:
        return new CodeBlockEditor(block, parent);

    case BlockType::Divider:
        return new DividerBlockEditor(block, parent);

    case BlockType::Chart:
    case BlockType::DataReference:
        return new ChartBlockEditor(block, parent);

    case BlockType::Formula:
        return new FormulaBlockEditor(block, parent);
    }

    // 默认返回文本块
    return new TextBlockEditor(block, parent);
}

QList<BlockType> BlockEditorFactory::supportedTypes()
{
    return {
        BlockType::Paragraph,
        BlockType::Heading1,
        BlockType::Heading2,
        BlockType::Heading3,
        BlockType::BulletList,
        BlockType::NumberedList,
        BlockType::Quote,
        BlockType::CodeBlock,
        BlockType::Table,
        BlockType::Image,
        BlockType::Chart,
        BlockType::Divider
    };
}

QString BlockEditorFactory::typeDisplayName(BlockType type)
{
    switch (type) {
    case BlockType::Paragraph:     return QObject::tr("正文");
    case BlockType::Heading1:      return QObject::tr("一级标题");
    case BlockType::Heading2:      return QObject::tr("二级标题");
    case BlockType::Heading3:      return QObject::tr("三级标题");
    case BlockType::BulletList:    return QObject::tr("无序列表");
    case BlockType::NumberedList:  return QObject::tr("有序列表");
    case BlockType::Quote:         return QObject::tr("引用");
    case BlockType::CodeBlock:     return QObject::tr("代码块");
    case BlockType::Table:         return QObject::tr("表格");
    case BlockType::Image:         return QObject::tr("图片");
    case BlockType::Chart:         return QObject::tr("图表");
    case BlockType::Divider:       return QObject::tr("分割线");
    case BlockType::Formula:       return QObject::tr("公式");
    case BlockType::DataReference: return QObject::tr("数据引用");
    }
    return QObject::tr("未知");
}
