/**
 * @file AttachmentManagerDialog.cpp
 * @brief 附件管理对话框实现文件
 */

#include "AttachmentManagerDialog.h"
#include "data/repositories/AttachmentRepository.h"
#include "core/utils/Logger.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QDateTime>
#include <QMenu>
#include <QTimer>

// ===========================================================================
// 构造与析构
// ===========================================================================

AttachmentManagerDialog::AttachmentManagerDialog(qint64 reportId, QWidget* parent)
    : QDialog(parent)
    , m_attachmentList(nullptr)
    , m_uploadBtn(nullptr)
    , m_downloadBtn(nullptr)
    , m_openBtn(nullptr)
    , m_deleteBtn(nullptr)
    , m_refreshBtn(nullptr)
    , m_closeBtn(nullptr)
    , m_infoLabel(nullptr)
    , m_progressBar(nullptr)
    , m_reportId(reportId)
{
    setupUi();
    loadAttachments();
    setWindowTitle(tr("附件管理"));
    resize(700, 500);
}

AttachmentManagerDialog::~AttachmentManagerDialog()
{
}

// ===========================================================================
// UI 初始化
// ===========================================================================

void AttachmentManagerDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // -----------------------------------------------------------------------
    // 工具栏
    // -----------------------------------------------------------------------
    QHBoxLayout* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    m_uploadBtn = new QPushButton(tr("📤 上传附件"), this);
    m_uploadBtn->setStyleSheet(
        "QPushButton { background: #4A90D9; color: white; padding: 6px 16px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #357ABD; }");
    toolbar->addWidget(m_uploadBtn);

    m_downloadBtn = new QPushButton(tr("💾 下载"), this);
    m_downloadBtn->setEnabled(false);
    toolbar->addWidget(m_downloadBtn);

    m_openBtn = new QPushButton(tr("📂 打开"), this);
    m_openBtn->setEnabled(false);
    toolbar->addWidget(m_openBtn);

    m_deleteBtn = new QPushButton(tr("🗑️ 删除"), this);
    m_deleteBtn->setEnabled(false);
    m_deleteBtn->setStyleSheet("QPushButton { color: #ff4d4f; }");
    toolbar->addWidget(m_deleteBtn);

    toolbar->addStretch();

    m_refreshBtn = new QPushButton(tr("🔄 刷新"), this);
    toolbar->addWidget(m_refreshBtn);

    mainLayout->addLayout(toolbar);

    // -----------------------------------------------------------------------
    // 附件列表
    // -----------------------------------------------------------------------
    m_attachmentList = new QListWidget(this);
    m_attachmentList->setStyleSheet(
        "QListWidget { border: 1px solid #ddd; border-radius: 4px; }"
        "QListWidget::item { padding: 10px; border-bottom: 1px solid #eee; }"
        "QListWidget::item:selected { background: #e8f0fe; }"
        "QListWidget::item:hover { background: #f5f5f5; }");
    m_attachmentList->setContextMenuPolicy(Qt::CustomContextMenu);
    mainLayout->addWidget(m_attachmentList, 1);

    // -----------------------------------------------------------------------
    // 信息栏
    // -----------------------------------------------------------------------
    QHBoxLayout* infoLayout = new QHBoxLayout();
    m_infoLabel = new QLabel(tr("暂无附件"), this);
    m_infoLabel->setStyleSheet("color: #666; font-size: 12px;");
    infoLayout->addWidget(m_infoLabel);
    infoLayout->addStretch();

    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    m_progressBar->setMaximumWidth(200);
    infoLayout->addWidget(m_progressBar);

    mainLayout->addLayout(infoLayout);

    // -----------------------------------------------------------------------
    // 底部按钮
    // -----------------------------------------------------------------------
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_closeBtn = new QPushButton(tr("关闭"), this);
    buttonLayout->addWidget(m_closeBtn);
    mainLayout->addLayout(buttonLayout);

    // -----------------------------------------------------------------------
    // 连接信号
    // -----------------------------------------------------------------------
    connect(m_uploadBtn, &QPushButton::clicked, this, &AttachmentManagerDialog::onUpload);
    connect(m_downloadBtn, &QPushButton::clicked, this, &AttachmentManagerDialog::onDownload);
    connect(m_openBtn, &QPushButton::clicked, this, &AttachmentManagerDialog::onOpen);
    connect(m_deleteBtn, &QPushButton::clicked, this, &AttachmentManagerDialog::onDelete);
    connect(m_refreshBtn, &QPushButton::clicked, this, &AttachmentManagerDialog::onRefresh);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_attachmentList, &QListWidget::itemClicked,
            this, &AttachmentManagerDialog::onItemSelected);
    connect(m_attachmentList, &QListWidget::itemDoubleClicked,
            this, &AttachmentManagerDialog::onItemDoubleClicked);

    // 右键菜单
    connect(m_attachmentList, &QListWidget::customContextMenuRequested,
            this, [this](const QPoint& pos) {
                QListWidgetItem* item = m_attachmentList->itemAt(pos);
                if (!item) return;
                m_attachmentList->setCurrentItem(item);
                onItemSelected(item);

                QMenu menu(this);
                menu.addAction(tr("打开"), this, &AttachmentManagerDialog::onOpen);
                menu.addAction(tr("下载"), this, &AttachmentManagerDialog::onDownload);
                menu.addSeparator();
                menu.addAction(tr("删除"), this, &AttachmentManagerDialog::onDelete);
                menu.exec(m_attachmentList->mapToGlobal(pos));
            });
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
    m_attachmentList->clear();

    if (m_attachments.isEmpty()) {
        QListWidgetItem* item = new QListWidgetItem(
            tr("暂无附件\n点击「上传附件」添加文件"), m_attachmentList);
        item->setTextAlignment(Qt::AlignCenter);
        item->setForeground(QColor("#999"));
        item->setFlags(Qt::NoItemFlags);
        item->setSizeHint(QSize(0, 80));
        m_infoLabel->setText(tr("暂无附件"));
        return;
    }

    qint64 totalSize = 0;
    for (const Attachment::Ptr& att : m_attachments) {
        totalSize += att->fileSize();

        QListWidgetItem* item = new QListWidgetItem(m_attachmentList);

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

    m_infoLabel->setText(tr("共 %1 个附件，总计 %2")
        .arg(m_attachments.size()).arg(totalSizeStr));
}

void AttachmentManagerDialog::updateButtons()
{
    const bool hasSelection = m_attachmentList->currentItem() != nullptr
        && m_attachmentList->currentItem()->flags() & Qt::ItemIsSelectable;

    m_downloadBtn->setEnabled(hasSelection);
    m_openBtn->setEnabled(hasSelection);
    m_deleteBtn->setEnabled(hasSelection);
}

Attachment::Ptr AttachmentManagerDialog::currentAttachment() const
{
    QListWidgetItem* item = m_attachmentList->currentItem();
    if (!item || !item->data(Qt::UserRole).isValid()) {
        return Attachment::Ptr();
    }

    const qint64 id = item->data(Qt::UserRole).toLongLong();
    for (const Attachment::Ptr& att : m_attachments) {
        if (att->id() == id) return att;
    }
    return Attachment::Ptr();
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

    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, filePaths.size());
    m_progressBar->setValue(0);

    QApplication::setOverrideCursor(Qt::WaitCursor);

    int successCount = 0;
    for (int i = 0; i < filePaths.size(); ++i) {
        Attachment::Ptr att = AttachmentRepository::uploadFile(m_reportId, filePaths.at(i));
        if (att) {
            ++successCount;
        }
        m_progressBar->setValue(i + 1);
        QApplication::processEvents();
    }

    QApplication::restoreOverrideCursor();
    m_progressBar->setVisible(false);

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

void AttachmentManagerDialog::showStatusMessage(const QString &msg, int timeout)
{
    m_infoLabel->setText(msg);
    QTimer::singleShot(timeout,this,[this](){m_infoLabel->clear();});
}

