/**
 * @file ReportEditor.h
 * @brief 报告编辑器主组件头文件
 *
 * ReportEditor 是报告编辑的核心组件，管理所有内容块编辑器。
 * 负责块的增删改查、块间导航、类型转换、内容序列化等。
 *
 * 布局：
 * - 顶部：报告标题编辑栏 + 元信息（日期、状态）
 * - 中部：滚动区域，包含所有块编辑器
 * - 底部：字数统计 + 保存状态
 */

#ifndef REPORT_EDITOR_H
#define REPORT_EDITOR_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QTimer>

#include "core/models/Report.h"
#include "editor/BlockEditor.h"

// 前向声明
class AutoSaveManager;

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
     */
    QString reportTitle() const { return m_titleEdit->text(); }

    // -----------------------------------------------------------------------
    // 块操作
    // -----------------------------------------------------------------------

    /// 获取块数量
    int blockCount() const { return m_blockEditors.size(); }

    /// 获取指定索引的块编辑器
    BlockEditor* blockEditorAt(int index) const;

    /// 获取当前焦点所在的块索引
    int currentBlockIndex() const { return m_currentBlockIndex; }

    /**
     * @brief 在指定位置插入块
     * @param index 插入位置
     * @param type 块类型
     * @return 新创建的块编辑器
     */
    BlockEditor* insertBlock(int index, BlockType type);

    /**
     * @brief 追加块到末尾
     * @param type 块类型
     * @return 新创建的块编辑器
     */
    BlockEditor* appendBlock(BlockType type);

    /**
     * @brief 删除指定块
     * @param index 块索引
     */
    void removeBlock(int index);

    /**
     * @brief 移动块
     * @param from 源索引
     * @param to 目标索引
     */
    void moveBlock(int from, int to);

    /**
     * @brief 转换块类型
     * @param index 块索引
     * @param newType 新类型
     */
    void convertBlock(int index, BlockType newType);

    /**
     * @brief 复制块（在当前块之后插入副本）
     * @param index 块索引
     */
    void duplicateBlock(int index);

    // -----------------------------------------------------------------------
    // 编辑操作
    // -----------------------------------------------------------------------

    /// 设置焦点到指定块
    void focusBlock(int index);

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

    /// 块数量变化
    void blockCountChanged(int count);

private slots:
    // -----------------------------------------------------------------------
    // 块编辑器信号处理
    // -----------------------------------------------------------------------
    void onBlockContentChanged();
    void onBlockFocused(BlockEditor* editor);
    void onRequestInsertAfter(BlockEditor* editor, BlockType type);
    void onRequestInsertBefore(BlockEditor* editor, BlockType type);
    void onRequestDelete(BlockEditor* editor);
    void onRequestConvert(BlockEditor* editor, BlockType newType);
    void onRequestFocusPrevious(BlockEditor* editor);
    void onRequestFocusNext(BlockEditor* editor);
    void onRequestMoveUp(BlockEditor* editor);
    void onRequestMoveDown(BlockEditor* editor);

    // -----------------------------------------------------------------------
    // 标题栏信号
    // -----------------------------------------------------------------------
    void onTitleChanged(const QString& title);
    void onStatusChanged(int index);
    void onDateChanged(const QDate& date);

    // -----------------------------------------------------------------------
    // 工具栏
    // -----------------------------------------------------------------------
    void onAddBlock();
    void onUndo();
    void onRedo();

private:
    // -----------------------------------------------------------------------
    // 内部方法
    // -----------------------------------------------------------------------

    /// 初始化 UI
    void setupUi();

    /// 重建所有块编辑器（从报告内容）
    void rebuildBlocks();

    /// 清空所有块编辑器
    void clearBlocks();

    /// 查找块编辑器的索引
    int indexOfBlockEditor(BlockEditor* editor) const;

    /// 连接块编辑器的信号
    void connectBlockEditor(BlockEditor* editor);

    /// 更新底部状态栏
    void updateStatusBar();

    /// 更新块的视觉选中状态
    void updateBlockSelection();

    /// 更新所有图表块的报告 ID（用于查找数据表）
    void updateChartBlockReportId();

    // -----------------------------------------------------------------------
    // 成员变量 - UI
    // -----------------------------------------------------------------------

    // 顶部标题栏
    QLineEdit* m_titleEdit;        ///< 报告标题
    QComboBox* m_statusCombo;       ///< 报告状态
    QDateEdit* m_dateEdit;          ///< 实验日期
    QLineEdit* m_authorEdit;        ///< 作者

    // 工具栏
    QPushButton* m_addBlockBtn;     ///< 添加块按钮
    QPushButton* m_undoBtn;         ///< 撤销
    QPushButton* m_redoBtn;         ///< 重做

    // 滚动区域
    QScrollArea* m_scrollArea;       ///< 滚动区域
    QWidget* m_blocksContainer;      ///< 块容器
    QVBoxLayout* m_blocksLayout;     ///< 块布局

    // 底部状态栏
    QLabel* m_wordCountLabel;        ///< 字数统计
    QLabel* m_blockCountLabel;       ///< 块数量
    QLabel* m_saveStatusLabel;       ///< 保存状态

    // -----------------------------------------------------------------------
    // 成员变量 - 数据
    // -----------------------------------------------------------------------

    Report::Ptr m_report;             ///< 当前编辑的报告
    QList<BlockEditor*> m_blockEditors;  ///< 块编辑器列表
    int m_currentBlockIndex;          ///< 当前焦点块索引
    bool m_modified;                   ///< 是否有未保存更改
    bool m_readOnly;                   ///< 只读模式
    bool m_loading;                    ///< 是否正在加载（避免循环触发）

    AutoSaveManager* m_autoSave;      ///< 自动保存管理器
};

#endif // REPORT_EDITOR_H
