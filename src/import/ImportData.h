/**
 * @file ImportData.h
 * @brief 统一导入中间结构
 *
 * 任何导入格式（内置或未来插件）解析后都归一为原始行列文本，
 * 后续的预览、校验、入库逻辑只依赖本结构，与具体格式解耦。
 */

#ifndef IMPORT_DATA_H
#define IMPORT_DATA_H

#include <QString>
#include <QStringList>
#include <QList>

/**
 * @brief 统一导入中间结构
 */
struct ImportData {
    QList<QStringList> rows;   ///< 解析后的全部行（含表头行，若文件有表头）
    QStringList headers;       ///< 表头行（hasHeader 为 true 时有效）
    bool hasHeader = false;    ///< 文件是否含表头（语义由调用方/适配器决定）
    QString sourceFormat;      ///< 来源格式标识（"csv" 等，插件化扩展预留）
    QStringList warnings;      ///< 解析警告（不影响解析成功，如未知列等）
    int columnCount = 0;       ///< 列数
    int rowCount = 0;          ///< 行数

    /// 是否成功解析（rows 为空且失败时应结合 errorMessage 判断）
    bool isEmpty() const { return rows.isEmpty(); }
};

#endif // IMPORT_DATA_H
