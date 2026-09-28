/**
 * @file ChartConfigDialog.cpp
 * @brief 图表配置对话框实现文件
 */

#include "ChartConfigDialog.h"
#include "ui_ChartConfigDialog.h"  // 由 uic 工具从 .ui 文件自动生成
#include "core/utils/Logger.h"
#include "core/utils/AppDimensions.h"

#include <QMessageBox>
#include "ui/UiHelper.h"
#include <QTabWidget>
#include <QWidget>

// ===========================================================================
// ChartConfig 序列化
// ===========================================================================

QJsonObject ChartConfig::toJson() const
{
    QJsonObject obj;
    obj["type"] = chartTypeToString(type);
    obj["title"] = title;
    obj["x_axis_title"] = xAxisTitle;
    obj["y_axis_title"] = yAxisTitle;
    obj["data_table_id"] = static_cast<qint64>(dataTableId);
    obj["x_axis_column"] = xAxisColumn;

    QJsonArray yCols;
    for (int col : yAxisColumns) {
        yCols.append(col);
    }
    obj["y_axis_columns"] = yCols;

    obj["show_legend"] = showLegend;
    obj["show_grid"] = showGrid;
    obj["show_data_points"] = showDataPoints;
    obj["theme"] = theme;
    obj["width"] = width;
    obj["height"] = height;
    return obj;
}

ChartConfig ChartConfig::fromJson(const QJsonObject& json)
{
    ChartConfig config;
    config.type = chartTypeFromString(json.value("type").toString("line"));
    config.title = json.value("title").toString();
    config.xAxisTitle = json.value("x_axis_title").toString();
    config.yAxisTitle = json.value("y_axis_title").toString();
    // 注意：dataTableId 是 qint64 类型，必须使用 toVariant().toLongLong() 读取
    // 默认值为 0（未配置），不能用 -1，因为 -1 会被误认为是表格块的负 ID
    config.dataTableId = json.value("data_table_id").toVariant().toLongLong(0);
    config.xAxisColumn = json.value("x_axis_column").toInt(0);

    if (json.value("y_axis_columns").isArray()) {
        for (const QJsonValue& val : json.value("y_axis_columns").toArray()) {
            config.yAxisColumns.append(val.toInt());
        }
    }

    config.showLegend = json.value("show_legend").toBool(true);
    config.showGrid = json.value("show_grid").toBool(true);
    config.showDataPoints = json.value("show_data_points").toBool(true);
    config.theme = json.value("theme").toString("light");
    config.width = json.value("width").toInt(600);
    config.height = json.value("height").toInt(400);
    return config;
}

QString ChartConfig::chartTypeToString(ChartType type)
{
    switch (type) {
    case ChartType::Line:    return "line";
    case ChartType::Bar:     return "bar";
    case ChartType::Pie:     return "pie";
    case ChartType::Scatter: return "scatter";
    case ChartType::Area:    return "area";
    }
    return "line";
}

ChartType ChartConfig::chartTypeFromString(const QString& str)
{
    if (str == "bar")     return ChartType::Bar;
    if (str == "pie")     return ChartType::Pie;
    if (str == "scatter") return ChartType::Scatter;
    if (str == "area")    return ChartType::Area;
    return ChartType::Line;
}

// ===========================================================================
// ChartConfigDialog 实现
// ===========================================================================

ChartConfigDialog::ChartConfigDialog(const DataTable::List& tables,
                                       const ChartConfig& config,
                                       QWidget* parent)
    : BaseDialog(parent)
    , ui(new Ui::ChartConfigDialog)  // 创建 UI 界面对象
    , m_tables(tables)
    , m_config(config)
{
    ui->setupUi(this);  // 从 .ui 文件加载界面

    // 填充图表类型下拉框（先清空 .ui 中定义的无 data 值的选项）
    ui->m_typeCombo->clear();
    ui->m_typeCombo->addItem(tr("折线图"), static_cast<int>(ChartType::Line));
    ui->m_typeCombo->addItem(tr("柱状图"), static_cast<int>(ChartType::Bar));
    ui->m_typeCombo->addItem(tr("饼图"), static_cast<int>(ChartType::Pie));
    ui->m_typeCombo->addItem(tr("散点图"), static_cast<int>(ChartType::Scatter));
    ui->m_typeCombo->addItem(tr("面积图"), static_cast<int>(ChartType::Area));

    // 填充主题下拉框（先清空 .ui 中定义的无 data 值的选项）
    ui->m_themeCombo->clear();
    ui->m_themeCombo->addItem(tr("默认"), "light");
    ui->m_themeCombo->addItem(tr("深色"), "dark");

    // 填充数据表下拉框
    for (const DataTable::Ptr& table : m_tables) {
        ui->m_tableCombo->addItem(table->name(), table->id());
    }

    // 连接信号
    connect(ui->m_tableCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartConfigDialog::onTableChanged);
    connect(ui->m_buttonBox, &QDialogButtonBox::accepted,
            this, &ChartConfigDialog::onAccept);
    connect(ui->m_buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    // 加载配置（必须在填充数据之后调用）
    loadConfig();

    setWindowTitle(tr("图表配置"));
    resize(AppDimensions::Window::DialogMediumWidth,
           AppDimensions::Window::DialogMediumHeight);
}

// ===========================================================================
// 析构函数
// ===========================================================================

ChartConfigDialog::~ChartConfigDialog()
{
    delete ui;
}

void ChartConfigDialog::loadConfig()
{
    // 类型
    const int typeIdx = ui->m_typeCombo->findData(static_cast<int>(m_config.type));
    if (typeIdx >= 0) ui->m_typeCombo->setCurrentIndex(typeIdx);

    ui->m_titleEdit->setText(m_config.title);
    ui->m_xAxisTitleEdit->setText(m_config.xAxisTitle);
    ui->m_yAxisTitleEdit->setText(m_config.yAxisTitle);

    // 数据表
    const int tableIdx = ui->m_tableCombo->findData(m_config.dataTableId);
    if (tableIdx >= 0) {
        ui->m_tableCombo->setCurrentIndex(tableIdx);
    }
    updateColumnLists();

    // X轴列
    if (m_config.xAxisColumn >= 0 && m_config.xAxisColumn < ui->m_xAxisCombo->count()) {
        ui->m_xAxisCombo->setCurrentIndex(m_config.xAxisColumn);
    }

    // Y轴列
    for (int i = 0; i < ui->m_yAxisList->count(); ++i) {
        if (m_config.yAxisColumns.contains(i)) {
            ui->m_yAxisList->item(i)->setSelected(true);
        }
    }

    // 样式
    ui->m_showLegendCheck->setChecked(m_config.showLegend);
    ui->m_showGridCheck->setChecked(m_config.showGrid);
    ui->m_showDataPointsCheck->setChecked(m_config.showDataPoints);

    const int themeIdx = ui->m_themeCombo->findData(m_config.theme);
    if (themeIdx >= 0) ui->m_themeCombo->setCurrentIndex(themeIdx);

    // 尺寸
    ui->m_widthSpin->setValue(m_config.width);
    ui->m_heightSpin->setValue(m_config.height);
}

void ChartConfigDialog::updateColumnLists()
{
    ui->m_xAxisCombo->clear();
    ui->m_yAxisList->clear();

    const int tableIdx = ui->m_tableCombo->currentIndex();
    if (tableIdx < 0 || tableIdx >= m_tables.size()) return;

    const DataTable::Ptr& table = m_tables.at(tableIdx);
    for (int col = 0; col < table->columnCount(); ++col) {
        const ColumnDefinition& colDef = table->columnAt(col);
        const QString displayName = colDef.unit.isEmpty()
            ? colDef.name
            : QString("%1 (%2)").arg(colDef.name, colDef.unit);
        ui->m_xAxisCombo->addItem(displayName, col);
        ui->m_yAxisList->addItem(displayName);
    }
}

void ChartConfigDialog::onTableChanged(int index)
{
    Q_UNUSED(index);
    updateColumnLists();
}

void ChartConfigDialog::onAccept()
{
    if (!validateConfig()) return;

    // 收集配置
    m_config.type = static_cast<ChartType>(ui->m_typeCombo->currentData().toInt());
    m_config.title = ui->m_titleEdit->text().trimmed();
    m_config.xAxisTitle = ui->m_xAxisTitleEdit->text().trimmed();
    m_config.yAxisTitle = ui->m_yAxisTitleEdit->text().trimmed();
    m_config.dataTableId = ui->m_tableCombo->currentData().toLongLong();
    m_config.xAxisColumn = ui->m_xAxisCombo->currentData().toInt();

    m_config.yAxisColumns.clear();
    for (const QListWidgetItem* item : ui->m_yAxisList->selectedItems()) {
        m_config.yAxisColumns.append(ui->m_yAxisList->row(item));
    }

    m_config.showLegend = ui->m_showLegendCheck->isChecked();
    m_config.showGrid = ui->m_showGridCheck->isChecked();
    m_config.showDataPoints = ui->m_showDataPointsCheck->isChecked();
    m_config.theme = ui->m_themeCombo->currentData().toString();
    m_config.width = ui->m_widthSpin->value();
    m_config.height = ui->m_heightSpin->value();

    accept();
}

bool ChartConfigDialog::validateConfig()
{
    if (ui->m_tableCombo->currentIndex() < 0) {
        UiHelper::warning(this, tr("输入错误"), tr("请选择数据表"));
        return false;
    }
    if (ui->m_yAxisList->selectedItems().isEmpty()) {
        UiHelper::warning(this, tr("输入错误"), tr("请至少选择一个Y轴列"));
        return false;
    }
    return true;
}

void ChartConfigDialog::onPreview()
{
    // 预览功能（后续实现）
    UiHelper::info(this, tr("预览"), tr("图表预览功能将在后续版本中实现"));
}
