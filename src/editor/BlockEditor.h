/**
 * @file BlockEditor.h
 * @brief 内容块编辑器基类头文件
 *
 * BlockEditor 是所有内容块编辑器的抽象基类。
 * 报告编辑器由多个 BlockEditor 子类实例纵向排列组成，
 * 每个 BlockEditor 对应报告内容中的一个 ContentBlock。
 *
 * 设计思路类似 Notion / 语雀的块级编辑器：
 * - 每个块独立编辑
 * - 块之间可以通过键盘导航（上下箭头）
 * - 支持块类型转换
 * - 支持拖拽排序
 */

#ifndef BLOCK_EDITOR_H
#define BLOCK_EDITOR_H

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QFocusEvent>
#include <QKeyEvent>

#include "core/models/Report.h"

/**
 * @brief 块编辑器基类
 *
 * 所有具体块编辑器（文本、标题、表格、图片等）都继承此类。
 *
 * 子类需要实现：
 * - blockData()：返回块的数据（QJsonObject）
 * - setBlockData()：设置块数据
 * - blockType()：返回块类型
 * - setupEditor()：构建编辑器 UI
 *
 * 基类提供：
 * - 块左侧的操作手柄（拖拽、菜单）
 * - 焦点管理
 * - 块类型转换菜单
 * - 键盘事件处理（Enter 新建块、Backspace 删除空块等）
 */
class BlockEditor : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param block 内容块数据
     * @param parent 父窗口
     */
    explicit BlockEditor(const ContentBlock& block, QWidget* parent = nullptr);

    ~BlockEditor() override;

    // -----------------------------------------------------------------------
    // 纯虚方法（子类必须实现）
    // -----------------------------------------------------------------------

    /**
     * @brief 获取块数据
     * @return 块数据的 JSON 对象
     */
    virtual QJsonObject blockData() const = 0;

    /**
     * @brief 设置块数据
     * @param data 块数据 JSON
     */
    virtual void setBlockData(const QJsonObject& data) = 0;

    /**
     * @brief 获取块类型
     * @return 块类型枚举
     */
    virtual BlockType blockType() const = 0;

    /**
     * @brief 获取块的纯文本内容（用于搜索、字数统计）
     * @return 纯文本
     */
    virtual QString plainText() const { return QString(); }

    /**
     * @brief 设置焦点到编辑器
     */
    virtual void setFocusToEditor() {}

    // -----------------------------------------------------------------------
    // 通用方法
    // -----------------------------------------------------------------------

    /// 获取块 ID
    QString blockId() const { return m_block.id; }

    /// 设置块 ID
    void setBlockId(const QString& id) { m_block.id = id; }

    /// 获取完整的 ContentBlock
    ContentBlock contentBlock() const;

    /// 设置块类型（用于类型转换）
    void setBlockType(BlockType type);

    /// 是否为空块（用于 Backspace 删除判断）
    virtual bool isEmpty() const { return false; }

    /// 设置选中状态（视觉高亮）
    void setBlockSelected(bool selected);

    /// 设置只读模式
    void setReadOnly(bool readOnly);

    /// 是否只读
    bool isReadOnly() const { return m_readOnly; }

signals:
    /**
     * @brief 请求在当前块之后插入新块
     * @param blockEditor 当前块（this）
     * @param type 新块类型
     */
    void requestInsertBlockAfter(BlockEditor* blockEditor, BlockType type);

    /**
     * @brief 请求在当前块之前插入新块
     * @param blockEditor 当前块
     * @param type 新块类型
     */
    void requestInsertBlockBefore(BlockEditor* blockEditor, BlockType type);

    /**
     * @brief 请求删除当前块
     * @param blockEditor 当前块
     */
    void requestDeleteBlock(BlockEditor* blockEditor);

    /**
     * @brief 请求将当前块转换为其他类型
     * @param blockEditor 当前块
     * @param newType 目标类型
     */
    void requestConvertBlock(BlockEditor* blockEditor, BlockType newType);

    /**
     * @brief 请求将焦点移动到上一个块
     * @param blockEditor 当前块
     */
    void requestFocusPrevious(BlockEditor* blockEditor);

    /**
     * @brief 请求将焦点移动到下一个块
     * @param blockEditor 当前块
     */
    void requestFocusNext(BlockEditor* blockEditor);

    /**
     * @brief 请求向上移动块（排序）
     * @param blockEditor 当前块
     */
    void requestMoveUp(BlockEditor* blockEditor);

    /**
     * @brief 请求向下移动块（排序）
     * @param blockEditor 当前块
     */
    void requestMoveDown(BlockEditor* blockEditor);

    /**
     * @brief 块内容变化信号（用于自动保存）
     */
    void contentChanged();

    /**
     * @brief 块获得焦点
     */
    void blockFocused(BlockEditor* blockEditor);

protected:
    // -----------------------------------------------------------------------
    // 子类可调用的保护方法
    // -----------------------------------------------------------------------

    /**
     * @brief 初始化编辑器 UI（子类在构造函数中调用）
     *
     * 基类会创建左侧操作手柄，子类的编辑器内容放在右侧。
     */
    void setupEditor();

    /**
     * @brief 获取编辑器内容容器（子类将自己的控件放入此布局）
     * @return 内容容器布局
     */
    QVBoxLayout* contentContainer() { return m_contentLayout; }

    /**
     * @brief 处理通用键盘事件（Enter、Backspace、方向键等）
     *
     * 子类在 keyPressEvent 中先调用此方法，返回 true 表示已处理。
     */
    bool handleCommonKeyPress(QKeyEvent* event);

    /**
     * @brief 显示块类型转换菜单
     * @param pos 菜单显示位置
     */
    void showBlockTypeMenu(const QPoint& pos);

    /// 内容变化时调用（发射 contentChanged 信号）
    void notifyContentChanged();

    // -----------------------------------------------------------------------
    // 事件处理
    // -----------------------------------------------------------------------

    void focusInEvent(QFocusEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

protected:
    ContentBlock m_block;       ///< 关联的内容块
    bool m_readOnly;            ///< 只读模式
    bool m_selected;            ///< 是否选中

private:
    /// 创建左侧操作手柄
    void createHandle();

    /// 手柄菜单动作
    QAction* m_actionMoveUp;
    QAction* m_actionMoveDown;
    QAction* m_actionConvert;
    QAction* m_actionDelete;
    QAction* m_actionDuplicate;

    QPushButton* m_handleButton;  ///< 左侧手柄按钮（⋮⋮）
    QVBoxLayout* m_contentLayout;  ///< 内容容器布局
    QHBoxLayout* m_mainLayout;     ///< 主布局（手柄 + 内容）
};

#endif // BLOCK_EDITOR_H
