/**
 * @file ObjectPreviewRenderer.cpp
 * @brief 内容块预览图渲染器实现
 */

#include "ObjectPreviewRenderer.h"

#include "data/repositories/DataTableRepository.h"
#include "core/models/DataTable.h"
#include "chart/ChartRenderer.h"
#include "chart/ChartConfigDialog.h"

#include <QPainter>
#include <QImageReader>
#include <QJsonArray>
#include <QCoreApplication>
#include <QColor>
#include <QFont>
#include <QSize>

// ===========================================================================
// 公开入口
// ===========================================================================

QPixmap ObjectPreviewRenderer::render(const ContentBlock& object)
{
    switch (object.type) {
    case BlockType::Table:
    case BlockType::DataReference:
        return renderTable(object);
    case BlockType::Chart:
        return renderChart(object);
    case BlockType::Image:
        return renderImage(object);
    case BlockType::Formula:
        return renderFormula(object);
    default:
        return QPixmap();
    }
}

// ===========================================================================
// 表格预览
// ===========================================================================

QPixmap ObjectPreviewRenderer::renderTable(const ContentBlock& object)
{
    const int width = 560;
    const int rows = 4;
    const int rowHeight = 24;
    const int headerHeight = 26;

    // 表头（优先查库，旧版迁移表格回退内嵌数据）
    const qint64 tableId = static_cast<qint64>(object.data.value("tableId").toDouble());
    QStringList headers;
    QJsonArray embeddedCells;
    bool hasEmbedded = object.data.value("headers").isArray();
    if (hasEmbedded) {
        const QJsonArray headerArr = object.data.value("headers").toArray();
        for (int c = 0; c < headerArr.size(); ++c) {
            headers << headerArr.at(c).toString();
        }
        embeddedCells = object.data.value("cells").toArray();
    } else if (tableId > 0) {
        const DataTable::Ptr table = DataTableRepository::findById(tableId);
        if (table) {
            const QList<ColumnDefinition>& columnDefs = table->columns();
            for (const ColumnDefinition& col : columnDefs) {
                headers << col.name;
            }
            const QList<QVariantList>& dataRows = table->rows();
            QJsonArray arr;
            for (const QVariantList& row : dataRows) {
                QJsonArray rowArr;
                for (const QVariant& v : row) rowArr.append(v.toString());
                arr.append(rowArr);
            }
            embeddedCells = arr;
        }
    }
    const int cols = qMax(headers.size(), 2);
    while (headers.size() < cols) headers << QString("列%1").arg(headers.size() + 1);
    const int height = headerHeight + rows * rowHeight + 4;

    QPixmap pixmap(width, height);
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // 数据预览行
    QStringList rowsText;
    for (int r = 0; r < rows && r < embeddedCells.size(); ++r) {
        const QJsonArray row = embeddedCells.at(r).toArray();
        QStringList cells;
        for (int c = 0; c < cols; ++c) {
            cells << (c < row.size() ? row.at(c).toString() : QString());
        }
        rowsText << cells.join("  |  ");
    }
    while (rowsText.size() < rows) rowsText << QString();

    painter.fillRect(0, 0, width, headerHeight, QColor("#e8e8e8"));
    painter.setPen(QColor("#333"));
    QFont headerFont = painter.font();
    headerFont.setBold(true);
    painter.setFont(headerFont);
    painter.drawText(QRect(8, 0, width - 16, headerHeight),
                     Qt::AlignVCenter | Qt::AlignLeft,
                     headers.join("  |  "));

    painter.setPen(QColor("#666"));
    QFont dataFont = painter.font();
    dataFont.setBold(false);
    painter.setFont(dataFont);
    for (int r = 0; r < rows; ++r) {
        const int y = headerHeight + r * rowHeight;
        painter.setPen(QColor("#e0e0e0"));
        painter.drawLine(0, y, width, y);
        painter.setPen(QColor("#666"));
        painter.drawText(QRect(8, y, width - 16, rowHeight),
                         Qt::AlignVCenter | Qt::AlignLeft,
                         rowsText.at(r));
    }
    painter.setPen(QColor("#e0e0e0"));
    painter.drawLine(0, height - 1, width, height - 1);

    // 题注
    const QString caption = object.data.value("caption").toString();
    if (!caption.isEmpty()) {
        painter.setPen(QColor("#888"));
        painter.drawText(QRect(8, height - 20, width - 16, 18),
                         Qt::AlignRight | Qt::AlignVCenter, caption);
    }
    painter.end();
    return pixmap;
}

// ===========================================================================
// 图表预览
// ===========================================================================

QPixmap ObjectPreviewRenderer::renderChart(const ContentBlock& object)
{
    const qint64 tableId = static_cast<qint64>(object.data.value("tableId").toDouble());
    const QJsonObject configJson = object.data.value("config").toObject();

    ChartConfig config = ChartConfig::fromJson(configJson);
    ChartRenderer renderer;
    renderer.setConfig(config);
    if (tableId > 0) {
        renderer.setDataTable(DataTableRepository::findById(tableId));
    }
    if (renderer.render()) {
        QPixmap pixmap = renderer.toPixmap(560, 280);
        if (!pixmap.isNull()) return pixmap;
    }

    // 兜底占位
    QPixmap pixmap(560, 280);
    pixmap.fill(QColor("#f5f5f5"));
    QPainter painter(&pixmap);
    painter.setPen(QColor("#999"));
    painter.drawText(pixmap.rect(), Qt::AlignCenter,
                     QCoreApplication::translate("ObjectPreviewRenderer", "图表预览"));
    painter.end();
    return pixmap;
}

// ===========================================================================
// 图片预览
// ===========================================================================

QPixmap ObjectPreviewRenderer::renderImage(const ContentBlock& object)
{
    const QString path = object.data.value("path").toString();
    const int maxWidth = 560;
    const int maxHeight = 360;

    // 缩略解码：先读原始尺寸，再按目标尺寸缩放解码，避免超大原图整张载入内存
    QImageReader reader(path);
    reader.setAutoTransform(true);  // 按 EXIF 方向自动旋转
    const QSize origSize = reader.size();
    if (origSize.isValid()) {
        QSize scaledSize = origSize;
        if (origSize.width() > maxWidth || origSize.height() > maxHeight) {
            scaledSize = origSize.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio);
        }
        reader.setScaledSize(scaledSize);
        const QImage img = reader.read();
        if (!img.isNull()) {
            return QPixmap::fromImage(img);
        }
    }

    // 兜底：直接完整加载（格式受限等场景）
    QPixmap pixmap(path);
    if (pixmap.isNull()) {
        pixmap = QPixmap(560, 100);
        pixmap.fill(QColor("#f5f5f5"));
        QPainter painter(&pixmap);
        painter.setPen(QColor("#999"));
        painter.drawText(pixmap.rect(), Qt::AlignCenter,
                         QCoreApplication::translate("ObjectPreviewRenderer", "（图片加载失败）"));
        painter.end();
        return pixmap;
    }

    QPixmap scaled = pixmap;
    if (pixmap.width() > maxWidth || pixmap.height() > maxHeight) {
        scaled = pixmap.scaled(maxWidth, maxHeight,
                               Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return scaled;
}

// ===========================================================================
// 公式预览
// ===========================================================================

QPixmap ObjectPreviewRenderer::renderFormula(const ContentBlock& object)
{
    const int width = 560;
    const int height = 80;
    QPixmap pixmap(width, height);
    pixmap.fill(QColor("#faf7ef"));

    QPainter painter(&pixmap);
    painter.setPen(QColor("#444"));
    QFont font(QStringLiteral("Cambria Math"), 13);
    painter.setFont(font);
    const QString latex = object.data.value("latex").toString();
    painter.drawText(QRect(12, 0, width - 24, height),
                     Qt::AlignVCenter | Qt::AlignLeft,
                     latex.left(80));
    painter.end();
    return pixmap;
}
