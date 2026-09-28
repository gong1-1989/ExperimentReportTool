/**
 * @file AttachmentManagerDialog.h
 * @brief 附件管理对话框头文件
 *
 * 管理报告的附件：上传、下载、打开、删除。
 */

#ifndef ATTACHMENT_MANAGER_DIALOG_H
#define ATTACHMENT_MANAGER_DIALOG_H

#include "BaseDialog.h"
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>

#include "core/models/Attachment.h"


// 前向声明 UI 类（由 uic 工具从 .ui 文件自动生成）
namespace Ui {
class AttachmentManagerDialog;
}
/**
 * @brief 附件管理对话框
 */
class AttachmentManagerDialog : public BaseDialog
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
    void on_m_uploadBtn_clicked();
    void on_m_downloadBtn_clicked();
    void on_m_openBtn_clicked();
    void on_m_deleteBtn_clicked();
    void on_m_closeBtn_clicked();
    void on_m_attachmentList_itemClicked(QListWidgetItem* item);
    void on_m_attachmentList_itemDoubleClicked(QListWidgetItem* item);
    void on_m_refreshBtn_clicked();

private:

    Ui::AttachmentManagerDialog* ui;  ///< UI 界面对象（从 .ui 文件自动生成）
    void loadAttachments();
    void updateAttachmentList();
    void updateButtons();
    Attachment::Ptr currentAttachment() const;

    /// 显示状态消息（对话框中使用 QMessageBox 提示）
    void showStatusMessage(const QString& message);

    // -----------------------------------------------------------------------
    // UI 控件
    // -----------------------------------------------------------------------


    // -----------------------------------------------------------------------
    // 数据
    // -----------------------------------------------------------------------

    qint64 m_reportId;                 ///< 报告 ID
    Attachment::List m_attachments;    ///< 附件列表
};

#endif // ATTACHMENT_MANAGER_DIALOG_H
