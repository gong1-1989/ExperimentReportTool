/**
 * @file PrintManager.h
 * @brief 打印管理器头文件
 *
 * 负责报告的打印预览、页面设置和打印功能。
 * 基于 QPrinter + QTextDocument 实现，支持页眉页脚、页码、页边距等。
 */

#ifndef PRINT_MANAGER_H
#define PRINT_MANAGER_H

#include <QObject>
#include <QString>
#include <QPrinter>
#include <QPageSize>
#include <QPageLayout>
#include <QTextDocument>

#include "core/models/Report.h"
#include "export/ExportManager.h"

/**
 * @brief 打印配置结构体
 */
struct PrintConfig {
    QPageSize::PageSizeId pageSize;   ///< 页面大小
    QPageLayout::Orientation orientation; ///< 方向
    QMarginsF margins;                  ///< 页边距（毫米）
    bool printHeader;                   ///< 是否打印页眉
    bool printFooter;                   ///< 是否打印页脚
    bool printPageNumbers;              ///< 是否打印页码
    QString headerText;                 ///< 页眉文本
    QString footerText;                 ///< 页脚文本
    bool includeTitle;                  ///< 是否包含标题
    bool includeMeta;                   ///< 是否包含元信息
    bool includeTableOfContents;        ///< 是否包含目录
    int copies;                         ///< 打印份数
    bool collate;                       ///< 是否逐份打印
    QPrinter::PrintRange printRange;    ///< 打印范围
    int fromPage;                       ///< 起始页
    int toPage;                         ///< 结束页

    PrintConfig()
        : pageSize(QPageSize::A4)
        , orientation(QPageLayout::Portrait)
        , margins(20, 20, 20, 20)
        , printHeader(true)
        , printFooter(true)
        , printPageNumbers(true)
        , headerText("")
        , footerText("")
        , includeTitle(true)
        , includeMeta(true)
        , includeTableOfContents(false)
        , copies(1)
        , collate(true)
        , printRange(QPrinter::AllPages)
        , fromPage(1)
        , toPage(1)
    {}
};

/**
 * @brief 打印管理器
 *
 * 使用方式：
 * @code
 *   PrintManager printer;
 *   printer.printPreview(report, this);  // 打印预览
 *   printer.print(report, this);          // 直接打印
 * @endcode
 */
class PrintManager : public QObject
{
    Q_OBJECT

public:
    explicit PrintManager(QObject* parent = nullptr);
    ~PrintManager() override;

    /**
     * @brief 显示打印预览对话框
     * @param report 要打印的报告
     * @param parent 父窗口
     * @return 用户确认打印返回 true
     */
    bool printPreview(const Report::Ptr& report, QWidget* parent = nullptr);

    /**
     * @brief 显示打印对话框并打印
     * @param report 要打印的报告
     * @param parent 父窗口
     * @return 成功返回 true
     */
    bool print(const Report::Ptr& report, QWidget* parent = nullptr);

    /**
     * @brief 使用指定配置直接打印（不显示对话框）
     * @param report 报告
     * @param config 打印配置
     * @param parent 父窗口
     * @return 成功返回 true
     */
    bool printWithConfig(const Report::Ptr& report,
                          const PrintConfig& config,
                          QWidget* parent = nullptr);

    /**
     * @brief 显示页面设置对话框
     * @param config 打印配置（输入/输出）
     * @param parent 父窗口
     * @return 用户确认返回 true
     */
    bool pageSetup(PrintConfig& config, QWidget* parent = nullptr);

    /**
     * @brief 获取当前打印配置
     */
    PrintConfig currentConfig() const { return m_config; }

    /**
     * @brief 设置打印配置
     */
    void setConfig(const PrintConfig& config) { m_config = config; }

private:
    /**
     * @brief 配置 QPrinter
     */
    void setupPrinter(QPrinter& printer, const PrintConfig& config);

    /**
     * @brief 将报告渲染到 QTextDocument
     */
    QTextDocument* renderDocument(const Report::Ptr& report, const PrintConfig& config);

    /**
     * @brief 打印页眉页脚
     */
    void printHeaderFooter(QPrinter* printer, QPainter* painter, int page, int totalPages);

    // -----------------------------------------------------------------------
    // 成员变量
    // -----------------------------------------------------------------------

    PrintConfig m_config;  ///< 当前打印配置
};

#endif // PRINT_MANAGER_H
