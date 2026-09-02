/**
 * @file Attachment.h
 * @brief 附件实体头文件
 *
 * 报告附件，支持任意类型文件的上传、存储和管理。
 * 附件文件存储在应用数据目录下，数据库记录元信息。
 */

#ifndef ATTACHMENT_H
#define ATTACHMENT_H

#include <QString>
#include <QDateTime>
#include <QSharedPointer>
#include <QList>
#include <QSize>

/**
 * @brief 附件实体类
 */
class Attachment
{
public:
    using Ptr = QSharedPointer<Attachment>;
    using List = QList<Ptr>;

    Attachment();
    ~Attachment();

    // -----------------------------------------------------------------------
    // 属性访问
    // -----------------------------------------------------------------------

    qint64 id() const { return m_id; }
    void setId(qint64 id) { m_id = id; }

    qint64 reportId() const { return m_reportId; }
    void setReportId(qint64 id) { m_reportId = id; }

    QString fileName() const { return m_fileName; }
    void setFileName(const QString& name) { m_fileName = name; }

    QString storedPath() const { return m_storedPath; }
    void setStoredPath(const QString& path) { m_storedPath = path; }

    qint64 fileSize() const { return m_fileSize; }
    void setFileSize(qint64 size) { m_fileSize = size; }

    QString mimeType() const { return m_mimeType; }
    void setMimeType(const QString& type) { m_mimeType = type; }

    QDateTime uploadedAt() const { return m_uploadedAt; }
    void setUploadedAt(const QDateTime& dt) { m_uploadedAt = dt; }

    // -----------------------------------------------------------------------
    // 工具方法
    // -----------------------------------------------------------------------

    /// 是否为新附件（未保存到数据库）
    bool isNew() const { return m_id <= 0; }

    /// 获取文件扩展名
    QString extension() const;

    /// 获取格式化的文件大小（如 "1.5 MB"）
    QString formattedSize() const;

    /// 判断是否为图片类型
    bool isImage() const;

    /// 判断是否为 PDF 类型
    bool isPdf() const;

    /// 判断是否为文档类型
    bool isDocument() const;

    /// 获取文件类型图标名称
    QString typeIcon() const;

    /// 创建新附件的工厂方法
    static Ptr create();

private:
    qint64 m_id;            ///< 附件 ID
    qint64 m_reportId;       ///< 所属报告 ID
    QString m_fileName;       ///< 原始文件名
    QString m_storedPath;     ///< 存储路径（应用数据目录下）
    qint64 m_fileSize;        ///< 文件大小（字节）
    QString m_mimeType;       ///< MIME 类型
    QDateTime m_uploadedAt;   ///< 上传时间
};

#endif // ATTACHMENT_H
