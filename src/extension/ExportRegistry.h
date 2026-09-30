/**
 * @file ExportRegistry.h
 * @brief 导出适配器注册表
 *
 * 插件（内置或动态库）通过 registerAdapter() 注册导出能力，
 * 主程序导出对话框通过 allAdapters() 自动聚合格式列表。
 */

#ifndef EXPORT_REGISTRY_H
#define EXPORT_REGISTRY_H

#include <QList>
#include <QString>

class ExportAdapter;

class ExportRegistry
{
public:
    static ExportRegistry& instance();

    /// 注册导出适配器（注册表不接管所有权）
    void registerAdapter(ExportAdapter* adapter);

    /// 惰性注册内置适配器（Excel/Markdown/JSON），幂等
    void ensureBuiltinAdapters();

    /// 按文件路径/扩展名查找适配器（找不到返回 nullptr）
    ExportAdapter* adapterForFile(const QString& filePath) const;

    /// 全部适配器（按注册顺序）
    QList<ExportAdapter*> allAdapters() const;

    /// 组合文件过滤器："报告 (*.pdf);;Excel 工作簿 (*.xlsx);;..."
    QString combinedFileFilter() const;

private:
    ExportRegistry() = default;
    static ExportRegistry* s_instance;
    QList<ExportAdapter*> m_adapters;
};

#endif // EXPORT_REGISTRY_H
