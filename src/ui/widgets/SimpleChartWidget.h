#pragma once
// ===========================================================================
// 轻量自绘图表组件
// ---------------------------------------------------------------------------
// 用 QPainter 基础绘图实现 pie/bar/line/scatter 四类图表，
// 完全不依赖 QtCharts（QChart/QChartView 渲染曾出现运行期崩溃）。
// 数据为纯值，标签用基础 drawText，经过全项目 QPainter 路径验证。
// ===========================================================================

#include <QColor>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

class QPaintEvent;

class SimpleChartWidget : public QWidget {
    Q_OBJECT
public:
    struct Series {
        QString name;
        QVector<QPointF> points;   ///< 数据点（pie 取 y 为扇区值；bar 取 y 为柱高，x 为类别索引）
    };

    explicit SimpleChartWidget(QWidget* parent = nullptr);

    /// 设置图表数据；type: pie/bar/line/scatter
    void setChart(const QString& type, const QVector<Series>& series,
                  const QStringList& categories = QStringList());

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void paintPie(QPainter& p);
    void paintBars(QPainter& p);
    void paintLines(QPainter& p, bool scatter);
    void paintLegend(QPainter& p, int x, int y, int itemW);

    QString            m_type;
    QVector<Series>    m_series;
    QStringList        m_categories;
};
