#include "AttachmentCardProvider.h"

#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QUrl>

namespace {

QString humanSize(qint64 bytes)
{
    if (bytes < 1024) return QStringLiteral("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024LL * 1024 * 1024)
        return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    return QStringLiteral("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
}

QString mimeLabel(const QString& mime)
{
    if (mime.startsWith(QStringLiteral("image/"))) return QStringLiteral("图片");
    if (mime.startsWith(QStringLiteral("video/"))) return QStringLiteral("视频");
    if (mime.startsWith(QStringLiteral("audio/"))) return QStringLiteral("音频");
    if (mime == QStringLiteral("application/pdf")) return QStringLiteral("PDF");
    if (mime.contains(QStringLiteral("spreadsheet"))
        || mime == QStringLiteral("application/vnd.ms-excel")) return QStringLiteral("表格");
    if (mime.contains(QStringLiteral("word"))
        || mime == QStringLiteral("application/msword")) return QStringLiteral("文档");
    if (mime.contains(QStringLiteral("zip")) || mime.contains(QStringLiteral("compressed")))
        return QStringLiteral("压缩包");
    return QStringLiteral("文件");
}

}  // namespace

QVariantMap AttachmentCardProvider::defaultPayload() const
{
    return { { QStringLiteral("fileName"), QString() },
             { QStringLiteral("filePath"), QString() },
             { QStringLiteral("fileSize"), qint64(0) },
             { QStringLiteral("mimeType"), QString() },
             { QStringLiteral("uploadedAt"), QString() },
             { QStringLiteral("caption"), QString() } };
}

QString AttachmentCardProvider::renderHtml(const QVariantMap& payload, const QString& align) const
{
    const QString fileName = payload.value(QStringLiteral("fileName")).toString();
    const QString filePath = payload.value(QStringLiteral("filePath")).toString();
    if (fileName.isEmpty()) return QString();

    const qint64 fileSize = payload.value(QStringLiteral("fileSize")).toLongLong();
    const QString mime = payload.value(QStringLiteral("mimeType")).toString();
    const QString caption = payload.value(QStringLiteral("caption")).toString();

    // 链接地址：文件存在时用本地文件 URL，否则只展示信息不链接
    QString href;
    if (QFile::exists(filePath)) {
        href = QUrl::fromLocalFile(filePath).toString();
    }
    const QString sizeText = humanSize(fileSize);
    const QString typeText = mimeLabel(mime);

    QString card = QStringLiteral(
        "<div class=\"att-card\" align=\"%1\" style=\"display:inline-block;border:1px solid #d0d5dd;"
        "border-radius:6px;padding:8px 12px;margin:4px 0;background:#f7f9fc;\">").arg(align);
    if (!href.isEmpty()) {
        card += QStringLiteral("<a href=\"%1\" style=\"text-decoration:none;color:#1f4e79;\">")
                    .arg(href.toHtmlEscaped());
    }
    card += QStringLiteral("<span style=\"font-weight:600;font-size:11pt;\">%1</span>")
                .arg(fileName.toHtmlEscaped());
    if (!href.isEmpty()) card += QStringLiteral("</a>");
    card += QStringLiteral("<div style=\"color:#666;font-size:9pt;margin-top:2px;\">%1 · %2</div>")
                .arg(typeText, sizeText);
    if (!caption.isEmpty()) {
        card += QStringLiteral("<div style=\"color:#333;font-size:10pt;margin-top:2px;\">%1</div>")
                    .arg(caption.toHtmlEscaped());
    }
    card += QStringLiteral("</div>");
    return card;
}

QString AttachmentCardProvider::renderText(const QVariantMap& payload) const
{
    const QString fileName = payload.value(QStringLiteral("fileName")).toString();
    if (fileName.isEmpty()) return QString();
    return QStringLiteral("[附件: %1]").arg(fileName);
}

QPixmap AttachmentCardProvider::renderPreview(const QVariantMap& payload, int width) const
{
    const QString fileName = payload.value(QStringLiteral("fileName")).toString();
    const qint64 fileSize = payload.value(QStringLiteral("fileSize")).toLongLong();
    const QString mime = payload.value(QStringLiteral("mimeType")).toString();

    const int height = 88;
    QPixmap pm(width, height);
    pm.fill(QColor(0xF7, 0xF9, 0xFC));

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    // 边框
    p.setPen(QColor(0xD0, 0xD5, 0xDD));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(1, 1, width - 2, height - 2, 6, 6);

    // 图标（回形针示意）
    p.setPen(QPen(QColor(0x1F, 0x4E, 0x79), 3));
    const QPointF clip(28, 46);
    p.drawArc(QRectF(clip.x() - 10, clip.y() - 14, 26, 28), 300 * 16, -120 * 16);
    p.drawArc(QRectF(clip.x() - 12, clip.y() - 24, 30, 48), 300 * 16, -120 * 16);

    QFontMetrics fm(p.font());
    // 文件名（截断）
    const int textX = 58;
    const int textW = width - textX - 14;
    QString name = fileName;
    if (fm.horizontalAdvance(name) > textW)
        name = fm.elidedText(name, Qt::ElideMiddle, textW);
    p.setPen(QColor(0x20, 0x20, 0x20));
    p.setFont([&] { QFont f = p.font(); f.setBold(true); return f; }());
    p.drawText(textX, 38, name);

    // 类型 · 大小
    p.setPen(QColor(0x66, 0x66, 0x66));
    QFont sf = p.font();
    sf.setPointSizeF(sf.pointSizeF() - 1.5);
    sf.setBold(false);
    p.setFont(sf);
    p.drawText(textX, 64, QStringLiteral("%1 · %2").arg(mimeLabel(mime), humanSize(fileSize)));
    return pm;
}
