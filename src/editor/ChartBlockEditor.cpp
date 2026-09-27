/**
 * @file ChartBlockEditor.cpp
 * @brief 图表块编辑器实现文件
 */

#include "ChartBlockEditor.h"
#include "chart/ChartRenderer.h"
#include "chart/ChartConfigDialog.h"
#include "data/repositories/DataTableRepository.h"
#include "data/repositories/ReportRepository.h"
#include "editor/DataTableEditorDialog.h"
#include "core/models/Report.h"
#include "core/models/DataTable.h"
#include "core/utils/AppTheme.h"
#include "core/utils/AppDimensions.h"
#include "core/utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QChartView>

// ===========================================================================
// 构造函数 / 析构函数
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

    updateHeight();
}

ChartBlockEditor::~ChartBlockEditor()
{
    if (m_renderer) {
        delete m_renderer;
    }
}

// ===========================================================================
// 报告 ID 设置
// ===========================================================================

void ChartBlockEditor::setReportId(qint64 reportId)
{
    m_reportId = reportId;
    if (m_config.dataTableId != 0) {
        renderChart();
    }
}

// ===========================================================================
// UI 初始化
// ===========================================================================

void ChartBlockEditor::setupChartArea()
{
    m_chartContainer = new QWidget(this);
    m_chartContainer->setFixedHeight(AppDimensions::Widget::ChartMinHeight);
    m_chartContainer->setStyleSheet(
        QString("QWidget { background: white; border: 1px solid %1; border-radius: %2px; }")
            .arg(AppTheme::Color::Border).arg(AppTheme::Radius::Medium));
    QVBoxLayout* containerLayout = new QVBoxLayout(m_chartContainer);
    containerLayout->setContentsMargins(8, 8, 8, 8);
    containerLayout->setSpacing(8);

    // 工具栏
    QHBoxLayout* toolbar = new QHBoxLayout();
    m_configBtn = new QPushButton(tr("⚙ 配置图表"), this);
    m_editDataBtn = new QPushButton(tr("📊 编辑数据"), this);
    const QString btnStyle = QString(
        "QPushButton { padding: %1px %2px; font-size: %3px; background: %4; "
        "border: 1px solid %5; border-radius: %6px; }"
        "QPushButton:hover { background: %7; }")
        .arg(AppTheme::Spacing::Small).arg(AppTheme::Spacing::Large)
        .arg(AppTheme::FontSize::Small)
        .arg(AppTheme::Color::BgGray)
        .arg(AppTheme::Color::BorderLight)
        .arg(AppTheme::Radius::Small)
        .arg(AppTheme::Color::Border);
    m_configBtn->setStyleSheet(btnStyle);
    m_editDataBtn->setStyleSheet(btnStyle);
    toolbar->addWidget(m_configBtn);
    toolbar->addWidget(m_editDataBtn);
    toolbar->addStretch();
    containerLayout->addLayout(toolbar);

    // 占位标签
    m_placeholderLabel = new QLabel(this);
    m_placeholderLabel->setAlignment(Qt::AlignCenter);
    m_placeholderLabel->setMinimumHeight(AppDimensions::Widget::ImageMinHeight);
    m_placeholderLabel->setStyleSheet(
        QString("QLabel { background-color: %1; border: 2px dashed %2; "
                "border-radius: %3px; color: %4; padding: %5px; font-size: %6px; }")
            .arg(AppTheme::Color::BgGray)
            .arg(AppTheme::Color::BorderLight)
            .arg(AppTheme::Radius::Medium)
            .arg(AppTheme::Color::TextSecondary)
            .arg(AppTheme::Spacing::Large)
            .arg(AppTheme::FontSize::Normal));
    m_placeholderLabel->setText(
        tr("📊 点击「配置图表」选择数据表和图表类型\n\n"
           "支持折线图、柱状图、饼图、散点图、面积图"));
    containerLayout->addWidget(m_placeholderLabel);

    contentContainer()->addWidget(m_chartContainer);

    connect(m_configBtn, &QPushButton::clicked, this, &ChartBlockEditor::onConfigureChart);
    connect(m_editDataBtn, &QPushButton::clicked, this, &ChartBlockEditor::onEditData);
}

// ===========================================================================
// 数据存取
// ===========================================================================

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
// 图表渲染
// ===========================================================================

DataTable::Ptr ChartBlockEditor::tableBlockToDataTable(const ContentBlock& block, int index) const
{
    DataTable::Ptr table = DataTable::create();
    table->setId(-index - 1);
    table->setName(tr("表格块 #%1").arg(index + 1));
    table->setReportId(m_reportId);

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
        return DataTableRepository::findById(id);
    } else if (id < 0) {
        if (m_reportId <= 0) return nullptr;

        Report::Ptr report = ReportRepository::findById(m_reportId);
        if (!report) return nullptr;

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
    return nullptr;
}

void ChartBlockEditor::renderChart()
{
    if (m_config.dataTableId == 0) {
        m_placeholderLabel->show();
        return;
    }

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

    if (!m_renderer) {
        m_renderer = new ChartRenderer(this);
    }

    m_renderer->setConfig(m_config);
    m_renderer->setDataTable(table);

    if (m_renderer->render()) {
        QLayoutItem* item;
        while ((item = m_chartContainer->layout()->takeAt(2)) != nullptr) {
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }

        QChartView* view = m_renderer->chartView();
        view->setMinimumHeight(250);
        static_cast<QVBoxLayout*>(m_chartContainer->layout())->addWidget(view, 1);
        m_placeholderLabel->hide();
    } else {
        m_placeholderLabel->setText(tr("⚠ 图表渲染失败，请检查数据"));
        m_placeholderLabel->show();
    }
}

// ===========================================================================
// 槽函数
// ===========================================================================

void ChartBlockEditor::onConfigureChart()
{
    DataTable::List allDataSources;

    DataTable::List globalTables = DataTableRepository::findGlobal();
    allDataSources.append(globalTables);

    if (m_reportId > 0) {
        DataTable::List dbTables = DataTableRepository::findByReport(m_reportId);
        allDataSources.append(dbTables);
    }

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
        DataTable::Ptr table = DataTableRepository::findById(m_config.dataTableId);
        if (!table) {
            QMessageBox::warning(this, tr("错误"), tr("数据表不存在"));
            return;
        }

        DataTableEditorDialog dialog(table, this);
        if (dialog.exec() == QDialog::Accepted) {
            DataTableRepository::update(table);
            renderChart();
            notifyContentChanged();
        }
    } else {
        QMessageBox::information(this, tr("提示"),
            tr("当前图表引用的是报告中的表格块。\n请直接在报告中编辑对应的表格块，图表会自动更新。"));
    }
}
