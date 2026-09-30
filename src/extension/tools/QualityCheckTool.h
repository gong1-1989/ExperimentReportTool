#pragma once
#include "extension/DocumentTool.h"

/**
 * @brief 文本质量检查工具：重复词、超长句、中英混排缺空格、连续空行
 */
class QualityCheckTool : public DocumentTool {
public:
    QString toolId() const override { return QStringLiteral("quality_check"); }
    QString displayName() const override { return QStringLiteral("质量检查"); }
    QString description() const override { return QStringLiteral("检查重复词、超长句、中英混排缺空格、连续空行"); }
    DocumentToolResult execute(const DocumentContext& ctx, QString* error) const override;
};
