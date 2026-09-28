/**
 * @file CsvImporter.h
 * @brief CSV 文件导入工具（框架内置实现）
 *
 * 从 CSV 文件解析并构建 DataTable，供主窗口导入功能使用。
 * 原为独立插件 CsvImportPlugin，现并入框架以简化部署。
 */

#ifndef CSV_IMPORTER_H
#define CSV_IMPORTER_H

#include <QString>

#include "core/models/DataTable.h"

class CsvImporter
{
public:
    /// 支持的格式列表
    static QStringList supportedFormats();

    /// 文件选择过滤器
    static QString fileFilter();

    /// 格式显示名称
    static QString formatDisplayName(const QString& format);

    /**
     * @brief 从文件导入数据表
     * @param filePath 文件路径
     * @param errorMessage 输出错误信息（可选）
     * @return 数据表指针，失败返回 nullptr
     */
    static DataTable::Ptr importFile(const QString& filePath, QString* errorMessage = nullptr);

    /**
     * @brief 预览导入结果（限制最大行数）
     * @param filePath 文件路径
     * @param maxRows 最大行数
     * @return 数据表指针
     */
    static DataTable::Ptr preview(const QString& filePath, int maxRows = 100);

private:
    CsvImporter() = delete;
    ~CsvImporter() = delete;
};

#endif // CSV_IMPORTER_H
