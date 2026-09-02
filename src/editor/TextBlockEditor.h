/**
 * @file TextBlockEditor.h
 * @brief 文本块编辑器头文件
 *
 * TextBlockEditor 是最常用的块编辑器，用于以下块类型：
 * - Paragraph（段落）
 * - Heading1/Heading2/Heading3（标题）
 * - BulletList/NumberedList（列表）
 * - Quote（引用）
 *
 * 基于 QTextEdit 实现，支持：
 * - 富文本（加粗、斜体、下划线、链接、行内代码）
 * - 自动调整高度（随内容增长）
 * - 占位符提示文字
 * - Markdown 风格快捷输入（# 空格转标题、- 空格转列表等）
 * - 列表自动编号
 */

#ifndef TEXT_BLOCK_EDITOR_H
#define TEXT_BLOCK_EDITOR_H

#include <QTextEdit>
#include <QTextDocument>
#include <QTextCursor>

#include "editor/BlockEditor.h"

/**
 * @brief 自动调整高度的文本编辑控件
 *
 * 继承 QTextEdit，重写尺寸提示，使高度随内容自动变化。
 * 这是 TextBlockEditor 的内部编辑控件。
 */
class AutoResizeTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit AutoResizeTextEdit(QWidget* parent = nullptr);

    /// 推荐高度（根据文档内容计算）
    QSize sizeHint() const override;

    /// 最小高度
    QSize minimumSizeHint() const override;

    /// 设置占位符文字
    void setPlaceholderText(const QString& text);
    QString placeholderText() const { return m_placeholder; }

signals:
    /// 高度变化信号
    void heightChanged(int height);

protected:
    /// 绘制占位符
    void paintEvent(QPaintEvent* event) override;
    /// 内容变化时重新计算高度
    void keyPressEvent(QKeyEvent* event) override;
    /// 粘贴事件
    void insertFromMimeData(const QMimeData* source) override;

private slots:
    /// 文档内容变化时更新高度
    void onDocumentSizeChanged(const QSizeF& size);

private:
    QString m_placeholder;  ///< 占位符文字
    int m_minHeight;        ///< 最小高度
};

/**
 * @brief 文本块编辑器
 */
class TextBlockEditor : public BlockEditor
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param block 内容块数据
     * @param parent 父窗口
     */
    explicit TextBlockEditor(const ContentBlock& block, QWidget* parent = nullptr);

    // -----------------------------------------------------------------------
    // BlockEditor 接口实现
    // -----------------------------------------------------------------------

    QJsonObject blockData() const override;
    void setBlockData(const QJsonObject& data) override;
    BlockType blockType() const override;
    QString plainText() const override;
    void setFocusToEditor() override;
    bool isEmpty() const override;

    // -----------------------------------------------------------------------
    // 文本块特有方法
    // -----------------------------------------------------------------------

    /// 获取文本内容（HTML）
    QString toHtml() const;

    /// 获取纯文本
    QString toPlainText() const;

    /// 设置 HTML 内容
    void setHtml(const QString& html);

    /// 设置纯文本
    void setPlainText(const QString& text);

    /// 设置块类型（段落/标题/列表/引用）
    void setTextBlockType(BlockType type);

    /// 获取当前块类型
    BlockType textBlockType() const { return m_textType; }

    /// 应用文本格式（加粗、斜体等）
    void applyFormat(const QString& format);

    /// 获取内部编辑控件
    AutoResizeTextEdit* textEdit() const { return m_textEdit; }

protected:
    /// 键盘事件处理
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    /// 文本变化
    void onTextChanged();
    /// 处理 Markdown 风格快捷输入
    void checkMarkdownShortcuts();
    /// 处理列表自动编号
    void updateListNumbering();

private:
    /// 根据块类型更新样式
    void updateStyleForType();

    /// 获取占位符文字
    QString placeholderForType() const;

    // -----------------------------------------------------------------------
    // 成员变量
    // -----------------------------------------------------------------------

    AutoResizeTextEdit* m_textEdit;  ///< 内部文本编辑控件
    BlockType m_textType;             ///< 文本块类型
    bool m_loadingData;               ///< 是否正在加载数据（避免循环触发）
};

#endif // TEXT_BLOCK_EDITOR_H
