#pragma once
// ===========================================================================
// 文档对象提供者契约（D 域）
// ---------------------------------------------------------------------------
// 使“报告结构化对象类型”可插拔：主程序负责对象的创建与数据准备
// （上传附件、文件路径解析等），提供者只负责按 payload 渲染
// （导出 HTML / 纯文本 / 编辑器预览位图）。
//
// 与 ExportAdapter / DataAnalyzer / StatsProvider / DocumentTool 同风格：
// 纯 Qt 类型、非 QObject、支持内置注册与动态库插件。
// ===========================================================================

#include <QPixmap>
#include <QVariant>
#include <QVariantMap>
#include <memory>

/**
 * @brief 文档对象提供者抽象契约
 *
 * 对象以 ContentBlock{type, data(QJsonObject)} 存储，data 即 payload。
 * 提供者通过 objectTypeId 与对象类型关联（如 BlockType::AttachmentCard
 * 对应 objectTypeId "attachment_card"）。
 */
class DocumentObjectProvider {
public:
    virtual ~DocumentObjectProvider() = default;

    /// 对象类型唯一 ID（注册键，如 "attachment_card" / "media_ref"）
    virtual QString objectTypeId() const = 0;
    /// UI 显示名（如 "附件卡片"）
    virtual QString displayName() const = 0;
    /// 说明
    virtual QString description() const = 0;
    /// 插入时的默认 payload（主程序创建对象时写入 data）
    virtual QVariantMap defaultPayload() const = 0;

    /**
     * @brief 导出 HTML 渲染（打印/HTML 导出共用）
     * @param payload 对象数据（含文件路径/名称/大小等，主程序构造）
     * @param align   段落对齐（left/center/right）
     */
    virtual QString renderHtml(const QVariantMap& payload, const QString& align) const = 0;
    /// 纯文本渲染（批量导出/统计摘要用，如 "[附件: xxx.pdf]"）
    virtual QString renderText(const QVariantMap& payload) const = 0;
    /// 编辑器预览位图（文档中对象占位显示）
    virtual QPixmap renderPreview(const QVariantMap& payload, int width) const = 0;
};

using DocumentObjectProviderPtr = std::shared_ptr<DocumentObjectProvider>;
