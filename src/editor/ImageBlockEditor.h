/**
 * @file ImageBlockEditor.h
 * @brief 图片块编辑器头文件
 *
 * 用于在报告中插入图片，支持从文件选择、图片说明文字。
 */

#ifndef IMAGE_BLOCK_EDITOR_H
#define IMAGE_BLOCK_EDITOR_H

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include "editor/BlockEditor.h"

/**
 * @brief 图片块编辑器
 */
class ImageBlockEditor : public BlockEditor
{
    Q_OBJECT

public:
    explicit ImageBlockEditor(const ContentBlock& block, QWidget* parent = nullptr);

    QJsonObject blockData() const override;
    void setBlockData(const QJsonObject& data) override;
    BlockType blockType() const override { return BlockType::Image; }
    QString plainText() const override { return m_captionEdit->text(); }
    bool isEmpty() const override { return m_imagePath.isEmpty(); }

private slots:
    void onSelectImage();
    void onCaptionChanged();

private:
    void updateImageDisplay();

    QLabel* m_imageLabel;
    QLineEdit* m_captionEdit;
    QPushButton* m_selectBtn;
    QString m_imagePath;
    int m_displayWidth;
};

#endif // IMAGE_BLOCK_EDITOR_H
