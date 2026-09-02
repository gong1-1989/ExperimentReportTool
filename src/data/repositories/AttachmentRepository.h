/**
 * @file AttachmentRepository.h
 * @brief 附件数据访问层头文件
 *
 * 提供附件的增删改查，以及文件的上传（复制到应用数据目录）和删除。
 */

#ifndef ATTACHMENT_REPOSITORY_H
#define ATTACHMENT_REPOSITORY_H

#include <QList>
#include <QString>
#include "core/models/Attachment.h"

/**
 * @brief 附件仓储类
 */
class AttachmentRepository
{
public:
    // -----------------------------------------------------------------------
    // 附件 CRUD
    // -----------------------------------------------------------------------

    /// 根据 ID 查找附件
    static Attachment::Ptr findById(qint64 attachmentId);

    /// 获取报告的所有附件
    static Attachment::List findByReport(qint64 reportId);

    /// 保存附件元信息（新建或更新）
    static bool save(Attachment::Ptr attachment);

    /// 删除附件（同时删除存储的文件）
    static bool remove(qint64 attachmentId);

    // -----------------------------------------------------------------------
    // 文件操作
    // -----------------------------------------------------------------------

    /**
     * @brief 上传文件（复制到应用数据目录并创建附件记录）
     * @param reportId 报告 ID
     * @param sourceFilePath 源文件路径
     * @return 新创建的附件，失败返回空
     */
    static Attachment::Ptr uploadFile(qint64 reportId, const QString& sourceFilePath);

    /**
     * @brief 批量上传文件
     * @param reportId 报告 ID
     * @param sourceFilePaths 源文件路径列表
     * @return 成功上传的附件列表
     */
    static Attachment::List uploadFiles(qint64 reportId, const QStringList& sourceFilePaths);

    /**
     * @brief 下载附件到指定路径
     * @param attachmentId 附件 ID
     * @param destPath 目标路径
     * @return 成功返回 true
     */
    static bool downloadTo(qint64 attachmentId, const QString& destPath);

    /**
     * @brief 用系统默认程序打开附件
     * @param attachmentId 附件 ID
     * @return 成功返回 true
     */
    static bool openWithDefaultApp(qint64 attachmentId);

    /**
     * @brief 获取附件存储目录
     * @return 目录路径
     */
    static QString storageDirectory();

    /**
     * @brief 确保存储目录存在
     */
    static void ensureStorageDirectory();

    // -----------------------------------------------------------------------
    // 统计
    // -----------------------------------------------------------------------

    /// 获取报告附件数量
    static int countByReport(qint64 reportId);

    /// 获取报告附件总大小
    static qint64 totalSizeByReport(qint64 reportId);

private:
    /// 从 SQL 查询结果创建 Attachment 对象
    static Attachment::Ptr createFromQuery(const class QSqlQuery& query);

    /// 生成唯一的存储文件名
    static QString generateStoredFileName(const QString& originalFileName);
};

#endif // ATTACHMENT_REPOSITORY_H
