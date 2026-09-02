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
 * 报告内容由多个"块"组成，每个块有不同的类型和数据结构。
 * 这种设计类似 Notion / 语雀的块级编辑器。
 */
enum class BlockType {
    Heading1,     ///< 一级标题
    Heading2,     ///< 二级标题
    Heading3,     ///< 三级标题
    Paragraph,    ///< 普通段落
    BulletList,   ///< 无序列表
    NumberedList, ///< 有序列表
    Table,        ///< 表格
    Image,        ///< 图片
    Chart,        ///< 图表（关联数据表）
    CodeBlock,    ///< 代码块
    Formula,      ///< 数学公式（LaTeX）
    Quote,        ///< 引用块
    Divider,      ///< 分割线
    DataReference ///< 数据引用（关联数据表）
};

/**
 * @brief 内容块结构体
 *
 * 每个块包含：
 * - id: 唯一标识（UUID）
 * - type: 块类型
 * - data: 块数据（JSON 对象，结构因类型而异）
 *
 * 例如：
 * - Paragraph: { "text": "段落内容", "alignment": "left" }
 * - Heading1:  { "text": "标题文字" }
 * - Image:     { "path": "attachments/xxx.png", "caption": "图1", "width": 800 }
 * - Table:     { "tableId": 123 } （关联到 data_tables 表）
 */
struct ContentBlock {
    QString     id;     ///< 块唯一标识（UUID）
    BlockType   type;   ///< 块类型
    QJsonObject data;   ///< 块数据（JSON 对象）

    /// 默认构造
    ContentBlock() : type(BlockType::Paragraph) {}

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
 * 报告内容（content）以 JSON 数组形式存储，每个元素是一个 ContentBlock。
 *
 * 使用示例：
 * @code
 *   auto report = Report::create();
 *   report->setTitle("牛顿第二定律验证实验报告");
 *   report->setProjectId(1);
 *   report->setTemplateId(1);
 *   report->appendBlock(ContentBlock(BlockType::Heading1, "实验目的"));
 *   report->appendBlock(ContentBlock(BlockType::Paragraph, "验证牛顿第二定律..."));
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

    QDate experimentDate() const { return m_experimentDate; }
    void setExperimentDate(const QDate& date) { m_experimentDate = date; }

    QDateTime createdAt() const { return m_createdAt; }
    void setCreatedAt(const QDateTime& dt) { m_createdAt = dt; }

    QDateTime updatedAt() const { return m_updatedAt; }
    void setUpdatedAt(const QDateTime& dt) { m_updatedAt = dt; }

    // -----------------------------------------------------------------------
    // 内容块操作
    // -----------------------------------------------------------------------

    /**
     * @brief 获取所有内容块（只读引用）
     * @return 内容块列表
     */
    const QList<ContentBlock>& blocks() const { return m_blocks; }

    /**
     * @brief 获取内容块数量
     * @return 块数量
     */
    int blockCount() const { return m_blocks.size(); }

    /**
     * @brief 获取指定索引的块
     * @param index 块索引
     * @return 块引用（越界返回空块的引用，需检查）
     */
    const ContentBlock& blockAt(int index) const;

    /**
     * @brief 追加一个块到末尾
     * @param block 要追加的块
     */
    void appendBlock(const ContentBlock& block);

    /**
     * @brief 在指定位置插入块
     * @param index 插入位置（0 表示开头）
     * @param block 要插入的块
     */
    void insertBlock(int index, const ContentBlock& block);

    /**
     * @brief 替换指定位置的块
     * @param index 块索引
     * @param block 新块
     */
    void replaceBlock(int index, const ContentBlock& block);

    /**
     * @brief 删除指定位置的块
     * @param index 块索引
     */
    void removeBlock(int index);

    /**
     * @brief 移动块的位置
     * @param from 源索引
     * @param to 目标索引
     */
    void moveBlock(int from, int to);

    /**
     * @brief 清空所有内容块
     */
    void clearBlocks();

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
     * @brief 统计报告字数（中文字符 + 英文单词）
     * @return 字数
     */
    int wordCount() const;

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
     * @brief 生成新的块 ID（UUID）
     * @return UUID 字符串
     */
    static QString generateBlockId();

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
    QDate               m_experimentDate;  ///< 实验日期
    QDateTime           m_createdAt;       ///< 创建时间
    QDateTime           m_updatedAt;       ///< 最后更新时间

    QList<ContentBlock> m_blocks;          ///< 内容块列表（报告正文）
};

#endif // REPORT_H
