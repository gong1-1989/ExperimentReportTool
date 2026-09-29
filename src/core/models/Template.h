/**
 * @file Template.h
 * @brief 报告模板实体类头文件
 *
 * Template 定义了报告的结构骨架，包含一系列预定义的内容块。
 * 用户基于模板创建报告时，模板中的块会被复制到新报告中。
 * 模板支持导入导出（JSON 文件），方便共享。
 */

#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <QString>
#include <QDateTime>
#include <QList>
#include <QSharedPointer>
#include <QJsonObject>

#include "Report.h"  // 复用 ContentBlock 定义

/**
 * @brief 报告模板实体类
 *
 * 对应数据库中的 templates 表。
 * 模板结构（structure）以 JSON 对象存储：{ "version": 2, "document": "<html>..." }
 * 模板通常只含文本骨架（标题/段落），不含结构化对象。
 *
 * 内置模板（is_builtin = true）不可删除，只能复制后修改。
 */
class Template
{
public:
    using Ptr = QSharedPointer<Template>;
    using List = QList<Ptr>;

    /// 工厂方法
    static Ptr create();

    Template();
    ~Template();

    // -----------------------------------------------------------------------
    // 属性访问器
    // -----------------------------------------------------------------------

    qint64 id() const { return m_id; }
    void setId(qint64 id) { m_id = id; }

    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    QString category() const { return m_category; }
    void setCategory(const QString& category) { m_category = category; }

    QString description() const { return m_description; }
    void setDescription(const QString& desc) { m_description = desc; }

    bool isBuiltin() const { return m_isBuiltin; }
    void setBuiltin(bool builtin) { m_isBuiltin = builtin; }

    QDateTime createdAt() const { return m_createdAt; }
    void setCreatedAt(const QDateTime& dt) { m_createdAt = dt; }

    QDateTime updatedAt() const { return m_updatedAt; }
    void setUpdatedAt(const QDateTime& dt) { m_updatedAt = dt; }

    // -----------------------------------------------------------------------
    // 模板内容（连续文档 HTML）
    // -----------------------------------------------------------------------

    /// 获取模板文档 HTML
    const QString& document() const { return m_document; }
    /// 设置模板文档 HTML
    void setDocument(const QString& html) { m_document = html; }

    /// 模板内嵌结构化对象（表格/图表/图片/公式/代码，与 document 中的锚点对应）
    const QList<ContentBlock>& objects() const { return m_objects; }
    void setObjects(const QList<ContentBlock>& objects) { m_objects = objects; }
    int objectCount() const { return m_objects.size(); }

    // -----------------------------------------------------------------------
    // 序列化
    // -----------------------------------------------------------------------

    /// 结构序列化为 JSON 字符串（数据库存储）
    QString structureToJson() const;
    /// 从 JSON 字符串解析结构
    void structureFromJson(const QString& json);

    /**
     * @brief 导出模板到文件（JSON 格式）
     * @param filePath 文件路径
     * @return 成功返回 true
     */
    bool exportToFile(const QString& filePath) const;

    /**
     * @brief 从文件导入模板
     * @param filePath 文件路径
     * @return 导入的模板指针，失败返回 nullptr
     */
    static Ptr importFromFile(const QString& filePath);

    /// 是否已保存
    bool isPersisted() const { return m_id > 0; }

    /// 调试字符串
    QString toString() const;

private:
    qint64  m_id;           ///< 主键
    QString m_name;         ///< 模板名称
    QString m_category;     ///< 分类（物理/化学/生物/通用等）
    QString m_description;  ///< 模板描述
    bool    m_isBuiltin;    ///< 是否内置模板
    QDateTime m_createdAt;  ///< 创建时间
    QDateTime m_updatedAt;  ///< 更新时间

    QString m_document;          ///< 模板文档 HTML
    QList<ContentBlock> m_objects;  ///< 模板内嵌结构化对象（与 document 锚点对应）
};

#endif // TEMPLATE_H
