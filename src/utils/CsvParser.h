/**
 * @file CsvParser.h
 * @brief CSV 解析器头文件
 *
 * 支持标准 CSV 格式解析，包括：
 * - 逗号分隔
 * - 引号包裹的字段（支持字段内包含逗号、换行、引号）
 * - 转义引号（"" 表示一个引号）
 * - 自动检测分隔符（逗号、分号、制表符）
 * - 编码检测（UTF-8 / GBK）
 */

#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QChar>
#include <QIODevice>

/**
 * @brief CSV 解析结果
 */
struct CsvParseResult {
    bool success;              ///< 是否成功
    QString errorMessage;      ///< 错误信息
    int errorLine;             ///< 错误行号（从1开始）
    QList<QStringList> rows;   ///< 解析后的数据行
    int columnCount;           ///< 列数
    int rowCount;              ///< 行数
    bool hasHeader;            ///< 是否有表头（第一行）

    CsvParseResult()
        : success(false)
        , errorLine(0)
        , columnCount(0)
        , rowCount(0)
        , hasHeader(true)
    {}
};

/**
 * @brief CSV 解析器
 *
 * 使用方式：
 * @code
 *   CsvParser parser;
 *   parser.setDelimiter(',');
 *   CsvParseResult result = parser.parseFile("/path/to/data.csv");
 *   if (result.success) {
 *       for (const QStringList& row : result.rows) {
 *           // 处理每一行
 *       }
 *   }
 * @endcode
 */
class CsvParser
{
public:
    explicit CsvParser();
    ~CsvParser();

    /**
     * @brief 设置分隔符
     * @param delimiter 分隔符（默认自动检测）
     */
    void setDelimiter(QChar delimiter) { m_delimiter = delimiter; m_autoDetect = false; }

    /**
     * @brief 设置是否自动检测分隔符
     */
    void setAutoDetect(bool autoDetect) { m_autoDetect = autoDetect; }

    /**
     * @brief 设置引号字符
     */
    void setQuoteChar(QChar quote) { m_quoteChar = quote; }

    /**
     * @brief 设置是否有表头
     */
    void setHasHeader(bool hasHeader) { m_hasHeader = hasHeader; }

    /**
     * @brief 解析文件
     * @param filePath 文件路径
     * @return 解析结果
     */
    CsvParseResult parseFile(const QString& filePath);

    /**
     * @brief 解析字符串
     * @param content CSV 内容
     * @return 解析结果
     */
    CsvParseResult parseString(const QString& content);

    /**
     * @brief 解析 QIODevice
     * @param device 输入设备
     * @return 解析结果
     */
    CsvParseResult parseDevice(QIODevice* device);

    /**
     * @brief 自动检测分隔符
     * @param sample 样本数据（前几行）
     * @return 检测到的分隔符
     */
    static QChar detectDelimiter(const QString& sample);

    /**
     * @brief 将数据导出为 CSV 字符串
     * @param rows 数据行
     * @param delimiter 分隔符
     * @return CSV 字符串
     */
    static QString toCsv(const QList<QStringList>& rows, QChar delimiter = ',');

    /**
     * @brief 转义 CSV 字段
     * @param field 字段内容
     * @param delimiter 分隔符
     * @return 转义后的字段
     */
    static QString escapeField(const QString& field, QChar delimiter = ',');

private:
    /**
     * @brief 解析一行 CSV
     * @param line 行内容
     * @param row 输出行数据
     * @return 成功返回 true
     */
    bool parseLine(const QString& line, QStringList& row);

    /**
     * @brief 读取文件内容，自动检测编码
     */
    QString readFileContent(const QString& filePath);

    // -----------------------------------------------------------------------
    // 成员变量
    // -----------------------------------------------------------------------

    QChar m_delimiter;     ///< 分隔符
    QChar m_quoteChar;     ///< 引号字符
    bool m_autoDetect;      ///< 是否自动检测分隔符
    bool m_hasHeader;       ///< 是否有表头
};

#endif // CSV_PARSER_H
