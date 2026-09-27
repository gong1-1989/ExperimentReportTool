/**
 * @file ExportPluginInterface.h
 * @brief 导出插件接口头文件
 *
 * 导出插件用于扩展报告导出格式，如 PDF、HTML、Word、纯文本等。
 * 新增一种导出格式只需要实现此接口并注册为插件即可。
 */

#ifndef EXPORT_PLUGIN_INTERFACE_H
#define EXPORT_PLUGIN_INTERFACE_H

#include "PluginInterface.h"
#include "core/models/Report.h"

// 前向声明
class ExportConfig;

/**
 * @brief 导出插件接口
 *
 * 实现此接口的插件可以为报告添加新的导出格式。
 *
 * @code
 * class PdfExportPlugin : public QObject, public ExportPluginInterface {
 *     Q_OBJECT
 *     Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.PdfExport")
 *     Q_INTERFACES(PluginInterface ExportPluginInterface)
 * public:
 *     QString format() const override { return "pdf"; }
 *     QString formatName() const override { return "PDF 文档"; }
 *     bool exportReport(const Report::Ptr& report, const ExportConfig& config) override;
 * };
 * @endcode
 */
class ExportPluginInterface : virtual public PluginInterface
{
public:
    virtual ~ExportPluginInterface() = default;

    // ========================================================================
    // 格式信息
    // ========================================================================

    /**
     * @brief 导出格式标识符
     * @return 格式字符串，如 "pdf"、"html"、"docx"
     */
    virtual QString format() const = 0;

    /**
     * @brief 格式显示名称
     * @return 显示名称，如 "PDF 文档"、"HTML 网页"
     */
    virtual QString formatName() const = 0;

    /**
     * @brief 文件扩展名
     * @return 扩展名，如 "pdf"、"html"、"docx"
     */
    virtual QString fileExtension() const = 0;

    /**
     * @brief 文件过滤器（用于文件对话框）
     * @return 过滤器字符串，如 "PDF 文档 (*.pdf)"
     */
    virtual QString fileFilter() const {
        return QString("%1 (*.%2)").arg(formatName(), fileExtension());
    }

    /**
     * @brief 格式描述
     * @return 描述文本
     */
    virtual QString formatDescription() const { return QString(); }

    // ========================================================================
    // 导出功能
    // ========================================================================

    /**
     * @brief 导出报告
     * @param report 要导出的报告
     * @param config 导出配置
     * @param parent 父窗口（用于显示进度对话框）
     * @return 成功返回 true，失败返回 false
     */
    virtual bool exportReport(const Report::Ptr& report,
                              const ExportConfig& config,
                              QWidget* parent = nullptr) = 0;

    /**
     * @brief 批量导出报告
     * @param reports 要导出的报告列表
     * @param config 导出配置
     * @param outputDir 输出目录
     * @param parent 父窗口
     * @return 成功导出的报告数量
     */
    virtual int exportReports(const QList<Report::Ptr>& reports,
                              const ExportConfig& config,
                              const QString& outputDir,
                              QWidget* parent = nullptr) {
        Q_UNUSED(reports);
        Q_UNUSED(config);
        Q_UNUSED(outputDir);
        Q_UNUSED(parent);
        return 0;  // 默认不支持批量导出
    }

    // ========================================================================
    // 能力
    // ========================================================================

    /**
     * @brief 是否支持批量导出
     * @return true 表示支持
     */
    virtual bool supportsBatchExport() const { return false; }

    /**
     * @brief 是否支持自定义配置
     * @return true 表示有额外的导出选项
     */
    virtual bool hasCustomConfig() const { return false; }

    /**
     * @brief 显示自定义配置对话框
     * @param config 导出配置（可修改）
     * @param parent 父窗口
     * @return 用户点击确定返回 true
     */
    virtual bool showConfigDialog(ExportConfig& config, QWidget* parent = nullptr) {
        Q_UNUSED(config);
        Q_UNUSED(parent);
        return true;
    }
};

#define EXPORT_PLUGIN_INTERFACE_IID "com.examplereporttool.ExportPluginInterface/1.0"
Q_DECLARE_INTERFACE(ExportPluginInterface, EXPORT_PLUGIN_INTERFACE_IID)

#endif // EXPORT_PLUGIN_INTERFACE_H
