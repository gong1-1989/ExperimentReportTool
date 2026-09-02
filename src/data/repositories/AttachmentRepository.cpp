/**
 * @file AttachmentRepository.cpp
 * @brief 附件数据访问层实现文件
 */

#include "AttachmentRepository.h"
#include "data/database/DatabaseManager.h"
#include "core/utils/Logger.h"
#include "core/utils/AppConstants.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QMimeDatabase>
#include <QMimeType>
#include <QUuid>

// ===========================================================================
// 附件 CRUD
// ===========================================================================

Attachment::Ptr AttachmentRepository::findById(qint64 attachmentId)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("SELECT * FROM attachments WHERE id = :id;");
    query.bindValue(":id", attachmentId);

    if (!query.exec() || !query.next()) {
        return Attachment::Ptr();
    }

    return createFromQuery(query);
}

Attachment::List AttachmentRepository::findByReport(qint64 reportId)
{
    Attachment::List attachments;
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("SELECT * FROM attachments WHERE report_id = :reportId ORDER BY uploaded_at DESC;");
    query.bindValue(":reportId", reportId);

    if (query.exec()) {
        while (query.next()) {
            attachments.append(createFromQuery(query));
        }
    }

    return attachments;
}

bool AttachmentRepository::save(Attachment::Ptr attachment)
{
    if (!attachment) return false;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    if (attachment->isNew()) {
        // 新建
        query.prepare(R"(
            INSERT INTO attachments (report_id, file_name, stored_path, file_size, mime_type, uploaded_at)
            VALUES (:reportId, :fileName, :storedPath, :fileSize, :mimeType, :uploadedAt);
        )");
        query.bindValue(":reportId", attachment->reportId());
        query.bindValue(":fileName", attachment->fileName());
        query.bindValue(":storedPath", attachment->storedPath());
        query.bindValue(":fileSize", attachment->fileSize());
        query.bindValue(":mimeType", attachment->mimeType());
        query.bindValue(":uploadedAt", attachment->uploadedAt());

        if (!query.exec()) {
            LOG_ERROR(QString("创建附件失败: %1").arg(query.lastError().text()));
            return false;
        }

        attachment->setId(query.lastInsertId().toLongLong());
    } else {
        // 更新
        query.prepare(R"(
            UPDATE attachments SET file_name = :fileName, file_size = :fileSize,
                   mime_type = :mimeType WHERE id = :id;
        )");
        query.bindValue(":fileName", attachment->fileName());
        query.bindValue(":fileSize", attachment->fileSize());
        query.bindValue(":mimeType", attachment->mimeType());
        query.bindValue(":id", attachment->id());

        if (!query.exec()) {
            LOG_ERROR(QString("更新附件失败: %1").arg(query.lastError().text()));
            return false;
        }
    }

    return true;
}

bool AttachmentRepository::remove(qint64 attachmentId)
{
    Attachment::Ptr attachment = findById(attachmentId);
    if (!attachment) return false;

    // 删除存储的文件
    if (!attachment->storedPath().isEmpty() && QFile::exists(attachment->storedPath())) {
        if (!QFile::remove(attachment->storedPath())) {
            LOG_WARNING(QString("删除附件文件失败: %1").arg(attachment->storedPath()));
        }
    }

    // 删除数据库记录
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("DELETE FROM attachments WHERE id = :id;");
    query.bindValue(":id", attachmentId);

    if (!query.exec()) {
        LOG_ERROR(QString("删除附件失败: %1").arg(query.lastError().text()));
        return false;
    }

    return query.numRowsAffected() > 0;
}

// ===========================================================================
// 文件操作
// ===========================================================================

Attachment::Ptr AttachmentRepository::uploadFile(qint64 reportId, const QString& sourceFilePath)
{
    if (reportId <= 0 || sourceFilePath.isEmpty()) {
        return Attachment::Ptr();
    }

    QFile sourceFile(sourceFilePath);
    if (!sourceFile.exists()) {
        LOG_ERROR(QString("源文件不存在: %1").arg(sourceFilePath));
        return Attachment::Ptr();
    }

    if (!sourceFile.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("无法打开源文件: %1").arg(sourceFilePath));
        return Attachment::Ptr();
    }

    const QByteArray fileData = sourceFile.readAll();
    sourceFile.close();

    // 确保存储目录存在
    ensureStorageDirectory();

    // 生成存储路径
    const QFileInfo fileInfo(sourceFilePath);
    const QString storedName = generateStoredFileName(fileInfo.fileName());
    const QString storedPath = storageDirectory() + "/" + storedName;

    // 复制文件
    QFile destFile(storedPath);
    if (!destFile.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("无法创建目标文件: %1").arg(storedPath));
        return Attachment::Ptr();
    }
    destFile.write(fileData);
    destFile.close();

    // 检测 MIME 类型
    QMimeDatabase mimeDb;
    const QMimeType mimeType = mimeDb.mimeTypeForFile(storedPath);

    // 创建附件记录
    Attachment::Ptr attachment = Attachment::create();
    attachment->setReportId(reportId);
    attachment->setFileName(fileInfo.fileName());
    attachment->setStoredPath(storedPath);
    attachment->setFileSize(fileData.size());
    attachment->setMimeType(mimeType.name());
    attachment->setUploadedAt(QDateTime::currentDateTime());

    if (!save(attachment)) {
        // 保存失败，删除已复制的文件
        QFile::remove(storedPath);
        return Attachment::Ptr();
    }

    LOG_INFO(QString("附件已上传: %1 -> %2").arg(fileInfo.fileName(), storedPath));
    return attachment;
}

Attachment::List AttachmentRepository::uploadFiles(qint64 reportId, const QStringList& sourceFilePaths)
{
    Attachment::List uploaded;
    for (const QString& path : sourceFilePaths) {
        Attachment::Ptr attachment = uploadFile(reportId, path);
        if (attachment) {
            uploaded.append(attachment);
        }
    }
    return uploaded;
}

bool AttachmentRepository::downloadTo(qint64 attachmentId, const QString& destPath)
{
    Attachment::Ptr attachment = findById(attachmentId);
    if (!attachment) return false;

    if (!QFile::exists(attachment->storedPath())) {
        LOG_ERROR(QString("附件文件不存在: %1").arg(attachment->storedPath()));
        return false;
    }

    return QFile::copy(attachment->storedPath(), destPath);
}

bool AttachmentRepository::openWithDefaultApp(qint64 attachmentId)
{
    Attachment::Ptr attachment = findById(attachmentId);
    if (!attachment) return false;

    if (!QFile::exists(attachment->storedPath())) {
        LOG_ERROR(QString("附件文件不存在: %1").arg(attachment->storedPath()));
        return false;
    }

    return QDesktopServices::openUrl(QUrl::fromLocalFile(attachment->storedPath()));
}

QString AttachmentRepository::storageDirectory()
{
    // 使用应用数据目录下的 attachments 子目录
    const QString dataDir = QDir::homePath() + "/.ExperimentReportTool/attachments";
    return dataDir;
}

void AttachmentRepository::ensureStorageDirectory()
{
    QDir().mkpath(storageDirectory());
}

// ===========================================================================
// 统计
// ===========================================================================

int AttachmentRepository::countByReport(qint64 reportId)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM attachments WHERE report_id = :reportId;");
    query.bindValue(":reportId", reportId);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

qint64 AttachmentRepository::totalSizeByReport(qint64 reportId)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("SELECT COALESCE(SUM(file_size), 0) FROM attachments WHERE report_id = :reportId;");
    query.bindValue(":reportId", reportId);
    if (query.exec() && query.next()) {
        return query.value(0).toLongLong();
    }
    return 0;
}

// ===========================================================================
// 辅助方法
// ===========================================================================

Attachment::Ptr AttachmentRepository::createFromQuery(const QSqlQuery& query)
{
    Attachment::Ptr attachment(new Attachment());
    attachment->setId(query.value("id").toLongLong());
    attachment->setReportId(query.value("report_id").toLongLong());
    attachment->setFileName(query.value("file_name").toString());
    attachment->setStoredPath(query.value("stored_path").toString());
    attachment->setFileSize(query.value("file_size").toLongLong());
    attachment->setMimeType(query.value("mime_type").toString());
    attachment->setUploadedAt(query.value("uploaded_at").toDateTime());
    return attachment;
}

QString AttachmentRepository::generateStoredFileName(const QString& originalFileName)
{
    const QFileInfo fileInfo(originalFileName);
    const QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    const QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    return QString("%1_%2.%3").arg(timestamp, uuid, fileInfo.suffix());
}
