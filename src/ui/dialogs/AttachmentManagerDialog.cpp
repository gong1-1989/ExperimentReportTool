/**
 * @file AttachmentManagerDialog.cpp
 * @brief 附件管理对话框实现文件
 */

#include "AttachmentManagerDialog.h"
#include "ui_AttachmentManagerDialog.h"  // 由 uic 工具从 .ui 文件自动生成
#include "data/repositories/AttachmentRepository.h"
#include "core/utils/Logger.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QDateTime>
#include <QMenu>

// ===========================================================================
// 构造与析构
// ===========================================================================

AttachmentManagerDialog::AttachmentManagerDialog(qint64 reportId, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::AttachmentManagerDialog)
    , m_reportId(reportId)
{
    ui->setupUi(this);
    loadAttachments();
    setWindowTitle(tr("附件管理"));
    resize(700, 500);
}

AttachmentManagerDialog::~AttachmentManagerDialog()
{
    delete ui;
}

// ===========================================================================
// 加载附件
// ===========================================================================

void AttachmentManagerDialog::loadAttachments()
{
    m_attachments = AttachmentRepository::findByReport(m_reportId);
    updateAttachmentList();
    updateButtons();
}

void AttachmentManagerDialog::updateAttachmentList()
{
    ui->m_attachmentList->clear();

    if (m_attachments.isEmpty()) {
        QListWidgetItem* item = new QListWidgetItem(
            tr("暂无附件\n点击「上传附件」添加文件"), ui->m_attachmentList);
        item->setTextAlignment(Qt::AlignCenter);
        item->setForeground(QColor("#999"));
        item->setFlags(Qt::NoItemFlags);
        item->setSizeHint(QSize(0, 80));
        ui->m_infoLabel->setText(tr("暂无附件"));
        return;
    }

    qint64 totalSize = 0;
    for (const Attachment::Ptr& att : m_attachments) {
        totalSize += att->fileSize();

        QListWidgetItem* item = new QListWidgetItem(ui->m_attachmentList);

        const QString displayText = QString(
            "<div style='display: flex; align-items: center;'>"
            "<span style='font-size: 24px; margin-right: 12px;'>%1</span>"
            "<div style='flex: 1;'>"
            "<div style='font-weight: bold; font-size: 14px; color: #1a1a1a;'>%2</div>"
            "<div style='font-size: 12px; color: #888; margin-top: 2px;'>"
            "%3 | %4 | 上传于 %5"
            "</div>"
            "</div>"
            "</div>"
        ).arg(att->typeIcon())
         .arg(att->fileName().toHtmlEscaped())
         .arg(att->formattedSize())
         .arg(att->mimeType())
         .arg(att->uploadedAt().toString("yyyy-MM-dd hh:mm"));

        item->setText(displayText);
        item->setData(Qt::UserRole, att->id());
        item->setSizeHint(QSize(0, 60));
    }

    // 格式化总大小
    QString totalSizeStr;
    if (totalSize < 1024 * 1024) {
        totalSizeStr = QString("%1 KB").arg(totalSize / 1024);
    } else {
        totalSizeStr = QString("%1 MB").arg(totalSize / (1024.0 * 1024), 0, 'f', 1);
    }

    ui->m_infoLabel->setText(tr("共 %1 个附件，总计 %2")
        .arg(m_attachments.size()).arg(totalSizeStr));
}

void AttachmentManagerDialog::updateButtons()
{
    const bool hasSelection = ui->m_attachmentList->currentItem() != nullptr
        && ui->m_attachmentList->currentItem()->flags() & Qt::ItemIsSelectable;

    ui->m_downloadBtn->setEnabled(hasSelection);
    ui->m_openBtn->setEnabled(hasSelection);
    ui->m_deleteBtn->setEnabled(hasSelection);
}

Attachment::Ptr AttachmentManagerDialog::currentAttachment() const
{
    QListWidgetItem* item = ui->m_attachmentList->currentItem();
    if (!item || !item->data(Qt::UserRole).isValid()) {
        return Attachment::Ptr();
    }

    const qint64 id = item->data(Qt::UserRole).toLongLong();
    for (const Attachment::Ptr& att : m_attachments) {
        if (att->id() == id) return att;
    }
    return Attachment::Ptr();
}

void AttachmentManagerDialog::showStatusMessage(const QString& message)
{
    // 对话框没有状态栏，使用 QMessageBox 显示操作结果提示
    QMessageBox::information(this, tr("提示"), message);
}

// ===========================================================================
// 上传
// ===========================================================================

void AttachmentManagerDialog::onUpload()
{
    const QStringList filePaths = QFileDialog::getOpenFileNames(
        this, tr("选择要上传的文件"), QString(),
        tr("所有文件 (*);;图片 (*.jpg *.jpeg *.png *.gif *.bmp *.svg);;"
           "文档 (*.pdf *.doc *.docx *.xls *.xlsx *.ppt *.pptx *.txt *.md);;"
           "压缩包 (*.zip *.rar *.7z)"));

    if (filePaths.isEmpty()) return;

    ui->m_progressBar->setVisible(true);
    ui->m_progressBar->setRange(0, filePaths.size());
    ui->m_progressBar->setValue(0);

    QApplication::setOverrideCursor(Qt::WaitCursor);

    int successCount = 0;
    for (int i = 0; i < filePaths.size(); ++i) {
        Attachment::Ptr att = AttachmentRepository::uploadFile(m_reportId, filePaths.at(i));
        if (att) {
            ++successCount;
        }
        ui->m_progressBar->setValue(i + 1);
        QApplication::processEvents();
    }

    QApplication::restoreOverrideCursor();
    ui->m_progressBar->setVisible(false);

    loadAttachments();

    if (successCount > 0) {
        QMessageBox::information(this, tr("上传完成"),
            tr("成功上传 %1 个文件").arg(successCount));
    } else {
        QMessageBox::warning(this, tr("上传失败"), tr("没有文件成功上传"));
    }
}

// ===========================================================================
// 下载
// ===========================================================================

void AttachmentManagerDialog::onDownload()
{
    Attachment::Ptr att = currentAttachment();
    if (!att) return;

    const QString savePath = QFileDialog::getSaveFileName(
        this, tr("保存附件"), att->fileName(),
        tr("所有文件 (*)"));

    if (savePath.isEmpty()) return;

    if (AttachmentRepository::downloadTo(att->id(), savePath)) {
        QMessageBox::information(this, tr("下载成功"),
            tr("附件已保存到:\n%1").arg(savePath));
    } else {
        QMessageBox::critical(this, tr("下载失败"), tr("保存附件时发生错误"));
    }
}

// ===========================================================================
// 打开
// ===========================================================================

void AttachmentManagerDialog::onOpen()
{
    Attachment::Ptr att = currentAttachment();
    if (!att) return;

    if (!AttachmentRepository::openWithDefaultApp(att->id())) {
        QMessageBox::warning(this, tr("打开失败"),
            tr("无法打开文件，请尝试先下载再打开。"));
    }
}

// ===========================================================================
// 删除
// ===========================================================================

void AttachmentManagerDialog::onDelete()
{
    Attachment::Ptr att = currentAttachment();
    if (!att) return;

    const auto result = QMessageBox::question(
        this, tr("确认删除"),
        tr("确定要删除附件「%1」吗？\n此操作不可撤销。").arg(att->fileName()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (result != QMessageBox::Yes) return;

    if (AttachmentRepository::remove(att->id())) {
        loadAttachments();
        showStatusMessage(tr("附件已删除"));
    } else {
        QMessageBox::critical(this, tr("删除失败"), tr("删除附件时发生错误"));
    }
}

// ===========================================================================
// 选择/双击
// ===========================================================================

void AttachmentManagerDialog::onItemSelected(QListWidgetItem* item)
{
    Q_UNUSED(item);
    updateButtons();
}

void AttachmentManagerDialog::onItemDoubleClicked(QListWidgetItem* item)
{
    Q_UNUSED(item);
    onOpen();
}

// ===========================================================================
// 刷新
// ===========================================================================

void AttachmentManagerDialog::onRefresh()
{
    loadAttachments();
}
