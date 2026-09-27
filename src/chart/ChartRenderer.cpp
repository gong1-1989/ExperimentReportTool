/**
 * @file ChartRenderer.cpp
 * @brief 图表渲染器实现文件
 */

#include "ChartRenderer.h"
#include "core/utils/Logger.h"
#include "core/utils/AppDimensions.h"

#include <QApplication>
#include <QPainter>
#include <QFont>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QLinearGradient>
#include <QTimer>
#include <QEventLoop>
#include <QGraphicsScene>
#include <QElapsedTimer>

// 预定义颜色方案
static const QList<QColor> CHART_COLORS = {
    QColor("#4A90D9"),  // 蓝色
    QColor("#67C23A"),  // 绿色
    QColor("#E6A23C"),  // 橙色
    QColor("#F56C6C"),  // 红色
    QColor("#9B59B6"),  // 紫色
    QColor("#1ABC9C"),  // 青色
    QColor("#E91E63"),  // 粉色
    QColor("#FF9800"),  // 深橙
};

// ===========================================================================
// 构造与析构
// ===========================================================================

ChartRenderer::ChartRenderer(QObject* parent)
    : QObject(parent)
    , m_chart(nullptr)
    , m_chartView(nullptr)
{
}

ChartRenderer::~ChartRenderer()
{
    // 直接删除 chartView，会自动删除 chart
    // 不要调用 hide() 或 disconnect()，可能在析构时导致问题
    delete m_chartView;
    m_chartView = nullptr;
    m_chart = nullptr;
}

// ===========================================================================
// 渲染
// ===========================================================================

bool ChartRenderer::render()
{
    if (!m_table) {
        LOG_WARNING("ChartRenderer: 数据表为空，无法渲染");
        return false;
    }

    // 删除旧图表
    if (m_chartView) {
        delete m_chartView;
        m_chartView = nullptr;
    }

    // 创建新图表
    m_chart = new QChart();
    m_chart->setTitle(m_config.title);
    // 关闭动画：离屏渲染时动画会导致数据系列尚未绘制完成
    m_chart->setAnimationOptions(QChart::NoAnimation);

    bool success = false;
    switch (m_config.type) {
    case ChartType::Line:
        success = createLineChart();
        break;
    case ChartType::Bar:
        success = createBarChart();
        break;
    case ChartType::Pie:
        success = createPieChart();
        break;
    case ChartType::Scatter:
        success = createScatterChart();
        break;
    case ChartType::Area:
        success = createAreaChart();
        break;
    }

    if (!success) {
        LOG_ERROR("ChartRenderer: 图表渲染失败");
        delete m_chart;
        m_chart = nullptr;
        return false;
    }

    applyCommonStyle();

    // 创建图表视图
    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumSize(400, 300);

    return true;
}

// ===========================================================================
// 折线图
// ===========================================================================

bool ChartRenderer::createLineChart()
{
    int colorIndex = 0;
    double minX = 0, maxX = 0, minY = 0, maxY = 0;
    bool hasData = false;

    for (int yCol : m_config.yAxisColumns) {
        if (yCol < 0 || yCol >= m_table->columnCount()) continue;

        QLineSeries* series = new QLineSeries();
        const ColumnDefinition& colDef = m_table->columnAt(yCol);
        series->setName(colDef.name);

        QPen pen(CHART_COLORS[colorIndex % CHART_COLORS.size()]);
        pen.setWidth(3);  // 加粗线条，确保可见
        series->setPen(pen);

        if (m_config.showDataPoints) {
            series->setPointsVisible(true);
            series->setMarkerSize(10);
        }

        QVector<QPointF> points = extractSeriesData(yCol);

        for (const QPointF& point : points) {
            series->append(point);
            if (!hasData) {
                minX = maxX = point.x();
                minY = maxY = point.y();
                hasData = true;
            } else {
                minX = qMin(minX, point.x());
                maxX = qMax(maxX, point.x());
                minY = qMin(minY, point.y());
                maxY = qMax(maxY, point.y());
            }
        }

        m_chart->addSeries(series);
        ++colorIndex;
    }

    // 使用 createDefaultAxes 自动创建坐标轴
    m_chart->createDefaultAxes();

    // 手动设置范围，确保折线可见
    if (hasData) {
        QList<QAbstractAxis*> axesX = m_chart->axes(Qt::Horizontal);
        QList<QAbstractAxis*> axesY = m_chart->axes(Qt::Vertical);
        if (!axesX.isEmpty()) {
            QValueAxis* ax = qobject_cast<QValueAxis*>(axesX.first());
            if (ax) {
                const double margin = (maxX - minX) * 0.1 + 0.5;
                ax->setRange(minX - margin, maxX + margin);
                ax->setTitleText(m_config.xAxisTitle);
                if (m_config.showGrid) ax->setGridLineVisible(true);
            }
        }
        if (!axesY.isEmpty()) {
            QValueAxis* ay = qobject_cast<QValueAxis*>(axesY.first());
            if (ay) {
                const double margin = (maxY - minY) * 0.1 + 0.5;
                ay->setRange(minY - margin, maxY + margin);
                ay->setTitleText(m_config.yAxisTitle);
                if (m_config.showGrid) ay->setGridLineVisible(true);
            }
        }
    }

    return colorIndex > 0;
}

// ===========================================================================
// 柱状图
// ===========================================================================

bool ChartRenderer::createBarChart()
{
    QBarSeries* series = new QBarSeries();

    int colorIndex = 0;
    for (int yCol : m_config.yAxisColumns) {
        if (yCol < 0 || yCol >= m_table->columnCount()) continue;

        const ColumnDefinition& colDef = m_table->columnAt(yCol);
        QBarSet* set = new QBarSet(colDef.name);
        set->setColor(CHART_COLORS[colorIndex % CHART_COLORS.size()]);

        const QVector<double> values = extractNumericColumn(yCol);
        for (double val : values) {
            *set << val;
        }

        series->append(set);
        ++colorIndex;
    }

    if (colorIndex == 0) return false;

    m_chart->addSeries(series);

    // X轴：类别
    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(extractCategories());
    axisX->setTitleText(m_config.xAxisTitle);
    m_chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // Y轴：数值
    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText(m_config.yAxisTitle);
    if (m_config.showGrid) axisY->setGridLineVisible(true);
    m_chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    return true;
}

// ===========================================================================
// 饼图
// ===========================================================================

bool ChartRenderer::createPieChart()
{
    if (m_config.yAxisColumns.isEmpty()) return false;

    // 饼图只使用第一个 Y 轴列
    const int yCol = m_config.yAxisColumns.first();
    if (yCol < 0 || yCol >= m_table->columnCount()) return false;

    QPieSeries* series = new QPieSeries();
    const QStringList categories = extractCategories();
    const QVector<double> values = extractNumericColumn(yCol);

    int colorIndex = 0;
    for (int i = 0; i < values.size() && i < categories.size(); ++i) {
        QPieSlice* slice = series->append(categories[i], values[i]);
        slice->setColor(CHART_COLORS[colorIndex % CHART_COLORS.size()]);
        slice->setLabelVisible(true);
        slice->setLabelPosition(QPieSlice::LabelOutside);
        ++colorIndex;
    }

    if (series->slices().isEmpty()) return false;

    m_chart->addSeries(series);

    return true;
}

// ===========================================================================
// 散点图
// ===========================================================================

bool ChartRenderer::createScatterChart()
{
    QValueAxis* axisX = new QValueAxis();
    QValueAxis* axisY = new QValueAxis();

    axisX->setTitleText(m_config.xAxisTitle);
    axisY->setTitleText(m_config.yAxisTitle);

    m_chart->addAxis(axisX, Qt::AlignBottom);
    m_chart->addAxis(axisY, Qt::AlignLeft);

    int colorIndex = 0;
    for (int yCol : m_config.yAxisColumns) {
        if (yCol < 0 || yCol >= m_table->columnCount()) continue;

        QScatterSeries* series = new QScatterSeries();
        const ColumnDefinition& colDef = m_table->columnAt(yCol);
        series->setName(colDef.name);
        series->setColor(CHART_COLORS[colorIndex % CHART_COLORS.size()]);
        series->setMarkerSize(10);
        series->setBorderColor(CHART_COLORS[colorIndex % CHART_COLORS.size()].darker(120));

        const QVector<QPointF> points = extractSeriesData(yCol);
        for (const QPointF& point : points) {
            series->append(point);
        }

        m_chart->addSeries(series);
        series->attachAxis(axisX);
        series->attachAxis(axisY);

        ++colorIndex;
    }

    return colorIndex > 0;
}

// ===========================================================================
// 面积图
// ===========================================================================

bool ChartRenderer::createAreaChart()
{
    if (m_config.yAxisColumns.isEmpty()) return false;

    QValueAxis* axisX = new QValueAxis();
    QValueAxis* axisY = new QValueAxis();

    axisX->setTitleText(m_config.xAxisTitle);
    axisY->setTitleText(m_config.yAxisTitle);

    m_chart->addAxis(axisX, Qt::AlignBottom);
    m_chart->addAxis(axisY, Qt::AlignLeft);

    int colorIndex = 0;
    for (int yCol : m_config.yAxisColumns) {
        if (yCol < 0 || yCol >= m_table->columnCount()) continue;

        // 面积图需要一个下边界线（通常是 0）
        QLineSeries* upperSeries = new QLineSeries();
        QLineSeries* lowerSeries = new QLineSeries();

        const ColumnDefinition& colDef = m_table->columnAt(yCol);
        upperSeries->setName(colDef.name);

        const QVector<QPointF> points = extractSeriesData(yCol);
        for (const QPointF& point : points) {
            upperSeries->append(point);
            lowerSeries->append(point.x(), 0);
        }

        QAreaSeries* areaSeries = new QAreaSeries(upperSeries, lowerSeries);
        QColor color = CHART_COLORS[colorIndex % CHART_COLORS.size()];
        color.setAlpha(100);
        areaSeries->setColor(color);
        areaSeries->setBorderColor(CHART_COLORS[colorIndex % CHART_COLORS.size()]);

        m_chart->addSeries(areaSeries);
        areaSeries->attachAxis(axisX);
        areaSeries->attachAxis(axisY);

        ++colorIndex;
    }

    return colorIndex > 0;
}

// ===========================================================================
// 通用样式
// ===========================================================================

void ChartRenderer::applyCommonStyle()
{
    // 标题字体
    QFont titleFont = m_chart->titleFont();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    m_chart->setTitleFont(titleFont);

    // 图例
    if (m_config.showLegend) {
        m_chart->legend()->setVisible(true);
        m_chart->legend()->setAlignment(Qt::AlignBottom);
        m_chart->legend()->setFont(QFont("Arial", 10));
    } else {
        m_chart->legend()->setVisible(false);
    }

    // 主题
    if (m_config.theme == "dark") {
        m_chart->setTheme(QChart::ChartThemeDark);
    } else {
        m_chart->setTheme(QChart::ChartThemeLight);
    }

    // 边距
    m_chart->setMargins(QMargins(20, 20, 20, 20));
}

// ===========================================================================
// 数据提取
// ===========================================================================

QVector<QPointF> ChartRenderer::extractSeriesData(int yColumn) const
{
    QVector<QPointF> points;

    // X 轴使用行索引（0, 1, 2, ...）
    // 如果 X 轴列是数值类型，也可以用该列的值
    const bool useXColumn = m_config.xAxisColumn >= 0
        && m_config.xAxisColumn < m_table->columnCount()
        && m_table->columnAt(m_config.xAxisColumn).type == ColumnType::Number;

    for (int row = 0; row < m_table->rowCount(); ++row) {
        double x = useXColumn
            ? m_table->cellValue(row, m_config.xAxisColumn).toDouble()
            : static_cast<double>(row);

        bool ok = false;
        const double y = m_table->cellValue(row, yColumn).toDouble(&ok);
        if (ok) {
            points.append(QPointF(x, y));
        }
    }

    return points;
}

QStringList ChartRenderer::extractCategories() const
{
    QStringList categories;

    if (m_config.xAxisColumn >= 0 && m_config.xAxisColumn < m_table->columnCount()) {
        for (int row = 0; row < m_table->rowCount(); ++row) {
            categories.append(m_table->cellValue(row, m_config.xAxisColumn).toString());
        }
    } else {
        for (int row = 0; row < m_table->rowCount(); ++row) {
            categories.append(QString::number(row + 1));
        }
    }

    return categories;
}

QVector<double> ChartRenderer::extractNumericColumn(int column) const
{
    return m_table->numericColumn(column);
}

// ===========================================================================
// 导出为图片
// ===========================================================================

QPixmap ChartRenderer::toPixmap(int width, int height)
{
    if (!m_chartView || !m_chart) return QPixmap();

    const int w = width > 0 ? width : m_config.width;
    const int h = height > 0 ? height : m_config.height;

    // ========================================================================
    // 使用 QChartView::grab() 离屏渲染
    // QWidget::grab() 不需要 widget 可见，会渲染到离屏表面
    //
    // 关键：QChart 的坐标轴、系列等子元素需要多次布局才能正确定位。
    // 经验证，需要等待约 RenderWaitMs 让 QChart 完成所有异步布局和渲染，
    // 否则只会显示坐标轴而不显示折线/柱状图。
    // ========================================================================

    // 设置图表视图和图表的大小
    m_chartView->resize(w, h);
    m_chart->setGeometry(QRectF(0, 0, w, h));

    // 强制初始布局更新
    m_chartView->ensurePolished();
    m_chart->update();
    m_chartView->update();

    // 等待 RenderWaitMs，期间持续处理事件让 QChart 完成布局
    // 这是关键：QChart 的系列渲染是异步的，必须给足够时间
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < AppDimensions::Chart::RenderWaitMs) {
        QApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
        QApplication::sendPostedEvents(nullptr, QEvent::Paint);
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 10);
        m_chart->update();
        m_chartView->update();
    }

    // 最后再强制刷新一次
    m_chartView->repaint();
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 50);

    // 使用 grab 渲染整个图表视图
    QPixmap pixmap = m_chartView->grab(QRect(0, 0, w, h));

    // 安全检查：如果 pixmap 为空或尺寸不对，用 QPainter 手动渲染作为后备
    if (pixmap.isNull() || pixmap.width() != w || pixmap.height() != h) {
        LOG_WARNING("ChartRenderer: grab() 失败，使用 QGraphicsScene 后备渲染");

        // 后备方案：创建独立场景渲染
        // 注意：不能直接把 m_chart 加入新场景，因为它已属于 chartView 的场景
        // 所以这里只填充背景，确保不返回空
        pixmap = QPixmap(w, h);
        pixmap.fill(Qt::white);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.drawText(QRect(0, 0, w, h), Qt::AlignCenter,
                         QString("图表渲染失败\n%1").arg(m_config.title));
        painter.end();
    }

    return pixmap;
}
