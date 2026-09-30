/**
 * @file CsvImportAdapter.h
 * @brief CSV 导入适配器（内置实现）
 *
 * 包装底层 CsvParser（分隔符自动检测/引号/编码/表头全部复用），
 * 向上提供 ImportAdapter 契约；同时承担 CSV 标准模板生成。
 */

#ifndef CSV_IMPORT_ADAPTER_H
#define CSV_IMPORT_ADAPTER_H

#include "import/ImportAdapter.h"

class CsvImportAdapter : public ImportAdapter
{
public:
    QString formatName() const override;
    QStringList extensions() const override;
    QString fileFilter() const override;

    ImportData parse(const QString& filePath,
                     QString* errorMessage = nullptr,
                     const QHash<QString, QVariant>& options = {}) const override;

    QByteArray buildTemplate(const QList<ColumnDefinition>& columns) const override;
};

#endif // CSV_IMPORT_ADAPTER_H
