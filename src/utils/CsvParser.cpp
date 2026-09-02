/**
 * @file CsvParser.cpp
 * @brief CSV 解析器实现文件
 */

#include "CsvParser.h"
#include "core/utils/Logger.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QStringConverter>

// ===========================================================================
// 构造与析构
// ===========================================================================

CsvParser::CsvParser()
    : m_delimiter(',')
    , m_quoteChar('"')
    , m_autoDetect(true)
    , m_hasHeader(true)
{
}

CsvParser::~CsvParser()
{
}

// ===========================================================================
// 文件解析
// ===========================================================================

CsvParseResult CsvParser::parseFile(const QString& filePath)
{
    CsvParseResult result;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.errorMessage = QString("无法打开文件: %1").arg(filePath);
        return result;
    }

    result = parseDevice(&file);
    file.close();
    return result;
}

// ===========================================================================
// 字符串解析
// ===========================================================================

CsvParseResult CsvParser::parseString(const QString& content)
{
    CsvParseResult result;

    if (content.isEmpty()) {
        result.errorMessage = "内容为空";
        return result;
    }

    // 自动检测分隔符
    if (m_autoDetect) {
        m_delimiter = detectDelimiter(content.left(1000));
    }

    // 按行分割（需要处理引号内的换行）
    QList<QStringList> rows;
    QString currentRow;
    bool inQuotes = false;
    int lineNumber = 0;

    const QStringList lines = content.split('\n');

    for (const QString& rawLine : lines) {
        QString line = rawLine;
        // 移除行尾的 \r
        if (line.endsWith('\r')) {
            line.chop(1);
        }

        if (line.isEmpty() && !inQuotes) {
            continue;  // 跳过空行
        }

        ++lineNumber;

        if (inQuotes) {
            // 引号内的换行，追加到当前行
            currentRow += "\n" + line;
        } else {
            currentRow = line;
        }

        // 检查引号是否闭合
        int quoteCount = currentRow.count(m_quoteChar);
        inQuotes = (quoteCount % 2 != 0);

        if (!inQuotes) {
            // 一行完整，解析
            QStringList row;
            if (!parseLine(currentRow, row)) {
                result.errorMessage = QString("第 %1 行解析失败").arg(lineNumber);
                result.errorLine = lineNumber;
                return result;
            }
            if (!row.isEmpty()) {
                rows.append(row);
            }
            currentRow.clear();
        }
    }

    if (inQuotes) {
        result.errorMessage = "文件末尾引号未闭合";
        result.errorLine = lineNumber;
        return result;
    }

    result.success = true;
    result.rows = rows;
    result.rowCount = rows.size();
    result.columnCount = rows.isEmpty() ? 0 : rows.first().size();
    result.hasHeader = m_hasHeader;

    return result;
}

// ===========================================================================
// QIODevice 解析
// ===========================================================================

CsvParseResult CsvParser::parseDevice(QIODevice* device)
{
    if (!device || !device->isReadable()) {
        CsvParseResult result;
        result.errorMessage = "设备不可读";
        return result;
    }

    // 读取全部内容
    QByteArray data = device->readAll();

    // 检测 BOM 和编码
    QString content;
    if (data.startsWith("\xEF\xBB\xBF")) {
        // UTF-8 BOM：直接去掉 BOM 后用 UTF-8 解码
        content = QString::fromUtf8(data.mid(3));
    } else {
        // 无 BOM：尝试自动检测编码
        // 使用 QStringConverter::encodingForData 检测 UTF-8
        auto detected = QStringConverter::encodingForData(data);

        if (detected.has_value()== QStringConverter::Utf8) {
            // 检测为 UTF-8
            QStringDecoder decoder(detected.value());
            content = decoder(data);
        }else {
            // 不是有效的 UTF-8，尝试使用系统本地编码
            // 在中文 Windows 上通常是 GBK/GB2312
            // 在 Linux/macOS 上通常是 UTF-8
            QStringDecoder decoder(QStringConverter::System);
            content = decoder(data);

            // 如果系统编码解码后仍有乱码，回退到 fromLocal8Bit
            if (content.contains(QChar(0xFFFD))) {
                content = QString::fromLocal8Bit(data);
            }
        }
    }

    return parseString(content);
}

// ===========================================================================
// 单行解析
// ===========================================================================

bool CsvParser::parseLine(const QString& line, QStringList& row)
{
    row.clear();
    QString field;
    bool inQuotes = false;
    int i = 0;

    while (i < line.length()) {
        const QChar c = line.at(i);

        if (inQuotes) {
            if (c == m_quoteChar) {
                // 检查是否是转义引号（""）
                if (i + 1 < line.length() && line.at(i + 1) == m_quoteChar) {
                    field += m_quoteChar;
                    i += 2;
                    continue;
                }
                // 引号结束
                inQuotes = false;
                ++i;
                continue;
            }
            field += c;
            ++i;
        } else {
            if (c == m_quoteChar) {
                // 字段以引号开头
                inQuotes = true;
                ++i;
                continue;
            }
            if (c == m_delimiter) {
                // 字段结束
                row.append(field);
                field.clear();
                ++i;
                continue;
            }
            field += c;
            ++i;
        }
    }

    // 添加最后一个字段
    row.append(field);

    return true;
}

// ===========================================================================
// 分隔符自动检测
// ===========================================================================

QChar CsvParser::detectDelimiter(const QString& sample)
{
    if (sample.isEmpty()) return ',';

    // 统计各种分隔符的出现频率
    const QList<QChar> candidates = {',', ';', '\t', '|'};
    QChar best = ',';
    int bestCount = 0;

    for (QChar delimiter : candidates) {
        int count = 0;
        bool inQuotes = false;

        for (int i = 0; i < sample.length(); ++i) {
            const QChar c = sample.at(i);
            if (c == '"') {
                inQuotes = !inQuotes;
            } else if (!inQuotes && c == delimiter) {
                ++count;
            }
        }

        // 计算每行的平均分隔符数
        const int lineCount = sample.count('\n') + 1;
        const double avgPerLine = static_cast<double>(count) / lineCount;

        // 优先选择每行至少有一个分隔符的
        if (avgPerLine >= 1.0 && count > bestCount) {
            bestCount = count;
            best = delimiter;
        }
    }

    return best;
}

// ===========================================================================
// CSV 导出
// ===========================================================================

QString CsvParser::toCsv(const QList<QStringList>& rows, QChar delimiter)
{
    QString csv;
    QTextStream stream(&csv);

    // 写入 BOM（Excel 兼容）
    stream << "\xEF\xBB\xBF";

    for (const QStringList& row : rows) {
        QStringList escapedRow;
        for (const QString& field : row) {
            escapedRow.append(escapeField(field, delimiter));
        }
        stream << escapedRow.join(delimiter) << "\n";
    }

    stream.flush();
    return csv;
}

QString CsvParser::escapeField(const QString& field, QChar delimiter)
{
    // 如果字段包含分隔符、引号、换行，则需要用引号包裹
    if (field.contains(delimiter) || field.contains('"') ||
        field.contains('\n') || field.contains('\r')) {
        // 转义内部引号
        QString escaped = field;
        escaped.replace('"', "\"\"");
        return "\"" + escaped + "\"";
    }
    return field;
}
