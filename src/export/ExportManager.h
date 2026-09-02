/**
 * @file ExportManager.h
 * @brief 导出管理器头文件
 *
 * 负责将报告导出为多种格式：PDF、HTML、Word(.docx)、纯文本。
 * 统一的导出入口，内部根据格式调用对应的导出器。
 */

#ifndef EXPORT_MANAGER_H
#define EXPORT_MANAGER_H

#include <QString>
#include <QWidget>

#include "core/models/Report.h"

/**
 * @brief 导出格式枚举
 */
enum class ExportFormat {
    Pdf,    ///< PDF 格式
    Html,   ///< HTML 格式
    Word,   ///< Word (.docx) 格式
    Text    ///< 纯文本格式
};

/**
 * @brief 导出配置结构体
 */
struct ExportConfig {
    ExportFormat format;       ///< 导出格式
    QString filePath;          ///< 输出文件路径
    bool includeTitle;         ///< 是否包含标题
    bool includeMeta;          ///< 是否包含元信息（作者、日期等）
    bool includeTableOfContents; ///< 是否包含目录
    QString pageSize;          ///< 页面大小（A4, Letter 等）
    QString fontFamily;        ///< 字体
    int fontSize;              ///< 基础字号
    bool enableSyntaxHighlight; ///< 代码块语法高亮

    ExportConfig()
        : format(ExportFormat::Pdf)
        , includeTitle(true)
        , includeMeta(true)
        , includeTableOfContents(false)
        , pageSize("A4")
        , fontFamily("Microsoft YaHei")
        , fontSize(12)
        , enableSyntaxHighlight(true)
    {}
};

/**
 * @brief 导出管理器
 *
 * 使用方式：
 * @code
 *   ExportManager exporter;
 *   ExportConfig config;
 *   config.format = ExportFormat::Pdf;
 *   config.filePath = "/path/to/output.pdf";
 *   bool ok = exporter.exportReport(report, config, parentWidget);
 * @endcode
 */
class ExportManager
{
public:
    explicit ExportManager();
    ~ExportManager();

    /**
     * @brief 导出报告
     * @param report 要导出的报告
     * @param config 导出配置
     * @param parent 父窗口（用于错误对话框）
     * @return 成功返回 true
     */
    bool exportReport(const Report::Ptr& report,
                       const ExportConfig& config,
                       QWidget* parent = nullptr);

    /**
     * @brief 导出报告（便捷方法，使用默认配置）
     * @param report 报告
     * @param format 导出格式
     * @param filePath 输出路径
     * @param parent 父窗口
     * @return 成功返回 true
     */
    bool exportReport(const Report::Ptr& report,
                       ExportFormat format,
                       const QString& filePath,
                       QWidget* parent = nullptr);

    /**
     * @brief 获取导出格式的文件过滤器（用于文件对话框）
     * @param format 格式
     * @return 文件过滤器字符串，如 "PDF 文件 (*.pdf)"
     */
    static QString formatFilter(ExportFormat format);

    /**
     * @brief 获取导出格式的默认扩展名
     * @param format 格式
     * @return 扩展名，如 ".pdf"
     */
    static QString formatExtension(ExportFormat format);

    /**
     * @brief 获取所有支持的格式列表
     * @return 格式列表
     */
    static QList<ExportFormat> supportedFormats();

    /**
     * @brief 获取格式显示名称
     * @param format 格式
     * @return 显示名称
     */
    static QString formatDisplayName(ExportFormat format);

    /**
     * @brief 显示导出文件对话框，返回选择的文件路径和格式
     * @param parent 父窗口
     * @param defaultName 默认文件名
     * @return 一对值：(文件路径, 格式)。用户取消时路径为空。
     */
    static QPair<QString, ExportFormat> getSaveFilePath(QWidget* parent,
                                                           const QString& defaultName);

private:
    // 各格式导出方法
    bool exportToPdf(const Report::Ptr& report, const ExportConfig& config, QWidget* parent);
    bool exportToHtml(const Report::Ptr& report, const ExportConfig& config, QWidget* parent);
    bool exportToWord(const Report::Ptr& report, const ExportConfig& config, QWidget* parent);
    bool exportToText(const Report::Ptr& report, const ExportConfig& config, QWidget* parent);

    // 将报告内容转换为 HTML（供 PDF/Word/HTML 导出共用）
    QString reportToHtml(const Report::Ptr& report, const ExportConfig& config);

    // 将内容块转换为 HTML
    QString blockToHtml(const ContentBlock& block, int& headingCounter);

    // 生成 CSS 样式
    QString generateCss(const ExportConfig& config);
};

#endif // EXPORT_MANAGER_H
