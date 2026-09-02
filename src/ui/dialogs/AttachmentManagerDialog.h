/**
 * @file AttachmentManagerDialog.h
 * @brief 附件管理对话框头文件
 *
 * 管理报告的附件：上传、下载、打开、删除。
 */

#ifndef ATTACHMENT_MANAGER_DIALOG_H
#define ATTACHMENT_MANAGER_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>

#include "core/models/Attachment.h"

/**
 * @brief 附件管理对话框
 */
class AttachmentManagerDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param reportId 报告 ID
     * @param parent 父窗口
     */
    explicit AttachmentManagerDialog(qint64 reportId, QWidget* parent = nullptr);
    ~AttachmentManagerDialog() override;

private slots:
    void onUpload();
    void onDownload();
    void onOpen();
    void onDelete();
    void onItemSelected(QListWidgetItem* item);
    void onItemDoubleClicked(QListWidgetItem* item);
    void onRefresh();

private:
    void setupUi();
    void loadAttachments();
    void updateAttachmentList();
    void updateButtons();
    Attachment::Ptr currentAttachment() const;
    void showStatusMessage(const QString &msg, int timeout = 3000);

    // -----------------------------------------------------------------------
    // UI 控件
    // -----------------------------------------------------------------------

    QListWidget* m_attachmentList;    ///< 附件列表
    QPushButton* m_uploadBtn;          ///< 上传按钮
    QPushButton* m_downloadBtn;        ///< 下载按钮
    QPushButton* m_openBtn;            ///< 打开按钮
    QPushButton* m_deleteBtn;          ///< 删除按钮
    QPushButton* m_refreshBtn;         ///< 刷新按钮
    QPushButton* m_closeBtn;           ///< 关闭按钮
    QLabel* m_infoLabel;               ///< 信息标签
    QProgressBar* m_progressBar;       ///< 进度条

    // -----------------------------------------------------------------------
    // 数据
    // -----------------------------------------------------------------------

    qint64 m_reportId;                 ///< 报告 ID
    Attachment::List m_attachments;    ///< 附件列表
};

#endif // ATTACHMENT_MANAGER_DIALOG_H
