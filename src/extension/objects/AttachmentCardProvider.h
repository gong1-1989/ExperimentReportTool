#pragma once
#include "extension/DocumentObjectProvider.h"

/**
 * @brief 附件卡片提供者（D 域首批）
 *
 * payload:
 *   - fileName   文件名
 *   - filePath   文件绝对路径（渲染时检查存在性并生成本地 URL）
 *   - fileSize   文件大小（字节）
 *   - mimeType   MIME 类型
 *   - uploadedAt 上传时间（yyyy-MM-dd HH:mm:ss，可为空）
 *   - caption    题注（可为空）
 */
class AttachmentCardProvider : public DocumentObjectProvider {
public:
    QString objectTypeId() const override { return QStringLiteral("attachment_card"); }
    QString displayName() const override { return QStringLiteral("附件卡片"); }
    QString description() const override { return QStringLiteral("以卡片形式展示报告关联附件（文件名/大小/类型）"); }
    QVariantMap defaultPayload() const override;
    QString renderHtml(const QVariantMap& payload, const QString& align) const override;
    QString renderText(const QVariantMap& payload) const override;
    QPixmap renderPreview(const QVariantMap& payload, int width) const override;
};
