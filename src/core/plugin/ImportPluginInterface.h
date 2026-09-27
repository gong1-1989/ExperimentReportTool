/**
 * @file ImportPluginInterface.h
 * @brief 导入插件接口头文件
 *
 * 导入插件用于从外部文件导入数据，如 CSV、Excel、JSON 等。
 */

#ifndef IMPORT_PLUGIN_INTERFACE_H
#define IMPORT_PLUGIN_INTERFACE_H

#include "PluginInterface.h"
#include "core/models/DataTable.h"

class QWidget;

/**
 * @brief 导入插件接口
 *
 * 所有数据导入插件都需要实现此接口。
 */
class ImportPluginInterface : virtual public PluginInterface
{
public:
    virtual ~ImportPluginInterface() = default;

    /**
     * @brief 获取支持的导入格式
     * @return 格式列表，如 ["csv", "json"]
     */
    virtual QStringList supportedFormats() const = 0;

    /**
     * @brief 获取格式的显示名称
     * @param format 格式标识
     * @return 显示名称，如 "CSV 文件"
     */
    virtual QString formatDisplayName(const QString& format) const = 0;

    /**
     * @brief 获取文件过滤器（用于文件对话框）
     * @return 过滤器字符串，如 "CSV 文件 (*.csv)"
     */
    virtual QString fileFilter() const = 0;

    /**
     * @brief 从文件导入数据
     * @param filePath 文件路径
     * @param parent 父窗口（用于进度对话框）
     * @return 导入的数据表，失败返回空指针
     */
    virtual DataTable::Ptr importFromFile(const QString& filePath,
                                           QWidget* parent = nullptr) = 0;

    /**
     * @brief 是否支持预览
     * @return 支持返回 true
     */
    virtual bool supportsPreview() const { return false; }

    /**
     * @brief 预览导入数据（不实际导入）
     * @param filePath 文件路径
     * @param maxRows 最大预览行数
     * @return 预览数据，失败返回空指针
     */
    virtual DataTable::Ptr preview(const QString& filePath, int maxRows = 100) {
        Q_UNUSED(filePath);
        Q_UNUSED(maxRows);
        return nullptr;
    }
};

#define ImportPluginInterface_iid "com.examplereporttool.plugin.ImportPluginInterface"
Q_DECLARE_INTERFACE(ImportPluginInterface, ImportPluginInterface_iid)

#endif // IMPORT_PLUGIN_INTERFACE_H
