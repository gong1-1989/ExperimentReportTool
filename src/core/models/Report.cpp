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
#include <QTextDocument>

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
        block.id = Report::generateObjectId();
    }

    return block;
}

QString ContentBlock::blockTypeToString(BlockType type)
{
    switch (type) {
    case BlockType::Table:         return "table";
    case BlockType::Image:         return "image";
    case BlockType::Chart:         return "chart";
    case BlockType::Formula:       return "formula";
    case BlockType::Divider:       return "divider";
    case BlockType::DataReference: return "data_reference";
    case BlockType::AttachmentCard: return "attachment_card";
    case BlockType::MediaRef:       return "media_ref";
    }
    return "data_reference";  // 默认
}

BlockType ContentBlock::blockTypeFromString(const QString& str)
{
    if (str == "table")         return BlockType::Table;
    if (str == "image")         return BlockType::Image;
    if (str == "chart")         return BlockType::Chart;
    if (str == "formula")       return BlockType::Formula;
    if (str == "divider")       return BlockType::Divider;
    if (str == "data_reference")return BlockType::DataReference;
    if (str == "attachment_card") return BlockType::AttachmentCard;
    if (str == "media_ref")       return BlockType::MediaRef;
    return BlockType::DataReference;  // 默认对象
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

ContentBlock Report::objectById(const QString& objectId) const
{
    for (const ContentBlock& object : m_objects) {
        if (object.id == objectId) return object;
    }
    return ContentBlock();
}

void Report::addObject(const ContentBlock& object)
{
    ContentBlock obj = object;
    // 确保对象有 ID
    if (obj.id.isEmpty()) {
        obj.id = generateObjectId();
    }
    m_objects.append(obj);
    m_updatedAt = QDateTime::currentDateTime();
}

bool Report::updateObject(const QString& objectId, const QJsonObject& data)
{
    for (ContentBlock& object : m_objects) {
        if (object.id == objectId) {
            object.data = data;
            m_updatedAt = QDateTime::currentDateTime();
            return true;
        }
    }
    return false;
}

bool Report::removeObject(const QString& objectId)
{
    for (int i = 0; i < m_objects.size(); ++i) {
        if (m_objects.at(i).id == objectId) {
            m_objects.removeAt(i);
            m_updatedAt = QDateTime::currentDateTime();
            return true;
        }
    }
    return false;
}

// ===========================================================================
// 内容序列化
// ===========================================================================

QString Report::contentToJson() const
{
    QJsonObject root;
    root["version"] = 2;
    root["document"] = m_document;

    QJsonArray objectArray;
    for (const ContentBlock& object : m_objects) {
        objectArray.append(object.toJson());
    }
    root["objects"] = objectArray;

    QJsonDocument doc(root);
    // 紧凑格式（不缩进），节省存储空间
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

void Report::contentFromJson(const QString& json)
{
    m_document.clear();
    m_objects.clear();

    if (json.isEmpty()) return;

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        // JSON 解析失败，记录错误但不抛出异常
        // 调用方可以通过日志系统记录
        return;
    }

    // 仅支持新版格式：顶层对象 { version, document, objects }
    // （旧版块数组格式不再迁移，直接按空内容处理）
    if (!doc.isObject()) return;

    const QJsonObject root = doc.object();
    m_document = root.value("document").toString();

    const QJsonArray objectArray = root.value("objects").toArray();
    for (const QJsonValue& value : objectArray) {
        if (value.isObject()) {
            m_objects.append(ContentBlock::fromJson(value.toObject()));
        }
    }
}

QString Report::toPlainText() const
{
    QStringList texts;

    // 连续文档：解析 HTML 得纯文本
    if (!m_document.isEmpty()) {
        QTextDocument doc;
        doc.setHtml(m_document);
        const QString plain = doc.toPlainText().trimmed();
        if (!plain.isEmpty()) texts.append(plain);
    }

    // 结构化对象：提取其中的文本内容
    for (const ContentBlock& object : m_objects) {
        switch (object.type) {
        case BlockType::Formula:
            texts.append(object.data.value("latex").toString());
            break;
        case BlockType::Image:
            texts.append(object.data.value("caption").toString());
            break;
        case BlockType::Table:
        case BlockType::Chart:
        case BlockType::DataReference:
            // 表格/图表/数据引用以数据表为主，不计入文本检索
            break;
        case BlockType::AttachmentCard:
            // 附件卡片：文件名 + 题注参与检索
            texts.append(object.data.value("fileName").toString());
            texts.append(object.data.value("caption").toString());
            break;
        case BlockType::MediaRef:
            // 音视频引用：标题参与检索
            texts.append(object.data.value("title").toString());
            break;
        default:
            break;
        }
    }

    return texts.join(" ");
}

int Report::wordCount() const
{
    // 优先返回保存时统计的字数值
    if (m_wordCount >= 0) {
        return m_wordCount;
    }

    // 旧数据兼容：从内容实时计算
    const QString plainText = toPlainText();
    if (plainText.isEmpty()) return 0;

    int count = 0;

    // 统计中文字符（CJK 统一表意文字范围）
    QRegularExpression cjkRegex(QStringLiteral("[\u4e00-\u9fff]"));
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
    return statusDisplayName(m_status);
}

QString Report::statusDisplayName(ReportStatus status)
{
    switch (status) {
    case ReportStatus::Draft:     return QStringLiteral("草稿");
    case ReportStatus::Submitted: return QStringLiteral("已提交");
    case ReportStatus::Reviewed:  return QStringLiteral("已审核");
    case ReportStatus::Approved:  return QStringLiteral("已审批");
    case ReportStatus::Archived:  return QStringLiteral("已归档");
    }
    return QStringLiteral("未知");
}

QString Report::statusToString() const
{
    switch (m_status) {
    case ReportStatus::Draft:     return AppConstants::REPORT_STATUS_DRAFT;
    case ReportStatus::Submitted: return AppConstants::REPORT_STATUS_SUBMITTED;
    case ReportStatus::Reviewed:  return AppConstants::REPORT_STATUS_REVIEWED;
    case ReportStatus::Approved:  return AppConstants::REPORT_STATUS_APPROVED;
    case ReportStatus::Archived:  return AppConstants::REPORT_STATUS_ARCHIVED;
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
    if (str == AppConstants::REPORT_STATUS_APPROVED) {
        return ReportStatus::Approved;
    }
    if (str == AppConstants::REPORT_STATUS_ARCHIVED) {
        return ReportStatus::Archived;
    }
    return ReportStatus::Draft;
}

QString Report::toString() const
{
    return QString("Report(id=%1, title='%2', project=%3, status=%4, objects=%5)")
        .arg(m_id)
        .arg(m_title)
        .arg(m_projectId)
        .arg(statusToString())
        .arg(m_objects.size());
}

QString Report::generateObjectId()
{
    // 生成不带花括号的 UUID
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
