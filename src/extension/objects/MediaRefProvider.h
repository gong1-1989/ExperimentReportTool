#pragma once
#include "extension/DocumentObjectProvider.h"

/**
 * @brief 音视频引用提供者（D 域首批）
 *
 * payload:
 *   - mediaType  video / audio
 *   - title     标题（文件名）
 *   - filePath  媒体文件绝对路径（渲染时检查存在性并生成本地 URL）
 */
class MediaRefProvider : public DocumentObjectProvider {
public:
    QString objectTypeId() const override { return QStringLiteral("media_ref"); }
    QString displayName() const override { return QStringLiteral("音视频引用"); }
    QString description() const override { return QStringLiteral("在报告中引用视频/音频文件（导出为 HTML5 播放器）"); }
    QVariantMap defaultPayload() const override;
    QString renderHtml(const QVariantMap& payload, const QString& align) const override;
    QString renderText(const QVariantMap& payload) const override;
    QPixmap renderPreview(const QVariantMap& payload, int width) const override;
};
