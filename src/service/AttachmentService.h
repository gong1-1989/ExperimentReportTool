/**
 * @file AttachmentService.h
 * @brief 附件业务逻辑服务层
 */

#ifndef ATTACHMENT_SERVICE_H
#define ATTACHMENT_SERVICE_H

#include "core/models/Attachment.h"

class AttachmentService
{
public:
    static Attachment::Ptr getById(qint64 id);
    static Attachment::List findByReport(qint64 reportId);
    static Attachment::Ptr uploadFile(qint64 reportId, const QString& filePath);
    static bool downloadTo(qint64 attachmentId, const QString& savePath);
    static bool openWithDefaultApp(qint64 attachmentId);
    static bool remove(qint64 id);

private:
    AttachmentService() = delete;
    ~AttachmentService() = delete;
};

#endif // ATTACHMENT_SERVICE_H
