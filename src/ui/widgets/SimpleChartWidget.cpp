#include "SimpleChartWidget.h"

#include <QFontMetrics>
#include <QPainter>
#include <QtMath>

namespace {

/// 图表配色（8 色，科学风格）
const QColor kColors[] = {
    QColor(0x4E, 0x79, 0xA7), QColor(0xF2, 0x8E, 0x2B),
    QColor(0xE1, 0x57, 0x59), QColor(0x59, 0xA1, 0x4F),
    QColor(0xB6, 0x99, 0x2D), QColor(0x76, 0xB7, 0xB2),
    QColor(0xED, 0xC9, 0x48), QColor(0xFF, 0x9D, 0xA7),
};

}  // namespace

SimpleChartWidget::SimpleChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(200);
}

void SimpleChartWidget::setChart(const QString& type, const QVector<Series>& series,
                                 const QStringList& categories)
{
    m_type = type;
    m_series = series;
    m_categories = categories;
    update();
}

void SimpleChartWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setFont(font());

    if (m_type == QStringLiteral("pie")) {
        paintPie(p);
    } else if (m_type == QStringLiteral("bar")) {
        paintBars(p);
    } else if (m_type == QStringLiteral("line")) {
        paintLines(p, false);
    } else if (m_type == QStringLiteral("scatter")) {
        paintLines(p, true);
    } else {
        p.setPen(palette().windowText().color());
        p.drawText(rect(), Qt::AlignCenter, tr("暂无数据"));
    }
}

void SimpleChartWidget::paintPie(QPainter& p)
{
    // 汇总
    double total = 0.0;
    for (const Series& s : m_series) {
        if (!s.points.isEmpty()) total += s.points.first().y();
    }
    if (total <= 0.0 || m_series.isEmpty()) {
        p.setPen(palette().windowText().color());
        p.drawText(rect(), Qt::AlignCenter, tr("暂无数据"));
        return;
    }

    // 绘制区：留出顶部图例
    const int legendH = 24;
    const QRect plot = QRect(0, legendH, width(), height() - legendH);
    const int side = qMin(plot.width(), plot.height()) - 16;
    if (side <= 20) return;
    const QRectF circleRect(plot.center().x() - side / 2.0, plot.center().y() - side / 2.0,
                            side, side);
    const QPointF center = circleRect.center();
    const qreal radius = side / 2.0;

    qreal start = 90.0 * 16;  // 从 12 点方向开始，逆时针为正
    int idx = 0;
    QFontMetrics fm(font());
    for (const Series& s : m_series) {
        if (s.points.isEmpty()) continue;
        const double value = s.points.first().y();
        const qreal span = value / total * 360.0 * 16;
        p.setBrush(kColors[idx % 8]);
        p.setPen(Qt::white);
        p.drawPie(circleRect, static_cast<int>(start), static_cast<int>(-span));

        // 标签：扇区中点方向
        const qreal midAngle = qDegreesToRadians(90.0 - value / total * 360.0 / 2.0);
        const QPointF labelPos(center.x() + radius * 0.62 * qCos(midAngle),
                               center.y() - radius * 0.62 * qSin(midAngle));
        const QString text = QStringLiteral("%1 %2%")
                                 .arg(s.name)
                                 .arg(QString::number(value / total * 100.0, 'f', 1));
        const int tw = fm.horizontalAdvance(text);
        const QRectF textRect(labelPos.x() - tw / 2.0, labelPos.y() - fm.height() / 2.0,
                              tw, fm.height());
        p.setPen(kColors[idx % 8]);
        p.drawText(textRect, Qt::AlignCenter, text);

        start -= span;
        ++idx;
    }

    paintLegend(p, 8, 4, 12);
}

void SimpleChartWidget::paintBars(QPainter& p)
{
    const int legendH = 24;
    const QRect plot = QRect(50, legendH + 10, width() - 60, height() - legendH - 40);
    if (plot.height() <= 20) return;

    // y 最大值（整数向上取整）
    double maxVal = 1.0;
    for (const Series& s : m_series) {
        for (const QPointF& pt : s.points) maxVal = qMax(maxVal, pt.y());
    }
    const int yMax = static_cast<int>(qCeil(maxVal));

    // y 轴网格与刻度
    QFontMetrics fm(font());
    for (int i = 0; i <= 4; ++i) {
        const double val = yMax * i / 4.0;
        const int y = plot.bottom() - static_cast<int>(plot.height() * i / 4.0);
        p.setPen(QColor(0x88, 0x88, 0x88));
        p.drawLine(plot.left(), y, plot.right(), y);
        p.setPen(palette().windowText().color());
        p.drawText(QRect(0, y - fm.height() / 2, plot.left() - 6, fm.height()),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(static_cast<int>(qRound(val))));
    }
    p.setPen(QColor(0x88, 0x88, 0x88));
    p.drawLine(plot.bottomLeft(), plot.bottomRight());

    const int seriesCount = m_series.size();
    const int catCount = m_categories.isEmpty()
                             ? (m_series.isEmpty() ? 0 : m_series.first().points.size())
                             : m_categories.size();
    if (catCount <= 0 || seriesCount <= 0) {
        p.setPen(palette().windowText().color());
        p.drawText(rect(), Qt::AlignCenter, tr("暂无数据"));
        return;
    }
    const qreal groupW = plot.width() * 1.0 / catCount;
    const qreal barW = groupW / seriesCount * 0.7;

    for (int c = 0; c < catCount; ++c) {
        for (int s = 0; s < seriesCount; ++s) {
            if (c >= m_series.at(s).points.size()) continue;
            const double val = m_series.at(s).points.at(c).y();
            const qreal h = val / yMax * plot.height();
            const qreal x = plot.left() + c * groupW + s * (groupW / seriesCount)
                            + (groupW / seriesCount - barW) / 2.0;
            p.setBrush(kColors[s % 8]);
            p.setPen(Qt::NoPen);
            p.drawRect(QRectF(x, plot.bottom() - h, barW, h));
        }
        const QString label = m_categories.isEmpty() ? QString::number(c + 1)
                                                     : m_categories.at(c);
        p.setPen(palette().windowText().color());
        p.drawText(QRect(plot.left() + static_cast<int>(c * groupW),
                         plot.bottom() + 4, static_cast<int>(groupW), fm.height() + 4),
                   Qt::AlignHCenter | Qt::AlignTop, label);
    }

    paintLegend(p, 8, 4, 12);
}

void SimpleChartWidget::paintLines(QPainter& p, bool scatter)
{
    const int legendH = 24;
    const QRect plot = QRect(50, legendH + 10, width() - 60, height() - legendH - 40);
    if (plot.height() <= 20) return;

    // x/y 范围
    double xMin = 0, xMax = 1, yMin = 0, yMax = 1;
    bool hasData = false;
    for (const Series& s : m_series) {
        for (const QPointF& pt : s.points) {
            hasData = true;
            xMin = qMin(xMin, pt.x());
            xMax = qMax(xMax, pt.x());
            yMin = qMin(yMin, pt.y());
            yMax = qMax(yMax, pt.y());
        }
    }
    if (!hasData || m_series.isEmpty()) {
        p.setPen(palette().windowText().color());
        p.drawText(rect(), Qt::AlignCenter, tr("暂无数据"));
        return;
    }
    if (xMax == xMin) xMax = xMin + 1;
    if (yMax == yMin) yMax = yMin + 1;

    auto toPlot = [&](double x, double y) {
        return QPointF(plot.left() + (x - xMin) / (xMax - xMin) * plot.width(),
                       plot.bottom() - (y - yMin) / (yMax - yMin) * plot.height());
    };

    // 网格与刻度
    QFontMetrics fm(font());
    for (int i = 0; i <= 4; ++i) {
        const double v = yMin + (yMax - yMin) * i / 4.0;
        const int y = plot.bottom() - static_cast<int>(plot.height() * i / 4.0);
        p.setPen(QColor(0x88, 0x88, 0x88));
        p.drawLine(plot.left(), y, plot.right(), y);
        p.setPen(palette().windowText().color());
        p.drawText(QRect(0, y - fm.height() / 2, plot.left() - 6, fm.height()),
                   Qt::AlignRight | Qt::AlignVCenter, QString::number(v, 'f', 0));
    }
    p.setPen(QColor(0x88, 0x88, 0x88));
    p.drawLine(plot.bottomLeft(), plot.bottomRight());

    // 系列
    for (int s = 0; s < m_series.size(); ++s) {
        const Series& series = m_series.at(s);
        if (series.points.isEmpty()) continue;
        p.setPen(QPen(kColors[s % 8], 2));
        p.setBrush(kColors[s % 8]);
        QPolygonF poly;
        for (const QPointF& pt : series.points) poly << toPlot(pt.x(), pt.y());
        if (scatter) {
            for (const QPointF& pt : poly) p.drawEllipse(pt, 5, 5);
        } else {
            p.drawPolyline(poly);
            for (const QPointF& pt : poly) p.drawEllipse(pt, 3, 3);
        }
    }

    // x 类别标签（趋势：时间段名）
    if (!m_categories.isEmpty()) {
        const qreal groupW = plot.width() * 1.0 / m_categories.size();
        p.setPen(palette().windowText().color());
        for (int c = 0; c < m_categories.size(); ++c) {
            const double x = xMin + (xMax - xMin) * c / qMax(1, m_categories.size() - 1);
            const QPointF pos = toPlot(x, yMin);
            p.drawText(QRectF(pos.x() - groupW / 2.0, plot.bottom() + 4, groupW,
                              fm.height() + 4),
                       Qt::AlignHCenter | Qt::AlignTop, m_categories.at(c));
        }
    }

    paintLegend(p, 8, 4, 12);
}

void SimpleChartWidget::paintLegend(QPainter& p, int x, int y, int itemW)
{
    QFontMetrics fm(font());
    int curX = x;
    const int maxX = width() - 8;
    for (int s = 0; s < m_series.size(); ++s) {
        const QString name = m_series.at(s).name;
        const int tw = fm.horizontalAdvance(name);
        if (curX + 16 + tw > maxX) {
            curX = x;
            y += fm.height() + 4;
        }
        p.setBrush(kColors[s % 8]);
        p.setPen(Qt::NoPen);
        p.drawRect(curX, y + fm.height() / 2 - 5, 10, 10);
        p.setPen(palette().windowText().color());
        p.drawText(curX + 14, y + fm.height() - 3, name);
        curX += 14 + tw + itemW;
    }
}
