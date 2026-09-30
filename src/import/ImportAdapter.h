/**
 * @file ImportAdapter.h
 * @brief 导入契约（适配器抽象接口）
 *
 * 任何导入格式（内置 CSV 或未来插件格式）都实现此接口：
 * - parse()：把文件解析为统一中间结构 ImportData（纯逻辑、无 UI，可后台线程运行）
 * - buildTemplate()：按目标表列定义生成标准模板（模板本身是合法可导入文件）
 *
 * 插件化扩展：未来第三方格式 = 插件类同时继承 PluginInterface 与本接口，
 * 由 PluginManager 加载后经 ImportRegistry::registerAdapter() 注册，主程序零改动。
 */

#ifndef IMPORT_ADAPTER_H
#define IMPORT_ADAPTER_H

#include <QByteArray>
#include <QHash>
#include <QVariant>
#include <QString>
#include <QStringList>

#include "import/ImportData.h"
#include "core/models/DataTable.h"   // ColumnDefinition

/**
 * @brief 导入适配器抽象基类
 */
class ImportAdapter
{
public:
    virtual ~ImportAdapter() = default;

    /// 格式显示名称（如 "CSV 文件"）
    virtual QString formatName() const = 0;

    /// 支持的文件扩展名（小写，不含点，如 {"csv","txt"}）
    virtual QStringList extensions() const = 0;

    /// 文件选择过滤器（如 "CSV 文件 (*.csv *.txt)"）
    virtual QString fileFilter() const = 0;

    /**
     * @brief 解析文件为统一中间结构
     * @param filePath 文件路径
     * @param errorMessage 输出错误信息（可选）
     * @param options 解析选项（格式相关的键值参数；CSV 支持 "delimiter": QChar，
     *                无效/缺省表示自动检测；其他格式可自定义扩展）
     * @return 解析结果；失败时 rows 为空且 errorMessage 被填充
     */
    virtual ImportData parse(const QString& filePath,
                             QString* errorMessage = nullptr,
                             const QHash<QString, QVariant>& options = {}) const = 0;

    /**
     * @brief 按目标表列定义生成标准模板（该模板本身是一份合法可导入文件）
     * @param columns 目标表列定义（表头列名/类型/必填/范围/单位）
     * @return 模板内容（编码由格式决定）
     */
    virtual QByteArray buildTemplate(const QList<ColumnDefinition>& columns) const = 0;
};

#endif // IMPORT_ADAPTER_H
