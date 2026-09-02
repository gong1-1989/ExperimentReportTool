/**
 * @file ChartRenderer.h
 * @brief 图表渲染器头文件
 *
 * 负责根据图表配置和数据表生成 Qt Charts 图表。
 * 支持折线图、柱状图、饼图、散点图、面积图。
 */

#ifndef CHART_RENDERER_H
#define CHART_RENDERER_H

#include <QWidget>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QBarSeries>
#include <QPieSeries>
#include <QScatterSeries>
#include <QAreaSeries>
#include <QBarSet>
#include <QValueAxis>
#include <QBarCategoryAxis>
#include <QPieSlice>
#include <QList>
#include <QVector>
#include <QPointF>

#include "core/models/DataTable.h"
#include "chart/ChartConfigDialog.h"

// Qt Charts 命名空间
QT_USE_NAMESPACE

/**
 * @brief 图表渲染器
 *
 * 使用方式：
 * @code
 *   ChartRenderer* renderer = new ChartRenderer(this);
 *   renderer->setConfig(config);
 *   renderer->setDataTable(table);
 *   renderer->render();
 *   layout->addWidget(renderer->chartView());
 * @endcode
 */
class ChartRenderer : public QObject
{
    Q_OBJECT

public:
    explicit ChartRenderer(QObject* parent = nullptr);
    ~ChartRenderer() override;

    /// 设置图表配置
    void setConfig(const ChartConfig& config) { m_config = config; }

    /// 设置数据表
    void setDataTable(const DataTable::Ptr& table) { m_table = table; }

    /// 渲染图表（必须先设置 config 和 dataTable）
    bool render();

    /// 获取图表视图（用于添加到布局）
    QChartView* chartView() const { return m_chartView; }

    /// 获取图表对象
    QChart* chart() const { return m_chart; }

    /// 将图表渲染为图片（用于导出）
    QPixmap toPixmap(int width = 0, int height = 0) const;

private:
    /// 创建折线图
    bool createLineChart();

    /// 创建柱状图
    bool createBarChart();

    /// 创建饼图
    bool createPieChart();

    /// 创建散点图
    bool createScatterChart();

    /// 创建面积图
    bool createAreaChart();

    /// 应用通用样式
    void applyCommonStyle();

    /// 从数据表提取 X 轴数据
    QVector<QPointF> extractSeriesData(int yColumn) const;

    /// 从数据表提取类别（用于柱状图/饼图）
    QStringList extractCategories() const;

    /// 从数据表提取数值列
    QVector<double> extractNumericColumn(int column) const;

    // -----------------------------------------------------------------------
    // 成员变量
    // -----------------------------------------------------------------------

    ChartConfig m_config;
    DataTable::Ptr m_table;
    QChart* m_chart;
    QChartView* m_chartView;
};

#endif // CHART_RENDERER_H
