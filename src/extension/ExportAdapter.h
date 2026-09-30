/**
 * @file ExportAdapter.h
 * @brief 导出适配器契约（A 域：导入导出）
 *
 * 主程序把报告渲染成"内容流"（ReportRenderContext），导出插件只负责
 * 把内容流排版成目标格式。插件不接触数据库、不接触编辑器。
 *
 * 契约使用纯 Qt 类型，插件只需依赖 Qt 头文件，可独立编译为动态库，
 * 也可内置注册（registerBuiltinPlugin / ExportRegistry::registerAdapter）。
 */

#ifndef EXPORT_ADAPTER_H
#define EXPORT_ADAPTER_H

#include <QString>
#include <QStringList>
#include <QList>
#include <functional>

/**
 * @brief 数据表快照（供导出插件排版，纯 Qt 类型）
 */
struct ExportTable {
    QString name;            ///< 表名
    QStringList headers;     ///< 列名（含单位，如 "温度 (℃)"）
    QList<QStringList> rows; ///< 数据行（已字符串化）
    QStringList columnTypes; ///< 列类型名（"数值"/"文本"/"日期"/"布尔"）
};

/**
 * @brief 报告渲染上下文：主程序在 UI 线程构造（含数据库查询），后台线程消费
 */
struct ReportRenderContext {
    QString title;           ///< 报告标题
    QString creator;         ///< 创建者用户名
    QString statusName;      ///< 状态显示名（草稿/待审核/...）
    QString createdAt;       ///< 创建时间（yyyy-MM-dd HH:mm:ss）
    QString updatedAt;       ///< 更新时间
    QString htmlBody;        ///< 报告正文 HTML（对象已渲染为占位/图片）
    QString plainText;       ///< 报告正文纯文本（HtmlGenerator body → 去标签）
    QList<ExportTable> tables;   ///< 报告关联数据表
};

/**
 * @brief 导出适配器抽象契约
 */
class ExportAdapter
{
public:
    virtual ~ExportAdapter() = default;

    /// 显示名称（如 "Excel 工作簿"）
    virtual QString formatName() const = 0;
    /// 文件过滤器（如 "Excel 工作簿 (*.xlsx)"）
    virtual QString fileFilter() const = 0;
    /// 支持的文件扩展名（小写，无点，如 {"xlsx"}）
    virtual QStringList extensions() const = 0;

    /**
     * @brief 导出报告
     * @param ctx 渲染上下文（UI 线程构造好传入）
     * @param targetPath 输出路径
     * @param progress 进度回调（0-100，可为空）
     * @return 成功返回 true
     */
    virtual bool exportReport(const ReportRenderContext& ctx,
                              const QString& targetPath,
                              std::function<void(int)> progress = {}) = 0;

    /// 是否支持批量导出（批量导出时 ctx.tables 聚合多报告）
    virtual bool supportsBatch() const { return false; }
};

#endif // EXPORT_ADAPTER_H
