/**
 * @file ChartConfigDialog.cpp
 * @brief 图表配置对话框实现文件
 */

#include "ChartConfigDialog.h"
#include "core/utils/Logger.h"

#include <QMessageBox>
#include <QTabWidget>
#include <QWidget>
#include <QPushButton>

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
    config.dataTableId = json.value("data_table_id").toInt(-1);
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
    : QDialog(parent)
    , m_typeCombo(nullptr)
    , m_titleEdit(nullptr)
    , m_xAxisTitleEdit(nullptr)
    , m_yAxisTitleEdit(nullptr)
    , m_tableCombo(nullptr)
    , m_xAxisCombo(nullptr)
    , m_yAxisList(nullptr)
    , m_showLegendCheck(nullptr)
    , m_showGridCheck(nullptr)
    , m_showDataPointsCheck(nullptr)
    , m_themeCombo(nullptr)
    , m_widthSpin(nullptr)
    , m_heightSpin(nullptr)
    , m_buttonBox(nullptr)
    , m_tables(tables)
    , m_config(config)
{
    setupUi();
    loadConfig();
    setWindowTitle(tr("图表配置"));
    resize(600, 550);
}

void ChartConfigDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // -----------------------------------------------------------------------
    // 基本设置
    // -----------------------------------------------------------------------
    QGroupBox* basicGroup = new QGroupBox(tr("基本设置"), this);
    QFormLayout* basicLayout = new QFormLayout(basicGroup);

    m_typeCombo = new QComboBox(basicGroup);
    m_typeCombo->addItem(tr("折线图"), static_cast<int>(ChartType::Line));
    m_typeCombo->addItem(tr("柱状图"), static_cast<int>(ChartType::Bar));
    m_typeCombo->addItem(tr("饼图"), static_cast<int>(ChartType::Pie));
    m_typeCombo->addItem(tr("散点图"), static_cast<int>(ChartType::Scatter));
    m_typeCombo->addItem(tr("面积图"), static_cast<int>(ChartType::Area));
    basicLayout->addRow(tr("图表类型:"), m_typeCombo);

    m_titleEdit = new QLineEdit(basicGroup);
    m_titleEdit->setPlaceholderText(tr("图表标题"));
    basicLayout->addRow(tr("标题:"), m_titleEdit);

    m_xAxisTitleEdit = new QLineEdit(basicGroup);
    m_xAxisTitleEdit->setPlaceholderText(tr("X轴标题"));
    basicLayout->addRow(tr("X轴标题:"), m_xAxisTitleEdit);

    m_yAxisTitleEdit = new QLineEdit(basicGroup);
    m_yAxisTitleEdit->setPlaceholderText(tr("Y轴标题"));
    basicLayout->addRow(tr("Y轴标题:"), m_yAxisTitleEdit);

    mainLayout->addWidget(basicGroup);

    // -----------------------------------------------------------------------
    // 数据源
    // -----------------------------------------------------------------------
    QGroupBox* dataGroup = new QGroupBox(tr("数据源"), this);
    QFormLayout* dataLayout = new QFormLayout(dataGroup);

    m_tableCombo = new QComboBox(dataGroup);
    for (const DataTable::Ptr& table : m_tables) {
        m_tableCombo->addItem(table->name(), table->id());
    }
    dataLayout->addRow(tr("数据表:"), m_tableCombo);

    m_xAxisCombo = new QComboBox(dataGroup);
    dataLayout->addRow(tr("X轴列:"), m_xAxisCombo);

    m_yAxisList = new QListWidget(dataGroup);
    m_yAxisList->setSelectionMode(QAbstractItemView::MultiSelection);
    m_yAxisList->setMaximumHeight(100);
    dataLayout->addRow(tr("Y轴列(可多选):"), m_yAxisList);

    mainLayout->addWidget(dataGroup);

    // -----------------------------------------------------------------------
    // 样式与尺寸
    // -----------------------------------------------------------------------
    QHBoxLayout* styleLayout = new QHBoxLayout();

    // 样式
    QGroupBox* styleGroup = new QGroupBox(tr("样式"), this);
    QVBoxLayout* styleVLayout = new QVBoxLayout(styleGroup);

    m_showLegendCheck = new QCheckBox(tr("显示图例"), styleGroup);
    m_showGridCheck = new QCheckBox(tr("显示网格"), styleGroup);
    m_showDataPointsCheck = new QCheckBox(tr("显示数据点"), styleGroup);

    styleVLayout->addWidget(m_showLegendCheck);
    styleVLayout->addWidget(m_showGridCheck);
    styleVLayout->addWidget(m_showDataPointsCheck);

    m_themeCombo = new QComboBox(styleGroup);
    m_themeCombo->addItem(tr("浅色"), "light");
    m_themeCombo->addItem(tr("深色"), "dark");
    styleVLayout->addWidget(new QLabel(tr("主题:"), styleGroup));
    styleVLayout->addWidget(m_themeCombo);
    styleVLayout->addStretch();

    styleLayout->addWidget(styleGroup);

    // 尺寸
    QGroupBox* sizeGroup = new QGroupBox(tr("尺寸"), this);
    QFormLayout* sizeLayout = new QFormLayout(sizeGroup);

    m_widthSpin = new QSpinBox(sizeGroup);
    m_widthSpin->setRange(200, 2000);
    m_widthSpin->setSuffix(tr(" px"));
    sizeLayout->addRow(tr("宽度:"), m_widthSpin);

    m_heightSpin = new QSpinBox(sizeGroup);
    m_heightSpin->setRange(150, 1500);
    m_heightSpin->setSuffix(tr(" px"));
    sizeLayout->addRow(tr("高度:"), m_heightSpin);

    sizeLayout->addRow(new QLabel(tr("(图表将按比例缩放)"), sizeGroup));

    styleLayout->addWidget(sizeGroup);
    mainLayout->addLayout(styleLayout);

    // -----------------------------------------------------------------------
    // 按钮
    // -----------------------------------------------------------------------
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setText(tr("确定"));
    m_buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("取消"));
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &ChartConfigDialog::onAccept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(m_buttonBox);

    // -----------------------------------------------------------------------
    // 连接信号
    // -----------------------------------------------------------------------
    connect(m_tableCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChartConfigDialog::onTableChanged);
}

void ChartConfigDialog::loadConfig()
{
    // 类型
    const int typeIdx = m_typeCombo->findData(static_cast<int>(m_config.type));
    if (typeIdx >= 0) m_typeCombo->setCurrentIndex(typeIdx);

    m_titleEdit->setText(m_config.title);
    m_xAxisTitleEdit->setText(m_config.xAxisTitle);
    m_yAxisTitleEdit->setText(m_config.yAxisTitle);

    // 数据表
    const int tableIdx = m_tableCombo->findData(m_config.dataTableId);
    if (tableIdx >= 0) {
        m_tableCombo->setCurrentIndex(tableIdx);
    }
    updateColumnLists();

    // X轴列
    if (m_config.xAxisColumn >= 0 && m_config.xAxisColumn < m_xAxisCombo->count()) {
        m_xAxisCombo->setCurrentIndex(m_config.xAxisColumn);
    }

    // Y轴列
    for (int i = 0; i < m_yAxisList->count(); ++i) {
        if (m_config.yAxisColumns.contains(i)) {
            m_yAxisList->item(i)->setSelected(true);
        }
    }

    // 样式
    m_showLegendCheck->setChecked(m_config.showLegend);
    m_showGridCheck->setChecked(m_config.showGrid);
    m_showDataPointsCheck->setChecked(m_config.showDataPoints);

    const int themeIdx = m_themeCombo->findData(m_config.theme);
    if (themeIdx >= 0) m_themeCombo->setCurrentIndex(themeIdx);

    // 尺寸
    m_widthSpin->setValue(m_config.width);
    m_heightSpin->setValue(m_config.height);
}

void ChartConfigDialog::updateColumnLists()
{
    m_xAxisCombo->clear();
    m_yAxisList->clear();

    const int tableIdx = m_tableCombo->currentIndex();
    if (tableIdx < 0 || tableIdx >= m_tables.size()) return;

    const DataTable::Ptr& table = m_tables.at(tableIdx);
    for (int col = 0; col < table->columnCount(); ++col) {
        const ColumnDefinition& colDef = table->columnAt(col);
        const QString displayName = colDef.unit.isEmpty()
            ? colDef.name
            : QString("%1 (%2)").arg(colDef.name, colDef.unit);
        m_xAxisCombo->addItem(displayName, col);
        m_yAxisList->addItem(displayName);
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
    m_config.type = static_cast<ChartType>(m_typeCombo->currentData().toInt());
    m_config.title = m_titleEdit->text().trimmed();
    m_config.xAxisTitle = m_xAxisTitleEdit->text().trimmed();
    m_config.yAxisTitle = m_yAxisTitleEdit->text().trimmed();
    m_config.dataTableId = m_tableCombo->currentData().toLongLong();
    m_config.xAxisColumn = m_xAxisCombo->currentData().toInt();

    m_config.yAxisColumns.clear();
    for (const QListWidgetItem* item : m_yAxisList->selectedItems()) {
        m_config.yAxisColumns.append(m_yAxisList->row(item));
    }

    m_config.showLegend = m_showLegendCheck->isChecked();
    m_config.showGrid = m_showGridCheck->isChecked();
    m_config.showDataPoints = m_showDataPointsCheck->isChecked();
    m_config.theme = m_themeCombo->currentData().toString();
    m_config.width = m_widthSpin->value();
    m_config.height = m_heightSpin->value();

    accept();
}

bool ChartConfigDialog::validateConfig()
{
    if (m_tableCombo->currentIndex() < 0) {
        QMessageBox::warning(this, tr("输入错误"), tr("请选择数据表"));
        return false;
    }
    if (m_yAxisList->selectedItems().isEmpty()) {
        QMessageBox::warning(this, tr("输入错误"), tr("请至少选择一个Y轴列"));
        return false;
    }
    return true;
}

void ChartConfigDialog::onPreview()
{
    // 预览功能（后续实现）
    QMessageBox::information(this, tr("预览"), tr("图表预览功能将在后续版本中实现"));
}
