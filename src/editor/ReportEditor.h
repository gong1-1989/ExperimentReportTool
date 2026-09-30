/**
 * @file ReportEditor.h
 * @brief 报告编辑器主组件头文件
 *
 * ReportEditor 是报告编辑的核心组件，采用「连续文档 + 结构化对象」模型：
 * - 标题/段落/列表/引用等文本内容在一个连续 QTextEdit 中编辑（Word 式输入体验）
 * - 表格/图表/图片/公式/代码等结构化内容以「对象锚点」形式内嵌在文档中，
 *   由用户通过工具栏手动插入，点击锚点可打开编辑对话框
 *
 * 布局：
 * - 顶部：报告标题编辑栏 + 元信息（日期、状态）
 * - 中部：连续文档编辑区（DocumentTextEdit）
 * - 底部：字数统计 + 保存状态
 */

#ifndef REPORT_EDITOR_H
#define REPORT_EDITOR_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QHash>
#include <QPixmap>
#include <QTimer>
#include <QColor>
#include <QEvent>

#include "core/models/Report.h"

// 前向声明 UI 类（由 uic 工具从 .ui 文件自动生成）
namespace Ui {
class ReportEditor;
}

// 前向声明
class AutoSaveManager;
class DocumentTextEdit;

/**
 * @brief 报告编辑器主组件
 */
class ReportEditor : public QWidget
{
    Q_OBJECT

public:
    explicit ReportEditor(QWidget* parent = nullptr);
    ~ReportEditor() override;

    // -----------------------------------------------------------------------
    // 报告加载与保存
    // -----------------------------------------------------------------------

    /**
     * @brief 加载报告到编辑器
     * @param report 报告对象
     */
    void loadReport(const Report::Ptr& report);

    /**
     * @brief 保存编辑器内容到报告对象
     * @return 报告对象
     */
    Report::Ptr saveToReport();

    /**
     * @brief 获取当前编辑的报告 ID
     * @return 报告 ID，新建报告返回 -1
     */
    qint64 reportId() const { return m_report ? m_report->id() : -1; }

    /**
     * @brief 获取报告标题
     * @return 标题
     *
     * @note 此函数在 .cpp 中实现，因为需要访问 ui 对象的完整定义
     */
    QString reportTitle() const;

    // -----------------------------------------------------------------------
    // 文档与对象操作
    // -----------------------------------------------------------------------

    /// 获取连续文档编辑控件（供格式工具栏等直接操作）
    DocumentTextEdit* textEdit() const;

    /// 获取对象数量
    int objectCount() const { return m_objects.size(); }

    /**
     * @brief 在光标处插入结构化对象
     * @param type 对象类型（Table/Chart/Image/Formula/Divider/DataReference）
     * @param data 对象数据（如 {tableId, caption} / {latex} / {code}）
     * @return 新对象 ID（插入失败返回空串）
     */
    QString insertObject(BlockType type, const QJsonObject& data);

    /**
     * @brief 删除指定对象（同时从文档移除其锚点）
     * @param objectId 对象 ID
     */
    void removeObjectById(const QString& objectId);

    /**
     * @brief 更新对象数据（编辑对话框保存后调用，刷新预览）
     * @param objectId 对象 ID
     * @param data 新对象数据
     */
    void updateObjectData(const QString& objectId, const QJsonObject& data);

    /**
     * @brief 在光标处插入分割线（文本级 <hr/>）
     */
    void insertDividerAtCursor();

    /// 获取指定 ID 的对象
    ContentBlock objectById(const QString& objectId) const;

    /// 获取对象预览图（DocumentTextEdit::loadResource 调用）
    /// 获取对象预览图（缓存缺失时自动重建，供文档资源提供者调用）
    QPixmap objectPreviewPixmap(const QString& imageName);

    /// 将对象预览图预注册到文档资源表（渲染兜底，与资源提供者双保险）
    void registerObjectResource(const ContentBlock& object);

    /**
     * @brief 锚点对象自愈
     *
     * 解析文档中所有对象锚点（object://type/id），对 objects 中缺失的 id
     * 按锚点类型补齐占位对象，使旧版本创建的报告（对象数据丢失）恢复可编辑状态。
     */
    void healMissingObjects();

    /// 表格自愈：Table 对象无数据表引用时自动创建并回填 tableId
    void healTableObjects();

    /// 填充标签下拉框（所有标签 + 当前报告标签选中）
    void populateTags();

    /// 保存当前选中的标签到报告（报告已入库后调用）
    void saveReportTags();

    // -----------------------------------------------------------------------
    // 查找辅助
    // -----------------------------------------------------------------------

    /// 获取文档纯文本（按段落切分，供查找对话框）
    QStringList documentParagraphs() const;

    /**
     * @brief 滚动到指定段落并聚焦（供查找跳转）
     * @param paragraphIndex 段落索引
     */
    void scrollToParagraph(int paragraphIndex);

    // -----------------------------------------------------------------------
    // 编辑操作
    // -----------------------------------------------------------------------

    /// 撤销（工具栏/菜单入口）
    void undo();

    /// 重做（工具栏/菜单入口）
    void redo();

    /// 是否有可撤销的历史
    bool canUndo() const;

    /// 是否有可重做的历史
    bool canRedo() const;

    /// 设置焦点到文档编辑区
    void focusDocument();

    /// 设置当前段落对齐方式
    void applyParagraphAlignment(Qt::Alignment align);

    /// 设置当前段落行高倍数（如 1.5 = 1.5 倍行高）
    void applyLineHeight(qreal multiplier);

    // -----------------------------------------------------------------------
    // 格式操作（Word 式块级应用：无选区时作用于整个段落）
    // -----------------------------------------------------------------------

    /// 加粗（bold = 是否加粗）
    void setBold(bool bold);
    /// 斜体
    void setItalic(bool italic);
    /// 下划线
    void setUnderline(bool underline);
    /// 设置字号（绝对磅值；同时清除相对字号调整）
    void setFontSize(int pointSize);
    /// 设置文字颜色
    void setTextColor(const QColor& color);
    /// 创建/切换列表（numbered = 有序列表，false 为无序）
    void setList(bool numbered);
    /// 切换引用缩进（0 ↔ 1）
    void setQuote();
    /// 应用字符格式：有选区作用于选中文字，无选区作用于整个段落
    void applyCharFormat(const QTextCharFormat& fmt);

    /// 读取当前光标格式并发出 formattingStateChanged（光标移动/内容变化后调用）
public:
    void refreshFormattingState();   // 工具栏状态刷新（公开：应用槽可直接调用，保证回显即时）

    /// 设置只读模式
    void setReadOnly(bool readOnly);

    /// 是否有未保存的更改
    bool isModified() const { return m_modified; }

    /// 设置修改状态
    void setModified(bool modified);

    /// 获取字数统计
    int wordCount() const;

signals:
    /// 报告内容变化（用于自动保存）
    void contentChanged();

    /// 报告标题变化
    void titleChanged(const QString& title);

    /// 保存状态变化
    void saveStateChanged(bool saved);

    /// 请求保存
    void saveRequested();

    /// 对象数量变化
    void objectCountChanged(int count);

    /// 撤销/重做可用状态变化（用于按钮置灰）
    void undoAvailableChanged(bool available);
    void redoAvailableChanged(bool available);

    /// 文本格式化状态变化（字号/粗体/斜体/下划线/行列）
        /// 格式化状态变化（字号、加粗、对齐、行高、标题、颜色等，用于工具栏状态同步）
    void formattingStateChanged(int fontSize, bool bold, bool italic, bool underline,
                                int blockNumber, int positionInBlock,
                                Qt::Alignment alignment, qreal lineHeightMultiplier,
                                int headingLevel, const QColor& textColor);

    /// 点击对象锚点（由 ReportEditorWindow 连接，打开编辑对话框）
    void objectDoubleClicked(const QString& objectId);

private slots:
    // -----------------------------------------------------------------------
    // 文档信号处理
    // -----------------------------------------------------------------------
    void onTextChanged();           ///< 文档内容变化（标记修改、字数、自动保存）
    void onCursorPositionChanged(); ///< 光标移动（同步格式状态）
    void onUndoAvailableChanged(bool available);
    void onRedoAvailableChanged(bool available);
    void onDocumentObjectClicked(const QString& objectId);

    // -----------------------------------------------------------------------
    // 标题栏信号
    // -----------------------------------------------------------------------
    void onTitleChanged(const QString& title);
    void onDateChanged(const QDate& date);
    void on_m_tagCombo_currentIndexChanged(int index);

    // -----------------------------------------------------------------------
    // 工具栏
    // -----------------------------------------------------------------------

private:
    // -----------------------------------------------------------------------
    // 内部方法
    // -----------------------------------------------------------------------

    /// 将报告内容载入编辑控件
    void loadDocument();

    /// 收集编辑控件内容到报告（含孤儿对象清理）
    void collectDocument();

    /// 从报告对象同步对象列表缓存
    void syncObjectsFromReport();

    /// 更新对象预览图缓存
    void refreshObjectPreview(const ContentBlock& object);

    /// 更新空提示标签的显示状态
    void updateEmptyLabelVisibility();

    /// 清除光标所在块的标题标记（HeadingLevel），避免 toHtml 输出 h1 及相对字号覆盖绝对字号
    void clearBlockHeading(QTextCursor& cursor);

    /// 计算字数
    int computeWordCount() const;


    /// 解析 "object://<type>/<id>" 得到对象
    ContentBlock objectFromImageName(const QString& imageName) const;

    // -----------------------------------------------------------------------
    // 成员变量
    // -----------------------------------------------------------------------

    Ui::ReportEditor* ui;                ///< UI 界面对象（从 .ui 文件自动生成）
    Report::Ptr m_report;                 ///< 当前编辑的报告
    QList<ContentBlock> m_objects;        ///< 结构化对象列表（缓存）
    QHash<QString, QPixmap> m_previewCache; ///< 对象预览图缓存（object://type/id -> pixmap）
    bool m_modified;                      ///< 是否有未保存更改
    bool m_readOnly;                      ///< 只读模式
    bool m_loading;
    bool m_tagsDirty = false;   ///< 标签下拉是否被用户修改过                       ///< 是否正在加载（避免循环触发）

    AutoSaveManager* m_autoSave;          ///< 自动保存管理器
};

#endif // REPORT_EDITOR_H
