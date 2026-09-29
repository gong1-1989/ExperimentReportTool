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
    QJsonObject root;
    root["version"] = 2;
    root["document"] = m_document;
    QJsonArray objectArray;
    for (const ContentBlock& object : m_objects) {
        objectArray.append(object.toJson());
    }
    root["objects"] = objectArray;
    QJsonDocument doc(root);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

void Template::structureFromJson(const QString& json)
{
    m_document.clear();
    if (json.isEmpty()) return;

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) return;

    m_objects.clear();

    // 仅支持新版格式：顶层对象 { version, document, objects }
    // （旧版块数组格式不再迁移，直接按空内容处理）
    if (doc.isObject()) {
        const QJsonObject root = doc.object();
        m_document = root.value("document").toString();
        const QJsonArray objectArray = root.value("objects").toArray();
        for (const QJsonValue& value : objectArray) {
            if (value.isObject()) {
                m_objects.append(ContentBlock::fromJson(value.toObject()));
            }
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

    root["structure"] = structureToJson();

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

    // 解析结构：新版对象 {version, document, objects}
    const QJsonValue structure = root.value("structure");
    if (structure.isObject()) {
        temp->structureFromJson(QString::fromUtf8(
            QJsonDocument(structure.toObject()).toJson(QJsonDocument::Compact)));
    } else if (structure.isArray()) {
        temp->structureFromJson(QString::fromUtf8(
            QJsonDocument(structure.toArray()).toJson(QJsonDocument::Compact)));
    }

    return temp;
}

QString Template::toString() const
{
    return QString("Template(id=%1, name='%2', category='%3', builtin=%4, document_len=%5)")
        .arg(m_id)
        .arg(m_name)
        .arg(m_category)
        .arg(m_isBuiltin)
        .arg(m_document.size());
}
