/**
 * @file Attachment.cpp
 * @brief 附件实体实现文件
 */

#include "Attachment.h"
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>

// ===========================================================================
// 构造与析构
// ===========================================================================

Attachment::Attachment()
    : m_id(-1)
    , m_reportId(-1)
    , m_fileName("")
    , m_storedPath("")
    , m_fileSize(0)
    , m_mimeType("application/octet-stream")
{
    m_uploadedAt = QDateTime::currentDateTime();
}

Attachment::~Attachment()
{
}

// ===========================================================================
// 工厂方法
// ===========================================================================

Attachment::Ptr Attachment::create()
{
    return Ptr(new Attachment());
}

// ===========================================================================
// 工具方法
// ===========================================================================

QString Attachment::extension() const
{
    return QFileInfo(m_fileName).suffix().toLower();
}

QString Attachment::formattedSize() const
{
    const qint64 size = m_fileSize;
    if (size < 1024) {
        return QString("%1 B").arg(size);
    } else if (size < 1024 * 1024) {
        return QString("%1 KB").arg(size / 1024.0, 0, 'f', 1);
    } else if (size < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(size / (1024.0 * 1024), 0, 'f', 1);
    } else {
        return QString("%1 GB").arg(size / (1024.0 * 1024 * 1024), 0, 'f', 2);
    }
}

bool Attachment::isImage() const
{
    static const QStringList imageExts = {"jpg", "jpeg", "png", "gif", "bmp", "svg", "webp", "tiff", "ico"};
    return imageExts.contains(extension());
}

bool Attachment::isPdf() const
{
    return extension() == "pdf" || m_mimeType == "application/pdf";
}

bool Attachment::isDocument() const
{
    static const QStringList docExts = {
        "doc", "docx", "xls", "xlsx", "ppt", "pptx",
        "txt", "md", "rtf", "odt", "ods", "odp", "csv"
    };
    return docExts.contains(extension()) || isPdf();
}

QString Attachment::typeIcon() const
{
    if (isImage()) return "🖼️";
    if (isPdf()) return "📕";
    if (extension() == "doc" || extension() == "docx") return "📘";
    if (extension() == "xls" || extension() == "xlsx" || extension() == "csv") return "📗";
    if (extension() == "ppt" || extension() == "pptx") return "📙";
    if (extension() == "txt" || extension() == "md") return "📄";
    if (extension() == "zip" || extension() == "rar" || extension() == "7z") return "📦";
    if (extension() == "mp3" || extension() == "wav" || extension() == "flac") return "🎵";
    if (extension() == "mp4" || extension() == "avi" || extension() == "mkv") return "🎬";
    return "📎";
}
