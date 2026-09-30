/**
 * @file ImportValidator.h
 * @brief 导入校验管道（纯逻辑，可单元测试，不依赖 UI）
 *
 * 校验规则（对目标表列定义）：
 * - 类型可转换：Number 必须可转数值、Date 必须可解析、Boolean 必须为是/否等
 * - 必填：required 列空值报错
 * - 范围：Number 列按 minValue/maxValue 检查
 *
 * 列匹配策略（按列名）：文件列顺序可乱；未知列忽略并提示；缺必填列报错。
 */

#ifndef IMPORT_VALIDATOR_H
#define IMPORT_VALIDATOR_H

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

#include "import/ImportData.h"
#include "core/models/DataTable.h"   // ColumnDefinition

/**
 * @brief 单元格校验问题
 */
struct CellIssue {
    enum class Level { Warning, Error };

    int row = -1;        ///< 数据行号（相对数据起始行，0-based，不含表头）
    int col = -1;        ///< 文件列号
    QString message;     ///< 问题描述
    Level level = Level::Error;
};

/**
 * @brief 导入校验器（静态工具类）
 */
class ImportValidator
{
public:
    /**
     * @brief 按列名构建文件列 → 目标列 映射
     * @param fileHeaders 文件表头行（已 trim 处理）
     * @param targetColumns 目标表列定义
     * @param missingRequired 输出：未被匹配到的必填列名（Error 级问题）
     * @param unknownColumns 输出：文件中的未知列名（Warning 级，忽略）
     * @return 映射：文件列号 → 目标列索引；未匹配列不在映射中
     */
    static QHash<int, int> buildColumnMapping(const QStringList& fileHeaders,
                                              const QList<ColumnDefinition>& targetColumns,
                                              QStringList* missingRequired = nullptr,
                                              QStringList* unknownColumns = nullptr);

    /**
     * @brief 校验导入数据
     * @param data 统一导入数据
     * @param hasHeader 文件是否含表头（决定数据起始行）
     * @param targetColumns 目标表列定义
     * @param columnMapping 文件列 → 目标列 映射（buildColumnMapping 结果）
     * @return 校验问题列表（Error 阻止导入，Warning 仅提示）
     */
    static QList<CellIssue> validate(const ImportData& data, bool hasHeader,
                                     const QList<ColumnDefinition>& targetColumns,
                                     const QHash<int, int>& columnMapping);

private:
    /// 单格类型校验；不通过时 why 输出原因
    static bool matchesType(ColumnType type, const QString& raw, QString* why);
};

#endif // IMPORT_VALIDATOR_H
