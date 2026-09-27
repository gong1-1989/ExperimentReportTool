/**
 * @file ImageBlockEditor.cpp
 * @brief 图片块编辑器实现文件
 */

#include "ImageBlockEditor.h"
#include "core/utils/AppTheme.h"
#include "core/utils/AppDimensions.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QPixmap>
#include <QVBoxLayout>
#include <QHBoxLayout>

// ===========================================================================
// 构造函数
// ===========================================================================

ImageBlockEditor::ImageBlockEditor(const ContentBlock& block, QWidget* parent)
    : BlockEditor(block, parent)
    , m_imageLabel(nullptr)
    , m_captionEdit(nullptr)
    , m_selectBtn(nullptr)
    , m_displayWidth(600)
{
    setupEditor();

    QVBoxLayout* layout = contentContainer();

    // 图片显示区域
    m_imageLabel = new QLabel(this);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setFixedHeight(AppDimensions::Widget::ImageMinHeight);
    m_imageLabel->setStyleSheet(
        QString("QLabel { border: 2px dashed %1; border-radius: %2px; "
                "background-color: %3; color: %4; }")
            .arg(AppTheme::Color::Border)
            .arg(AppTheme::Radius::Small)
            .arg(AppTheme::Color::BgGray)
            .arg(AppTheme::Color::TextSecondary));
    m_imageLabel->setText(tr("点击下方按钮选择图片"));
    layout->addWidget(m_imageLabel);

    // 工具栏
    QHBoxLayout* toolbar = new QHBoxLayout();
    m_selectBtn = new QPushButton(tr("选择图片"), this);
    m_captionEdit = new QLineEdit(this);
    m_captionEdit->setPlaceholderText(tr("图片说明（可选）"));
    toolbar->addWidget(m_selectBtn);
    toolbar->addWidget(m_captionEdit, 1);
    layout->addLayout(toolbar);

    connect(m_selectBtn, &QPushButton::clicked, this, &ImageBlockEditor::onSelectImage);
    connect(m_captionEdit, &QLineEdit::textChanged, this, &ImageBlockEditor::onCaptionChanged);

    if (!block.data.isEmpty()) {
        setBlockData(block.data);
    }

    updateHeight();
}

// ===========================================================================
// 数据存取
// ===========================================================================

QJsonObject ImageBlockEditor::blockData() const
{
    QJsonObject data;
    data["path"] = m_imagePath;
    data["caption"] = m_captionEdit->text();
    data["width"] = m_displayWidth;
    return data;
}

void ImageBlockEditor::setBlockData(const QJsonObject& data)
{
    m_imagePath = data.value("path").toString();
    m_displayWidth = data.value("width").toInt(600);
    m_captionEdit->setText(data.value("caption").toString());
    updateImageDisplay();
}

// ===========================================================================
// 槽函数
// ===========================================================================

void ImageBlockEditor::onSelectImage()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, tr("选择图片"), QString(),
        tr("图片文件 (*.png *.jpg *.jpeg *.gif *.bmp *.svg);;所有文件 (*)"));

    if (!filePath.isEmpty()) {
        m_imagePath = filePath;
        updateImageDisplay();
        notifyContentChanged();
    }
}

void ImageBlockEditor::onCaptionChanged()
{
    notifyContentChanged();
}

// ===========================================================================
// 内部方法
// ===========================================================================

void ImageBlockEditor::updateImageDisplay()
{
    if (m_imagePath.isEmpty()) {
        m_imageLabel->setText(tr("点击下方按钮选择图片"));
        m_imageLabel->setPixmap(QPixmap());
        return;
    }

    QPixmap pixmap(m_imagePath);
    if (pixmap.isNull()) {
        m_imageLabel->setText(tr("图片加载失败: %1").arg(m_imagePath));
        return;
    }

    const int maxWidth = qMin(m_displayWidth, width() - 40);
    if (pixmap.width() > maxWidth) {
        pixmap = pixmap.scaledToWidth(maxWidth, Qt::SmoothTransformation);
    }
    m_imageLabel->setPixmap(pixmap);
    m_imageLabel->setStyleSheet("QLabel { border: none; background: transparent; }");

    updateHeight();
}
