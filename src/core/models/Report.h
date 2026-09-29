/**
 * @file Report.h
 * @brief 实验报告实体类头文件
 *
 * Report 表示一份实验报告，是系统的核心实体。
 * 报告内容以结构化 JSON 存储，支持富文本、表格、图片、图表等多种块类型。
 */

#ifndef REPORT_H
#define REPORT_H

#include <QString>
#include <QDateTime>
#include <QList>
#include <QSharedPointer>
#include <QJsonObject>
#include <QJsonArray>

/**
 * @brief 报告状态枚举
 */
enum class ReportStatus {
    Draft,      ///< 草稿（编辑中）
    Submitted,  ///< 已提交（等待审核）
    Reviewed    ///< 已审核（完成）
};

/**
 * @brief 内容块类型枚举
 *
 * 报告内容为「连续文档 + 结构化对象锚点」：
 * 正文以 HTML 连续文档保存，表格/图片/图表等以对象列表（objects）保存，
 * 文档中通过 object://<type>/<id> 锚点引用对象。
 */
enum class BlockType {
    Table,        ///< 表格
    Image,        ///< 图片
    Chart,        ///< 图表（关联数据表）
    Formula,      ///< 数学公式（LaTeX）
    Divider,      ///< 分割线
    DataReference ///< 数据引用（关联数据表）
};

/**
 * @brief 结构化对象结构体（表格/图片/图表/公式/分割线/数据引用）
 *
 * 每个对象包含：
 * - id: 唯一标识（UUID）
 * - type: 对象类型
 * - data: 对象数据（JSON 对象，结构因类型而异）
 *
 * 例如：
 * - Image:     { "path": "attachments/xxx.png", "caption": "图1", "width": 800 }
 * - Table:     { "tableId": 123 } （关联到 data_tables 表）
 */
struct ContentBlock {
    QString     id;     ///< 块唯一标识（UUID）
    BlockType   type;   ///< 块类型
    QJsonObject data;   ///< 块数据（JSON 对象）

    /// 默认构造
    ContentBlock() : type(BlockType::DataReference) {}

    /// 带类型的构造
    explicit ContentBlock(BlockType t) : type(t) {}

    /**
     * @brief 序列化为 JSON 对象
     * @return JSON 对象
     */
    QJsonObject toJson() const;

    /**
     * @brief 从 JSON 对象反序列化
     * @param json JSON 对象
     * @return ContentBlock 实例
     */
    static ContentBlock fromJson(const QJsonObject& json);

    /**
     * @brief 块类型转换为字符串
     * @param type 块类型
     * @return 类型字符串
     */
    static QString blockTypeToString(BlockType type);

    /**
     * @brief 从字符串解析块类型
     * @param str 类型字符串
     * @return 块类型
     */
    static BlockType blockTypeFromString(const QString& str);
};

/**
 * @brief 实验报告实体类
 *
 * 对应数据库中的 reports 表。
 * 报告内容（content）以 JSON 对象存储：
 *   {
 *     "version": 2,
 *     "document": "<html>...",          // 连续文本（QTextDocument HTML，含 <img src="object://type/id"> 锚点）
 *     "objects":  [ {id,type,data}, ... ] // 结构化对象（表格/图表/图片/公式/代码/分割线/数据引用）
 *   }
 *
 *
 * 使用示例：
 * @code
 *   auto report = Report::create();
 *   report->setTitle("牛顿第二定律验证实验报告");
 *   report->setProjectId(1);
 *   report->setDocument("<p>实验目的</p><p>验证牛顿第二定律...</p>");
 *   report->addObject(ContentBlock(BlockType::Table));
 * @endcode
 */
class Report
{
public:
    /// 智能指针类型别名
    using Ptr = QSharedPointer<Report>;
    using List = QList<Ptr>;

    /**
     * @brief 创建新报告（工厂方法）
     * @return 指向新报告的智能指针
     */
    static Ptr create();

    // -----------------------------------------------------------------------
    // 构造与析构
    // -----------------------------------------------------------------------

    Report();
    ~Report();

    // -----------------------------------------------------------------------
    // 基本属性访问器
    // -----------------------------------------------------------------------

    qint64 id() const { return m_id; }
    void setId(qint64 id) { m_id = id; }

    qint64 projectId() const { return m_projectId; }
    void setProjectId(qint64 id) { m_projectId = id; }

    qint64 templateId() const { return m_templateId; }
    void setTemplateId(qint64 id) { m_templateId = id; }

    QString title() const { return m_title; }
    void setTitle(const QString& title) { m_title = title; }

    ReportStatus status() const { return m_status; }
    void setStatus(ReportStatus status) { m_status = status; }

    QString author() const { return m_author; }
    void setAuthor(const QString& author) { m_author = author; }

    qint64 createdBy() const { return m_createdBy; }
    void setCreatedBy(qint64 userId) { m_createdBy = userId; }

    /// 最后修改者用户 ID
    qint64 modifiedBy() const { return m_modifiedBy; }
    void setModifiedBy(qint64 userId) { m_modifiedBy = userId; }

    int version() const { return m_version; }
    void setVersion(int version) { m_version = version; }

    QDate experimentDate() const { return m_experimentDate; }
    void setExperimentDate(const QDate& date) { m_experimentDate = date; }

    QDateTime createdAt() const { return m_createdAt; }
    void setCreatedAt(const QDateTime& dt) { m_createdAt = dt; }

    QDateTime updatedAt() const { return m_updatedAt; }
    void setUpdatedAt(const QDateTime& dt) { m_updatedAt = dt; }

    // -----------------------------------------------------------------------
    // 内容操作（连续文档 + 对象）
    // -----------------------------------------------------------------------

    /**
     * @brief 获取连续文档 HTML（QTextDocument 导出格式，含 object:// 锚点图片）
     * @return 文档 HTML
     */
    const QString& document() const { return m_document; }

    /**
     * @brief 设置连续文档 HTML
     * @param html 文档 HTML
     */
    void setDocument(const QString& html) { m_document = html; }

    /**
     * @brief 获取结构化对象列表（只读引用）
     * @return 对象列表
     */
    const QList<ContentBlock>& objects() const { return m_objects; }

    /**
     * @brief 设置结构化对象列表
     * @param objects 对象列表
     */
    void setObjects(const QList<ContentBlock>& objects) { m_objects = objects; }

    /**
     * @brief 获取对象数量
     * @return 对象数量
     */
    int objectCount() const { return m_objects.size(); }

    /**
     * @brief 获取指定 ID 的对象
     * @param objectId 对象 ID
     * @return 对象（找不到返回空块，需检查 id 是否为空）
     */
    ContentBlock objectById(const QString& objectId) const;

    /**
     * @brief 追加一个对象到列表末尾
     * @param object 对象
     */
    void addObject(const ContentBlock& object);

    /**
     * @brief 按 ID 更新对象数据
     * @param objectId 对象 ID
     * @param data 新对象数据
     * @return 是否找到并更新
     */
    bool updateObject(const QString& objectId, const QJsonObject& data);

    /**
     * @brief 按 ID 删除对象
     * @param objectId 对象 ID
     * @return 是否找到并删除
     */
    bool removeObject(const QString& objectId);

    // -----------------------------------------------------------------------
    // 内容序列化（与数据库 JSON 字段交互）
    // -----------------------------------------------------------------------

    /**
     * @brief 将内容块序列化为 JSON 字符串（用于数据库存储）
     * @return JSON 字符串
     */
    QString contentToJson() const;

    /**
     * @brief 从 JSON 字符串解析内容块（从数据库读取后调用）
     * @param json JSON 字符串
     */
    void contentFromJson(const QString& json);

    /**
     * @brief 提取报告的纯文本内容（用于全文检索）
     * @return 纯文本字符串
     *
     * 遍历所有块，提取其中的文本内容，拼接成纯文本。
     * 用于 SQLite FTS5 全文索引。
     */
    QString toPlainText() const;

    /**
     * @brief 获取报告字数
     *
     * 优先返回保存时统计的字数值；如果未设置（旧数据），则从内容实时计算。
     *
     * @return 字数
     */
    int wordCount() const;

    /**
     * @brief 设置报告字数（保存时由编辑器统计后写入）
     * @param count 字数
     */
    void setWordCount(int count) { m_wordCount = count; }

    // -----------------------------------------------------------------------
    // 状态转换与工具方法
    // -----------------------------------------------------------------------

    /// 状态显示名称
    QString statusDisplayName() const;
    /// 状态转字符串
    QString statusToString() const;
    /// 从字符串解析状态
    static ReportStatus statusFromString(const QString& str);

    /// 是否已保存到数据库
    bool isPersisted() const { return m_id > 0; }

    /// 调试用字符串表示
    QString toString() const;

    /**
     * @brief 生成新的对象 ID（UUID，形如 obj-xxxx）
     * @return UUID 字符串
     */
    static QString generateObjectId();

private:
    // -----------------------------------------------------------------------
    // 数据成员
    // -----------------------------------------------------------------------

    qint64              m_id;              ///< 主键 ID
    qint64              m_projectId;       ///< 所属项目 ID
    qint64              m_templateId;      ///< 使用的模板 ID
    QString             m_title;           ///< 报告标题
    ReportStatus        m_status;          ///< 报告状态
    QString             m_author;          ///< 作者
    qint64              m_createdBy = -1;  ///< 创建者用户 ID
    qint64              m_modifiedBy = -1; ///< 最后修改者用户 ID
    int                 m_version = 1;     ///< 版本号（乐观锁）
    int                 m_wordCount = -1;  ///< 字数（保存时统计，-1表示未设置需实时计算）
    QDate               m_experimentDate;  ///< 实验日期
    QDateTime           m_createdAt;       ///< 创建时间
    QDateTime           m_updatedAt;       ///< 最后更新时间

    QString              m_document;        ///< 连续文档 HTML（含 object:// 锚点图片）
    QList<ContentBlock>  m_objects;         ///< 结构化对象列表（表格/图表/图片/公式/分割线/数据引用）
};

#endif // REPORT_H
