#include "MediaRefProvider.h"

#include <QFile>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QUrl>

QVariantMap MediaRefProvider::defaultPayload() const
{
    return { { QStringLiteral("mediaType"), QStringLiteral("video") },
             { QStringLiteral("title"), QString() },
             { QStringLiteral("filePath"), QString() } };
}

QString MediaRefProvider::renderHtml(const QVariantMap& payload, const QString& align) const
{
    const QString mediaType = payload.value(QStringLiteral("mediaType")).toString();
    const QString title = payload.value(QStringLiteral("title")).toString();
    const QString filePath = payload.value(QStringLiteral("filePath")).toString();
    if (title.isEmpty()) return QString();

    const bool isVideo = (mediaType == QStringLiteral("video"));
    QString html;
    // 文件存在时用本地文件 URL，否则显示不可用提示
    if (QFile::exists(filePath)) {
        const QString src = QUrl::fromLocalFile(filePath).toString();
        const QString tag = isVideo ? QStringLiteral("video") : QStringLiteral("audio");
        html += QStringLiteral("<p align=\"%1\"><%2 controls src=\"%3\" style=\"max-width:100%;\">"
                               "</%2></p>").arg(align).arg(tag, src);
        html += QStringLiteral("<p align=\"%1\" style=\"color:#666;font-size:9pt;\">%2</p>")
                    .arg(align, title.toHtmlEscaped());
    } else {
        html += QStringLiteral("<p align=\"%1\" style=\"color:#b00000;font-size:10pt;\">"
                               "%2（文件缺失）</p>").arg(align, title.toHtmlEscaped());
    }
    return html;
}

QString MediaRefProvider::renderText(const QVariantMap& payload) const
{
    const QString title = payload.value(QStringLiteral("title")).toString();
    if (title.isEmpty()) return QString();
    const QString mediaType = payload.value(QStringLiteral("mediaType")).toString();
    return QStringLiteral("[%1: %2]").arg(mediaType == QStringLiteral("video")
                                              ? QStringLiteral("视频") : QStringLiteral("音频"),
                                          title);
}

QPixmap MediaRefProvider::renderPreview(const QVariantMap& payload, int width) const
{
    const QString title = payload.value(QStringLiteral("title")).toString();
    const bool isVideo = (payload.value(QStringLiteral("mediaType")).toString()
                          == QStringLiteral("video"));

    const int height = 100;
    QPixmap pm(width, height);
    pm.fill(QColor(0x10, 0x14, 0x1A));

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    // 播放三角图标
    p.setBrush(QColor(0xE0, 0xE0, 0xE0));
    p.setPen(Qt::NoPen);
    const QPointF center(width / 2.0, 40);
    p.drawPolygon(QPolygonF() << QPointF(center.x() - 10, center.y() - 16)
                              << QPointF(center.x() + 12, center.y())
                              << QPointF(center.x() - 10, center.y() + 16));
    // 类型角标
    QFont tf = p.font();
    tf.setPointSizeF(tf.pointSizeF() - 2);
    p.setFont(tf);
    p.setPen(QColor(0xCC, 0xCC, 0xCC));
    p.drawText(8, 18, isVideo ? QStringLiteral("视频") : QStringLiteral("音频"));

    // 标题
    QFontMetrics fm(p.font());
    const int textW = width - 24;
    QString name = title;
    if (fm.horizontalAdvance(name) > textW)
        name = fm.elidedText(name, Qt::ElideMiddle, textW);
    p.setPen(QColor(0xE8, 0xE8, 0xE8));
    p.drawText(12, height - 12, name);
    return pm;
}
