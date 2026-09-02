/**
 * @file Template.cpp
 * @brief 报告模板实体类实现文件
 */

#include "Template.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>

// ===========================================================================
// 工厂方法与构造
// ===========================================================================

Template::Ptr Template::create()
{
    return Ptr(new Template());
}

Template::Template()
    : m_id(-1)
    , m_isBuiltin(false)
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
{
}

Template::~Template()
{
}

// ===========================================================================
// 序列化
// ===========================================================================

QString Template::structureToJson() const
{
    QJsonArray array;
    for (const ContentBlock& block : m_blocks) {
        array.append(block.toJson());
    }
    QJsonDocument doc(array);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

void Template::structureFromJson(const QString& json)
{
    m_blocks.clear();
    if (json.isEmpty()) return;

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isArray()) return;

    for (const QJsonValue& value : doc.array()) {
        if (value.isObject()) {
            m_blocks.append(ContentBlock::fromJson(value.toObject()));
        }
    }
}

// ===========================================================================
// 文件导入导出
// ===========================================================================

bool Template::exportToFile(const QString& filePath) const
{
    // 确保目录存在
    QDir().mkpath(QFileInfo(filePath).absolutePath());

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    // 构建完整的模板 JSON 对象（包含元信息 + 结构）
    QJsonObject root;
    root["format_version"] = 1;
    root["name"] = m_name;
    root["category"] = m_category;
    root["description"] = m_description;
    root["exported_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonArray blocksArray;
    for (const ContentBlock& block : m_blocks) {
        blocksArray.append(block.toJson());
    }
    root["structure"] = blocksArray;

    QJsonDocument doc(root);
    // 带缩进的格式，方便人工阅读
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

Template::Ptr Template::importFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return nullptr;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return nullptr;
    }

    const QJsonObject root = doc.object();

    // 格式版本检查（当前只支持版本 1）
    const int formatVersion = root.value("format_version").toInt(0);
    if (formatVersion != 1) {
        return nullptr;
    }

    Ptr temp = create();
    temp->setName(root.value("name").toString());
    temp->setCategory(root.value("category").toString());
    temp->setDescription(root.value("description").toString());

    // 解析结构
    QList<ContentBlock> blocks;
    const QJsonArray blocksArray = root.value("structure").toArray();
    for (const QJsonValue& value : blocksArray) {
        if (value.isObject()) {
            blocks.append(ContentBlock::fromJson(value.toObject()));
        }
    }
    temp->setBlocks(blocks);

    return temp;
}

QString Template::toString() const
{
    return QString("Template(id=%1, name='%2', category='%3', builtin=%4, blocks=%5)")
        .arg(m_id)
        .arg(m_name)
        .arg(m_category)
        .arg(m_isBuiltin)
        .arg(m_blocks.size());
}
