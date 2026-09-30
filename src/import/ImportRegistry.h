/**
 * @file ImportRegistry.h
 * @brief 导入适配器注册表
 *
 * - 内置适配器惰性注册（首次使用自动装载，无需手工初始化）
 * - registerAdapter() 为公开入口：未来第三方导入插件经 PluginManager
 *   加载后调用本方法注册，即可被主程序统一识别
 */

#ifndef IMPORT_REGISTRY_H
#define IMPORT_REGISTRY_H

#include <QString>
#include <QList>

#include "import/ImportAdapter.h"

class ImportRegistry
{
public:
    /// 注册适配器（注册表持有其指针，生命周期为程序运行期；插件适配器由 PluginManager 管理）
    static void registerAdapter(ImportAdapter* adapter);

    /// 按文件扩展名选择适配器；无匹配返回 nullptr
    static ImportAdapter* adapterForFile(const QString& filePath);

    /// 全部已注册适配器
    static QList<ImportAdapter*> allAdapters();

    /// 合并的文件选择过滤器（"全部支持格式 (*.csv *.txt);;CSV 文件 (*.csv *.txt)"）
    static QString combinedFileFilter();

private:
    /// 惰性装载内置适配器（目前：CSV）
    static void ensureBuiltinAdapters();

    /// 适配器容器（函数内静态，线程安全的首次初始化）
    static QList<ImportAdapter*>& adapters();
};

#endif // IMPORT_REGISTRY_H
