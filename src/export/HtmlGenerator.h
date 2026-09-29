/**
 * @file HtmlGenerator.h
 * @brief HTML 生成器头文件
 *
 * 负责将报告内容转换为 HTML，供 PDF/Word/HTML 导出共用。
 * 从 ExportManager 中分离，减少 ExportManager 的复杂度。
 */

#ifndef HTML_GENERATOR_H
#define HTML_GENERATOR_H

#include <QString>
#include "core/models/Report.h"

// 前向声明
struct ExportConfig;

/**
 * @brief HTML 生成器
 *
 * 将报告内容转换为带样式的 HTML 字符串（框架内置渲染）。
 */
class HtmlGenerator
{
public:
    explicit HtmlGenerator();
    ~HtmlGenerator();

    /**
     * @brief 将报告内容转换为完整 HTML 文档
     * @param report 报告
     * @param config 导出配置
     * @return 完整的 HTML 字符串（含 <html><head><body>）
     */
    QString generate(const Report::Ptr& report, const ExportConfig& config);

    /**
     * @brief 将报告内容转换为 HTML body（不含 <html><head>）
     * @param report 报告
     * @param config 导出配置
     * @return HTML body 内容
     */
    QString generateBody(const Report::Ptr& report, const ExportConfig& config);

private:
    /// 将报告内容转换为完整 HTML 文档（内部方法）
    QString reportToHtml(const Report::Ptr& report, const ExportConfig& config);

    /// 将单个内容块转换为 HTML

    /// 生成 CSS 样式表
    QString generateCss(const ExportConfig& config);

    // --- blockToHtml 拆分后的子方法（复杂块类型） ---
};

#endif // HTML_GENERATOR_H
