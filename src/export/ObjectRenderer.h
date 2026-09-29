#pragma once

#include "core/models/Report.h"   // ContentBlock / BlockType 定义于此
#include "core/models/DataTable.h"
#include <QString>
#include <QList>
#include <QCache>
#include <QPixmap>
#include <functional>

struct ChartConfig;  // chart/ChartConfigDialog.h 中定义，此处仅前置声明

// ===========================================================================
// 对象渲染器：对象锚点 → HTML 片段
//
// HtmlGenerator（HTML/Word 导出）与 PrintManager（打印/预览）共用同一套
// 对象渲染逻辑，通过 Options 参数化两个出口的差异：
//   - 图片引用方式：Base64 内嵌（导出） / 本地文件 URL（打印）
//   - 表格样式：CSS 类（导出） / 内联居中样式 + 灰底表头（打印）
//   - 图表输出：Base64 内嵌（导出） / 临时文件（打印）
//   - 空态/失败文案与段落模板
// 对齐（align）由调用方从锚点段落解析后传入，跟随编辑窗段落对齐设置。
// ===========================================================================
class ObjectRenderer
{
public:
    enum class ImageMode { Base64Embed, LocalFileUrl };
    enum class TableStyle { CssClass, InlineCentered };
    enum class ChartMode { Base64Embed, TempFile };

    struct Options {
        // ---- 图片 ----
        ImageMode imageMode = ImageMode::Base64Embed;
        QString cssClass = QStringLiteral("report-image"); // 图片段落 class（空 → 仅 align）
        int imageMaxWidth = 720;   // 打印页面适配最大宽（导出不限制传大值）
        int imageMaxHeight = 300;  // 打印页面适配最大高

        // ---- 表格 ----
        TableStyle tableStyle = TableStyle::CssClass;
        int maxTableRows = 100;    // 表格渲染行数上限

        // ---- 图表 ----
        ChartMode chartMode = ChartMode::Base64Embed;
        int chartMaxWidth = 720;
        int chartMaxHeight = 300;

        // ---- 公式（class 为空 → 内联斜体样式）----
        QString formulaClass = QStringLiteral("formula");

        // ---- 题注（class 为空 → 内联灰色小字样式）----
        QString captionClass = QStringLiteral("image-caption");

        // ---- 空态/失败占位：模板含 %1 为文案 ----
        QString emptyParaTemplate = QStringLiteral("<p class=\"empty-content\">%1</p>\n");
        QString imageMissingText   = QStringLiteral("[图片文件不存在]");
        QString imageReadFailText  = QStringLiteral("[图片读取失败]");
        QString tableMissingText   = QStringLiteral("[数据表不存在]");
        QString chartFailText      = QStringLiteral("[图表渲染失败]");
        QString sourceMissingText  = QStringLiteral("[数据源不存在]");
        QString chartNotConfiguredText = QStringLiteral("[未配置图表]");
    };

    // HtmlGenerator 导出（HTML/Word）：Base64 内嵌 + CSS 类
    static Options htmlExportOptions();
    // PrintManager 打印/预览：本地文件引用 + 内联样式 + 页面适配
    static Options printOptions();

    // 渲染单个对象为 HTML 片段
    // align：锚点段落对齐（left/center/right），由调用方解析传入
    // tableResolver：按 tableId 取数据表（两个出口查找逻辑不同，如打印带旧版 fallback）
    // outTempFiles：ChartMode::TempFile 下输出临时文件路径（调用方负责清理）
    static QString renderObject(const ContentBlock& object, const QString& align,
                                const Options& opt, qint64 ownerReportId,
                                const std::function<DataTable::Ptr(qint64)>& tableResolver,
                                QList<QString>* outTempFiles = nullptr);

private:
    // 图表渲染缓存：键 = (数据表id, 数据表更新时间, 配置JSON, 宽, 高)
    // 数据表内容变化（updatedAt 变化）或配置变化时自动失效，LRU 上限 64 张
    static QPixmap renderChartCached(const ChartConfig& config,
                                     const DataTable::Ptr& table,
                                     int width, int height);
};
