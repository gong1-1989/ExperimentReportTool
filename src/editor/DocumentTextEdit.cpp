/**
 * @file DocumentTextEdit.cpp
 * @brief 连续文档编辑控件实现
 */

#include "DocumentTextEdit.h"
#include "editor/ReportEditor.h"

#include <QTextImageFormat>
#include <QMouseEvent>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextFragment>
#include <QImage>

DocumentTextEdit::DocumentTextEdit(QWidget* parent)
    : QTextEdit(parent)
{
    setUndoRedoEnabled(true);

    // 文档层资源提供者：QTextDocument 渲染图片资源时必定调用它
    // （不依赖 QTextEdit 的 loadResource 桥接，跨 Qt 版本可靠）
    document()->setResourceProvider([this](const QUrl& name) -> QVariant {
        const QString imageName = name.toString();
        if (imageName.startsWith("object://")) {
            // 对象锚点 → 宿主提供的预览图
            if (m_host) {
                const QPixmap pixmap = m_host->objectPreviewPixmap(imageName);
                if (!pixmap.isNull()) return pixmap;
            }
            // 兜底：占位图（宽度与锚点默认宽度一致）
            QPixmap placeholder(560, 100);
            placeholder.fill(Qt::lightGray);
            return placeholder;
        }
        // 普通本地图片：默认加载逻辑
        if (name.isLocalFile()) {
            const QImage img(name.toLocalFile());
            if (!img.isNull()) return img;
        }
        const QImage img(imageName);
        if (!img.isNull()) return img;
        return QVariant();
    });
}

// ===========================================================================
// 对象锚点
// ===========================================================================

void DocumentTextEdit::insertObjectAnchor(const QString& objectType,
                                          const QString& objectId,
                                          int width,
                                          int height)
{
    QTextCursor cursor = textCursor();

    QTextImageFormat imageFormat;
    imageFormat.setName(QString("object://%1/%2").arg(objectType, objectId));
    imageFormat.setWidth(width);
    imageFormat.setHeight(height);
    imageFormat.setVerticalAlignment(QTextCharFormat::AlignMiddle);

    // 在光标处插入对象锚点图片，并换行（对象独占一段）
    cursor.insertText(QString(QChar::ObjectReplacementCharacter), imageFormat);
    cursor.insertText("\n");
    setTextCursor(cursor);
}

QString DocumentTextEdit::objectIdAtCursor() const
{
    const QTextCursor cursor = textCursor();
    const QTextCharFormat format = cursor.charFormat();
    const QString imageName = format.isImageFormat()
        ? format.toImageFormat().name()
        : QString();
    if (imageName.startsWith("object://")) {
        // object://type/id
        const int slash = imageName.lastIndexOf('/');
        if (slash > 0) return imageName.mid(slash + 1);
    }
    return QString();
}

// ===========================================================================
// 资源加载（对象预览图）
// ===========================================================================

QVariant DocumentTextEdit::loadResource(int type, const QUrl& name)
{
    if (type == QTextDocument::ImageResource) {
        const QString imageName = name.toString();
        if (imageName.startsWith("object://")) {
            if (m_host) {
                const QPixmap pixmap = m_host->objectPreviewPixmap(imageName);
                if (!pixmap.isNull()) return pixmap;
            }
            // 兜底：占位图（宽度与锚点默认宽度一致）
            QPixmap placeholder(560, 100);
            placeholder.fill(Qt::lightGray);
            return placeholder;
        }
    }
    return QTextEdit::loadResource(type, name);
}

// ===========================================================================
// 点击对象（双击才打开编辑窗口；单击仅定位光标）
// ===========================================================================

void DocumentTextEdit::mousePressEvent(QMouseEvent* event)
{
    // 单击不触发编辑窗口，只保留 QTextEdit 默认的光标定位行为
    QTextEdit::mousePressEvent(event);
}

void DocumentTextEdit::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // 定位双击处的字符格式
        QTextCursor cursor = cursorForPosition(event->pos());
        const QTextCharFormat format = cursor.charFormat();
        if (format.isImageFormat()) {
            const QString imageName = format.toImageFormat().name();
            if (imageName.startsWith("object://")) {
                const int slash = imageName.lastIndexOf('/');
                if (slash > 0) {
                    const QString objectId = imageName.mid(slash + 1);
                    emit objectDoubleClicked(objectId);
                    event->accept();
                    return;
                }
            }
        }
    }
    QTextEdit::mouseDoubleClickEvent(event);
}
