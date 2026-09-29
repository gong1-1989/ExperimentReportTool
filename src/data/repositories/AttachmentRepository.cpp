/**
 * @file AttachmentRepository.cpp
 * @brief 附件数据访问层实现文件
 *
 * 本文件实现了 AttachmentRepository 类，负责附件实体的持久化和文件操作。
 * 附件是报告的附属文件，如实验数据文件、图片、PDF 文档等。
 *
 * 主要功能：
 * - 附件的 CRUD（创建、读取、更新、删除）操作
 * - 附件文件上传（复制到存储目录）
 * - 附件文件下载（复制到指定位置）
 * - 用默认应用程序打开附件
 * - 附件数量和总大小统计
 * - 存储目录管理
 *
 * 设计说明：
 * - 采用 Repository 模式，将数据访问逻辑与业务逻辑分离
 * - 附件文件存储在本地文件系统中，数据库只存储元信息和文件路径
 * - 存储文件名使用时间戳+UUID生成，避免重名冲突
 * - 删除附件时会同时删除物理文件
 * - 上传失败时会自动清理已复制的文件
 * - 使用 QMimeDatabase 自动检测 MIME 类型
 * - 使用 QSharedPointer 管理附件对象的生命周期
 *
 * 数据库表结构：
 * - attachments：附件元信息表（id, report_id, file_name, stored_path, file_size, mime_type, uploaded_at）
 *
 * 文件存储：
 * - 存储目录：~/.ExperimentReportTool/attachments/
 * - 文件名格式：yyyyMMdd_hhmmss_xxxxxxxx.ext（时间戳+8位UUID）
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
// 附件 CRUD 操作
// ===========================================================================

/**
 * @brief 根据 ID 查找附件
 *
 * @param attachmentId 附件唯一 ID
 * @return Attachment::Ptr 找到时返回附件对象，未找到或查询失败时返回空指针
 *
 * 使用场景：
 * - 下载附件前获取附件信息
 * - 打开附件前获取存储路径
 * - 删除附件前获取文件路径
 */
Attachment::Ptr AttachmentRepository::findById(qint64 attachmentId)
{
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 使用预处理语句查询
    query.prepare("SELECT * FROM attachments WHERE id = :id;");
    query.bindValue(":id", attachmentId);

    // 执行查询并检查结果
    // 使用短路求值：exec() 失败或 next() 无结果都返回空指针
    if (!DatabaseManager::executeQuery(query, "查询附件")) {
        return Attachment::Ptr();
    }
    if (!query.next()) {
        return Attachment::Ptr();
    }

    // 从查询结果创建附件对象
    return createFromQuery(query);
}

/**
 * @brief 查询指定报告的所有附件
 *
 * @param reportId 报告 ID
 * @return Attachment::List 附件列表，按上传时间降序排列
 *
 * 使用场景：
 * - 报告编辑器中显示附件列表
 * - 报告详情页显示附件
 */
Attachment::List AttachmentRepository::findByReport(qint64 reportId)
{
    Attachment::List attachments;
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 按上传时间降序排列（最新的在前）
    query.prepare("SELECT * FROM attachments WHERE report_id = :reportId ORDER BY uploaded_at DESC;");
    query.bindValue(":reportId", reportId);

    // 执行查询并遍历结果
    if (DatabaseManager::executeQuery(query, "查询报告附件")) {
        while (query.next()) {
            attachments.append(createFromQuery(query));
        }
    }

    return attachments;
}

/**
 * @brief 保存附件（新建或更新）
 *
 * 根据附件对象的状态自动判断是新建还是更新：
 * - isNew() 返回 true（id <= 0）：执行 INSERT
 * - isNew() 返回 false（id > 0）：执行 UPDATE
 *
 * @param attachment 要保存的附件对象（智能指针）
 * @return bool 保存成功返回 true，失败返回 false
 *
 * @note 新建成功后，attachment 对象的 id 会被更新为数据库生成的自增 ID
 * @note 空指针直接返回 false
 * @note 更新时只修改文件名、文件大小、MIME 类型，不修改存储路径和上传时间
 */
bool AttachmentRepository::save(Attachment::Ptr attachment)
{
    // 空指针检查
    if (!attachment) return false;

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    if (attachment->isNew()) {
        // ================================================================
        // 新建附件
        // ================================================================
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

        // 执行插入，使用统一的错误处理方法
        if (!DatabaseManager::executeQuery(query, "创建附件")) {
            return false;
        }

        // 获取自增 ID 并回写到对象
        attachment->setId(query.lastInsertId().toLongLong());
    } else {
        // ================================================================
        // 更新附件
        // ================================================================
        query.prepare(R"(
            UPDATE attachments SET file_name = :fileName, file_size = :fileSize,
                   mime_type = :mimeType WHERE id = :id;
        )");
        query.bindValue(":fileName", attachment->fileName());
        query.bindValue(":fileSize", attachment->fileSize());
        query.bindValue(":mimeType", attachment->mimeType());
        query.bindValue(":id", attachment->id());

        // 执行更新，使用统一的错误处理方法
        if (!DatabaseManager::executeQuery(query, "更新附件")) {
            return false;
        }
    }

    return true;
}

/**
 * @brief 删除附件
 *
 * 删除附件时会同时删除物理文件和数据库记录。
 *
 * @param attachmentId 要删除的附件 ID
 * @return bool 删除成功返回 true，失败返回 false
 *
 * 执行流程：
 * 1. 先查询附件信息，获取存储路径
 * 2. 删除物理文件（如果存在）
 * 3. 删除数据库记录
 *
 * @warning 此操作不可恢复，删除后附件文件将永久丢失
 * @note 物理文件删除失败只会记录警告，不会阻止数据库记录的删除
 * @note 返回 numRowsAffected > 0，表示附件确实存在且已删除
 */
bool AttachmentRepository::remove(qint64 attachmentId)
{
    // 先查询附件信息，获取存储路径
    Attachment::Ptr attachment = findById(attachmentId);
    if (!attachment) return false;

    // 删除物理文件
    if (!attachment->storedPath().isEmpty() && QFile::exists(attachment->storedPath())) {
        if (!QFile::remove(attachment->storedPath())) {
            // 文件删除失败只记录警告，不阻止数据库记录的删除
            LOG_WARNING(QString("删除附件文件失败: %1").arg(attachment->storedPath()));
        }
    }

    // 删除数据库记录
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);
    query.prepare("DELETE FROM attachments WHERE id = :id;");
    query.bindValue(":id", attachmentId);

    // 执行删除，使用统一的错误处理方法
    if (!DatabaseManager::executeQuery(query, "删除附件")) {
        return false;
    }

    // 返回是否实际删除了记录
    return query.numRowsAffected() > 0;
}

// ===========================================================================
// 文件操作
// ===========================================================================

/**
 * @brief 上传文件作为附件
 *
 * 将源文件复制到附件存储目录，并在数据库中创建附件记录。
 *
 * @param reportId 所属报告 ID
 * @param sourceFilePath 源文件路径
 * @return Attachment::Ptr 上传成功返回附件对象，失败返回空指针
 *
 * 执行流程：
 * 1. 校验参数（reportId > 0，源文件路径非空）
 * 2. 检查源文件是否存在
 * 3. 打开并读取源文件内容
 * 4. 确保存储目录存在
 * 5. 生成唯一的存储文件名（时间戳+UUID）
 * 6. 将文件内容写入存储目录
 * 7. 检测 MIME 类型
 * 8. 创建附件记录并保存到数据库
 * 9. 如果保存失败，删除已复制的文件
 *
 * @note 存储文件名格式：yyyyMMdd_hhmmss_xxxxxxxx.ext（时间戳+8位UUID）
 * @note 上传失败时会自动清理已复制的文件，避免垃圾文件
 * @note 使用 QMimeDatabase 自动检测 MIME 类型
 */
Attachment::Ptr AttachmentRepository::uploadFile(qint64 reportId, const QString& sourceFilePath)
{
    // 参数校验
    if (reportId <= 0 || sourceFilePath.isEmpty()) {
        return Attachment::Ptr();
    }

    // 检查源文件是否存在
    QFile sourceFile(sourceFilePath);
    if (!sourceFile.exists()) {
        LOG_ERROR(QString("源文件不存在: %1").arg(sourceFilePath));
        return Attachment::Ptr();
    }

    // 打开源文件
    if (!sourceFile.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("无法打开源文件: %1").arg(sourceFilePath));
        return Attachment::Ptr();
    }

    // 读取文件内容到内存
    const QByteArray fileData = sourceFile.readAll();
    sourceFile.close();

    // 确保存储目录存在
    ensureStorageDirectory();

    // 生成唯一的存储文件名，避免重名冲突
    const QFileInfo fileInfo(sourceFilePath);
    const QString storedName = generateStoredFileName(fileInfo.fileName());
    const QString storedPath = storageDirectory() + "/" + storedName;

    // 将文件内容写入存储目录
    QFile destFile(storedPath);
    if (!destFile.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("无法创建目标文件: %1").arg(storedPath));
        return Attachment::Ptr();
    }
    destFile.write(fileData);
    destFile.close();

    // 使用 QMimeDatabase 自动检测 MIME 类型
    QMimeDatabase mimeDb;
    const QMimeType mimeType = mimeDb.mimeTypeForFile(storedPath);

    // 创建附件记录
    Attachment::Ptr attachment = Attachment::create();
    attachment->setReportId(reportId);
    attachment->setFileName(fileInfo.fileName());  // 原始文件名（用于显示）
    attachment->setStoredPath(storedPath);          // 存储路径（用于访问）
    attachment->setFileSize(fileData.size());       // 文件大小（字节）
    attachment->setMimeType(mimeType.name());       // MIME 类型
    attachment->setUploadedAt(QDateTime::currentDateTime()); // 上传时间

    // 保存到数据库
    if (!save(attachment)) {
        // 保存失败，删除已复制的文件，避免垃圾文件
        QFile::remove(storedPath);
        return Attachment::Ptr();
    }

    LOG_INFO(QString("附件已上传: %1 -> %2").arg(fileInfo.fileName(), storedPath));
    return attachment;
}

/**
 * @brief 批量上传文件作为附件
 *
 * @param reportId 所属报告 ID
 * @param sourceFilePaths 源文件路径列表
 * @return Attachment::List 成功上传的附件列表
 *
 * @note 逐个调用 uploadFile，单个文件失败不影响其他文件
 * @note 只返回成功上传的附件，失败的会被跳过
 */
Attachment::List AttachmentRepository::uploadFiles(qint64 reportId, const QStringList& sourceFilePaths)
{
    Attachment::List uploaded;

    // 逐个上传文件
    for (const QString& path : sourceFilePaths) {
        Attachment::Ptr attachment = uploadFile(reportId, path);
        if (attachment) {
            uploaded.append(attachment);
        }
    }

    return uploaded;
}

/**
 * @brief 下载附件到指定位置
 *
 * @param attachmentId 附件 ID
 * @param destPath 目标文件路径
 * @return bool 下载成功返回 true，失败返回 false
 *
 * @note 如果附件文件不存在，记录错误并返回 false
 * @note 使用 QFile::copy 复制文件，不会删除源文件
 */
bool AttachmentRepository::downloadTo(qint64 attachmentId, const QString& destPath)
{
    // 先查询附件信息
    Attachment::Ptr attachment = findById(attachmentId);
    if (!attachment) return false;

    // 检查附件文件是否存在
    if (!QFile::exists(attachment->storedPath())) {
        LOG_ERROR(QString("附件文件不存在: %1").arg(attachment->storedPath()));
        return false;
    }

    // 复制文件到目标位置
    return QFile::copy(attachment->storedPath(), destPath);
}

/**
 * @brief 用系统默认应用程序打开附件
 *
 * @param attachmentId 附件 ID
 * @return bool 打开成功返回 true，失败返回 false
 *
 * 使用场景：
 * - 用户双击附件时用默认程序打开
 * - 图片用图片查看器打开，PDF 用 PDF 阅读器打开等
 *
 * @note 使用 QDesktopServices::openUrl 调用系统默认程序
 * @note 如果附件文件不存在，记录错误并返回 false
 */
bool AttachmentRepository::openWithDefaultApp(qint64 attachmentId)
{
    // 先查询附件信息
    Attachment::Ptr attachment = findById(attachmentId);
    if (!attachment) return false;

    // 检查附件文件是否存在
    if (!QFile::exists(attachment->storedPath())) {
        LOG_ERROR(QString("附件文件不存在: %1").arg(attachment->storedPath()));
        return false;
    }

    // 复制到临时目录，使用原始文件名，这样打开时标题栏显示原始文件名
    const QString tempDir = QDir::tempPath() + "/ExperimentReportTool";
    QDir().mkpath(tempDir);
    const QString tempFilePath = tempDir + "/" + attachment->fileName();

    // 如果临时文件已存在，先删除
    if (QFile::exists(tempFilePath)) {
        QFile::remove(tempFilePath);
    }

    // 复制文件
    if (!QFile::copy(attachment->storedPath(), tempFilePath)) {
        LOG_ERROR(QString("复制附件到临时目录失败: %1 -> %2")
            .arg(attachment->storedPath()).arg(tempFilePath));
        // 复制失败时回退到直接打开存储路径
        return QDesktopServices::openUrl(QUrl::fromLocalFile(attachment->storedPath()));
    }

    // 用系统默认程序打开临时文件（显示原始文件名）
    return QDesktopServices::openUrl(QUrl::fromLocalFile(tempFilePath));
}

/**
 * @brief 获取附件存储目录路径
 *
 * @return QString 存储目录的绝对路径
 *
 * 存储目录位于用户主目录下的 .ExperimentReportTool/attachments/
 * 例如：/home/user/.ExperimentReportTool/attachments/
 *
 * @note 目录可能不存在，调用前应先调用 ensureStorageDirectory()
 */
QString AttachmentRepository::storageDirectory()
{
    // 使用应用数据目录下的 attachments 子目录
    const QString dataDir = QDir::homePath() + "/.ExperimentReportTool/attachments";
    return dataDir;
}

/**
 * @brief 确保附件存储目录存在
 *
 * 如果目录不存在则创建，包括所有必要的父目录。
 *
 * @note 使用 QDir::mkpath 递归创建目录
 * @note 此方法是幂等的，目录已存在时不做任何操作
 */
void AttachmentRepository::ensureStorageDirectory()
{
    QDir().mkpath(storageDirectory());
}

// ===========================================================================
// 统计操作
// ===========================================================================

/**
 * @brief 统计指定报告的附件数量
 *
 * @param reportId 报告 ID
 * @return int 附件数量，查询失败时返回 0
 *
 * 使用场景：
 * - 报告列表中显示附件数量徽标
 * - 报告详情页统计
 */
int AttachmentRepository::countByReport(qint64 reportId)
{
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 按报告 ID 统计
    query.prepare("SELECT COUNT(*) FROM attachments WHERE report_id = :reportId;");
    query.bindValue(":reportId", reportId);

    // 执行查询并返回计数值
    if (DatabaseManager::executeQuery(query, "统计附件数量") && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

/**
 * @brief 统计指定报告的附件总大小
 *
 * @param reportId 报告 ID
 * @return qint64 附件总大小（字节），查询失败时返回 0
 *
 * 使用场景：
 * - 报告详情页显示附件总大小
 * - 存储空间统计
 *
 * @note 使用 COALESCE(SUM(file_size), 0) 确保没有附件时返回 0 而不是 NULL
 */
qint64 AttachmentRepository::totalSizeByReport(qint64 reportId)
{
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 使用 COALESCE 确保没有附件时返回 0 而不是 NULL
    query.prepare("SELECT COALESCE(SUM(file_size), 0) FROM attachments WHERE report_id = :reportId;");
    query.bindValue(":reportId", reportId);

    // 执行查询并返回总大小
    if (DatabaseManager::executeQuery(query, "统计附件总大小") && query.next()) {
        return query.value(0).toLongLong();
    }
    return 0;
}

// ===========================================================================
// 内部辅助方法
// ===========================================================================

/**
 * @brief 从数据库查询结果创建附件对象
 *
 * @param query 已执行的数据库查询对象，当前行包含附件数据
 * @return Attachment::Ptr 填充好的附件智能指针
 *
 * @note 调用前需确保 query.next() 已返回 true
 */
Attachment::Ptr AttachmentRepository::createFromQuery(const QSqlQuery& query)
{
    // 创建空的附件对象
    Attachment::Ptr attachment(new Attachment());

    // 映射各字段
    attachment->setId(query.value("id").toLongLong());              // 附件唯一 ID
    attachment->setReportId(query.value("report_id").toLongLong());  // 所属报告 ID
    attachment->setFileName(query.value("file_name").toString());    // 原始文件名（用于显示）
    attachment->setStoredPath(query.value("stored_path").toString()); // 存储路径（用于访问）
    attachment->setFileSize(query.value("file_size").toLongLong());   // 文件大小（字节）
    attachment->setMimeType(query.value("mime_type").toString());     // MIME 类型
    attachment->setUploadedAt(query.value("uploaded_at").toDateTime()); // 上传时间

    return attachment;
}

/**
 * @brief 生成唯一的存储文件名
 *
 * 使用时间戳+UUID生成唯一文件名，避免重名冲突。
 *
 * @param originalFileName 原始文件名（用于提取扩展名）
 * @return QString 生成的存储文件名
 *
 * 文件名格式：yyyyMMdd_hhmmss_xxxxxxxx.ext
 * 例如：20240115_143025_a1b2c3d4.pdf
 *
 * @note 只取 UUID 的前 8 位，兼顾唯一性和文件名长度
 * @note 保留原始文件的扩展名
 */
QString AttachmentRepository::generateStoredFileName(const QString& originalFileName)
{
    const QFileInfo fileInfo(originalFileName);
    // 生成 8 位 UUID（不带花括号）
    const QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    // 生成时间戳
    const QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    // 组合文件名：时间戳_UUID.扩展名
    return QString("%1_%2.%3").arg(timestamp, uuid, fileInfo.suffix());
}
