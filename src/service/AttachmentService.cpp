/**
 * @file AttachmentService.cpp
 * @brief 附件业务逻辑服务层实现
 */

#include "AttachmentService.h"
#include "data/repositories/AttachmentRepository.h"
#include "core/utils/Logger.h"

Attachment::Ptr AttachmentService::getById(qint64 id)
{
    return AttachmentRepository::findById(id);
}

Attachment::List AttachmentService::findByReport(qint64 reportId)
{
    return AttachmentRepository::findByReport(reportId);
}

Attachment::Ptr AttachmentService::uploadFile(qint64 reportId, const QString& filePath)
{
    Attachment::Ptr att = AttachmentRepository::uploadFile(reportId, filePath);
    if (!att) {
        LOG_ERROR(QString("上传附件失败: reportId=%1, file=%2").arg(reportId).arg(filePath));
    }
    return att;
}

bool AttachmentService::downloadTo(qint64 attachmentId, const QString& savePath)
{
    const bool ok = AttachmentRepository::downloadTo(attachmentId, savePath);
    if (!ok) {
        LOG_ERROR(QString("下载附件失败: id=%1, savePath=%2").arg(attachmentId).arg(savePath));
    }
    return ok;
}

bool AttachmentService::openWithDefaultApp(qint64 attachmentId)
{
    const bool ok = AttachmentRepository::openWithDefaultApp(attachmentId);
    if (!ok) {
        LOG_ERROR(QString("打开附件失败: id=%1").arg(attachmentId));
    }
    return ok;
}

bool AttachmentService::remove(qint64 id)
{
    const bool ok = AttachmentRepository::remove(id);
    if (!ok) {
        LOG_ERROR(QString("删除附件失败: id=%1").arg(id));
    }
    return ok;
}
