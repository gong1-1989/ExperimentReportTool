/**
 * @file Report.cpp
 * @brief 实验报告实体类实现文件
 */

#include "Report.h"
#include "core/utils/AppConstants.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QUuid>
#include <QRegularExpression>

// ===========================================================================
// ContentBlock 实现
// ===========================================================================

QJsonObject ContentBlock::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["type"] = blockTypeToString(type);
    obj["data"] = data;  // data 本身就是 QJsonObject
    return obj;
}

ContentBlock ContentBlock::fromJson(const QJsonObject& json)
{
    ContentBlock block;
    block.id = json.value("id").toString();
    block.type = blockTypeFromString(json.value("type").toString());
    block.data = json.value("data").toObject();

    // 如果没有 ID，生成一个
    if (block.id.isEmpty()) {
        block.id = Report::generateBlockId();
    }

    return block;
}

QString ContentBlock::blockTypeToString(BlockType type)
{
    switch (type) {
    case BlockType::Heading1:      return "heading1";
    case BlockType::Heading2:      return "heading2";
    case BlockType::Heading3:      return "heading3";
    case BlockType::Paragraph:     return "paragraph";
    case BlockType::BulletList:    return "bullet_list";
    case BlockType::NumberedList:  return "numbered_list";
    case BlockType::Table:         return "table";
    case BlockType::Image:         return "image";
    case BlockType::Chart:         return "chart";
    case BlockType::CodeBlock:     return "code_block";
    case BlockType::Formula:       return "formula";
    case BlockType::Quote:         return "quote";
    case BlockType::Divider:       return "divider";
    case BlockType::DataReference: return "data_reference";
    }
    return "paragraph";  // 默认
}

BlockType ContentBlock::blockTypeFromString(const QString& str)
{
    if (str == "heading1")      return BlockType::Heading1;
    if (str == "heading2")      return BlockType::Heading2;
    if (str == "heading3")      return BlockType::Heading3;
    if (str == "bullet_list")   return BlockType::BulletList;
    if (str == "numbered_list") return BlockType::NumberedList;
    if (str == "table")         return BlockType::Table;
    if (str == "image")         return BlockType::Image;
    if (str == "chart")         return BlockType::Chart;
    if (str == "code_block")    return BlockType::CodeBlock;
    if (str == "formula")       return BlockType::Formula;
    if (str == "quote")         return BlockType::Quote;
    if (str == "divider")       return BlockType::Divider;
    if (str == "data_reference")return BlockType::DataReference;
    return BlockType::Paragraph;  // 默认段落
}

// ===========================================================================
// Report 工厂方法与构造
// ===========================================================================

Report::Ptr Report::create()
{
    return Ptr(new Report());
}

Report::Report()
    : m_id(-1)
    , m_projectId(-1)
    , m_templateId(-1)
    , m_status(ReportStatus::Draft)  // 新报告默认为草稿
    , m_experimentDate(QDate::currentDate())
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
{
}

Report::~Report()
{
}

// ===========================================================================
// 内容块操作
// ===========================================================================

const ContentBlock& Report::blockAt(int index) const
{
    // 静态空块，用于越界访问时返回（避免悬垂引用）
    static const ContentBlock s_emptyBlock;

    if (index >= 0 && index < m_blocks.size()) {
        return m_blocks.at(index);
    }
    return s_emptyBlock;
}

void Report::appendBlock(const ContentBlock& block)
{
    ContentBlock b = block;
    // 确保块有 ID
    if (b.id.isEmpty()) {
        b.id = generateBlockId();
    }
    m_blocks.append(b);
    m_updatedAt = QDateTime::currentDateTime();
}

void Report::insertBlock(int index, const ContentBlock& block)
{
    ContentBlock b = block;
    if (b.id.isEmpty()) {
        b.id = generateBlockId();
    }
    // 边界检查：index 小于 0 则插入开头，大于等于 size 则追加
    if (index < 0) index = 0;
    if (index >= m_blocks.size()) {
        m_blocks.append(b);
    } else {
        m_blocks.insert(index, b);
    }
    m_updatedAt = QDateTime::currentDateTime();
}

void Report::replaceBlock(int index, const ContentBlock& block)
{
    if (index >= 0 && index < m_blocks.size()) {
        ContentBlock b = block;
        // 保留原块的 ID（如果新块没有指定 ID）
        if (b.id.isEmpty()) {
            b.id = m_blocks.at(index).id;
        }
        m_blocks.replace(index, b);
        m_updatedAt = QDateTime::currentDateTime();
    }
}

void Report::removeBlock(int index)
{
    if (index >= 0 && index < m_blocks.size()) {
        m_blocks.removeAt(index);
        m_updatedAt = QDateTime::currentDateTime();
    }
}

void Report::moveBlock(int from, int to)
{
    // 边界检查
    if (from < 0 || from >= m_blocks.size()) return;
    if (to < 0 || to >= m_blocks.size()) return;
    if (from == to) return;

    // 使用 move 语义（QList 内部优化）
    ContentBlock block = m_blocks.takeAt(from);
    m_blocks.insert(to, block);
    m_updatedAt = QDateTime::currentDateTime();
}

void Report::clearBlocks()
{
    m_blocks.clear();
    m_updatedAt = QDateTime::currentDateTime();
}

// ===========================================================================
// 内容序列化
// ===========================================================================

QString Report::contentToJson() const
{
    QJsonArray array;
    for (const ContentBlock& block : m_blocks) {
        array.append(block.toJson());
    }

    QJsonDocument doc(array);
    // 紧凑格式（不缩进），节省存储空间
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

void Report::contentFromJson(const QString& json)
{
    m_blocks.clear();

    if (json.isEmpty()) return;

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        // JSON 解析失败，记录错误但不抛出异常
        // 调用方可以通过日志系统记录
        return;
    }

    if (!doc.isArray()) return;

    QJsonArray array = doc.array();
    for (const QJsonValue& value : array) {
        if (value.isObject()) {
            m_blocks.append(ContentBlock::fromJson(value.toObject()));
        }
    }
}

QString Report::toPlainText() const
{
    QStringList texts;

    for (const ContentBlock& block : m_blocks) {
        switch (block.type) {
        case BlockType::Heading1:
        case BlockType::Heading2:
        case BlockType::Heading3:
        case BlockType::Paragraph:
        case BlockType::Quote:
            // 这些块的文本都在 data["text"] 中
            texts.append(block.data.value("text").toString());
            break;

        case BlockType::BulletList:
        case BlockType::NumberedList:
            // 列表块的 data["items"] 是字符串数组
            if (block.data.value("items").isArray()) {
                const QJsonArray items = block.data.value("items").toArray();
                for (const QJsonValue& item : items) {
                    texts.append(item.toString());
                }
            }
            break;

        case BlockType::CodeBlock:
            // 代码块的内容在 data["code"] 中
            texts.append(block.data.value("code").toString());
            break;

        case BlockType::Formula:
            // 公式内容在 data["latex"] 中
            texts.append(block.data.value("latex").toString());
            break;

        case BlockType::Image:
            // 图片的说明文字在 data["caption"] 中
            texts.append(block.data.value("caption").toString());
            break;

        case BlockType::Table:
        case BlockType::Chart:
        case BlockType::DataReference:
        case BlockType::Divider:
            // 这些块没有可索引的文本内容，跳过
            break;
        }
    }

    return texts.join("\n");
}

int Report::wordCount() const
{
    const QString plainText = toPlainText();
    if (plainText.isEmpty()) return 0;

    int count = 0;

    // 统计中文字符（CJK 统一表意文字范围）
    // Unicode 范围：\u4e00 - \u9fff（常用汉字）
    // 扩展区也可以加上，但常用区足够
    QRegularExpression cjkRegex(QStringLiteral("[\\u4e00-\\u9fff]"));
    auto cjkIt = cjkRegex.globalMatch(plainText);
    while (cjkIt.hasNext()) {
        cjkIt.next();
        ++count;
    }

    // 统计英文单词（连续的字母数字序列）
    QRegularExpression wordRegex(QStringLiteral("[a-zA-Z0-9]+"));
    auto wordIt = wordRegex.globalMatch(plainText);
    while (wordIt.hasNext()) {
        wordIt.next();
        ++count;
    }

    return count;
}

// ===========================================================================
// 状态转换
// ===========================================================================

QString Report::statusDisplayName() const
{
    switch (m_status) {
    case ReportStatus::Draft:     return QStringLiteral("草稿");
    case ReportStatus::Submitted: return QStringLiteral("已提交");
    case ReportStatus::Reviewed:  return QStringLiteral("已审核");
    }
    return QStringLiteral("未知");
}

QString Report::statusToString() const
{
    switch (m_status) {
    case ReportStatus::Draft:     return AppConstants::REPORT_STATUS_DRAFT;
    case ReportStatus::Submitted: return AppConstants::REPORT_STATUS_SUBMITTED;
    case ReportStatus::Reviewed:  return AppConstants::REPORT_STATUS_REVIEWED;
    }
    return AppConstants::REPORT_STATUS_DRAFT;
}

ReportStatus Report::statusFromString(const QString& str)
{
    if (str == AppConstants::REPORT_STATUS_SUBMITTED) {
        return ReportStatus::Submitted;
    }
    if (str == AppConstants::REPORT_STATUS_REVIEWED) {
        return ReportStatus::Reviewed;
    }
    return ReportStatus::Draft;
}

QString Report::toString() const
{
    return QString("Report(id=%1, title='%2', project=%3, status=%4, blocks=%5)")
        .arg(m_id)
        .arg(m_title)
        .arg(m_projectId)
        .arg(statusToString())
        .arg(m_blocks.size());
}

QString Report::generateBlockId()
{
    // 生成不带花括号的 UUID
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
