/**
 * @file OtherBlockEditors.h
 * @brief 其他块类型编辑器统一头文件
 *
 * 本文件为兼容头文件，包含所有非文本块类型编辑器的头文件。
 * 各个块编辑器已拆分为独立文件：
 * - TableBlockEditor.h/cpp
 * - ImageBlockEditor.h/cpp
 * - CodeBlockEditor.h/cpp
 * - ChartBlockEditor.h/cpp
 *
 * DividerBlockEditor 和 BlockEditorFactory 仍在此文件中定义。
 */

#ifndef OTHER_BLOCK_EDITORS_H
#define OTHER_BLOCK_EDITORS_H

// 包含所有拆分后的块编辑器头文件
#include "editor/TableBlockEditor.h"
#include "editor/ImageBlockEditor.h"
#include "editor/CodeBlockEditor.h"
#include "editor/ChartBlockEditor.h"

#include <QFrame>

#include "editor/BlockEditor.h"

// ===========================================================================
// 分割线块编辑器
// ===========================================================================

/**
 * @brief 分割线块编辑器
 *
 * 简单的分割线，用于分隔报告内容。
 * 不可编辑，仅显示一条水平线。
 */
class DividerBlockEditor : public BlockEditor
{
    Q_OBJECT

public:
    explicit DividerBlockEditor(const ContentBlock& block, QWidget* parent = nullptr);

    QJsonObject blockData() const override { return QJsonObject(); }
    void setBlockData(const QJsonObject& data) override { Q_UNUSED(data); }
    BlockType blockType() const override { return BlockType::Divider; }
    bool isEmpty() const override { return false; }

private:
    QFrame* m_divider;
};

// ===========================================================================
// 块编辑器工厂
// ===========================================================================

// 前向声明
class PluginManager;

/**
 * @brief 块编辑器工厂
 *
 * 根据块类型创建对应的 BlockEditor 实例。
 * 优先通过插件创建，插件未提供时使用内置实现。
 */
class BlockEditorFactory
{
public:
    /**
     * @brief 设置插件管理器
     */
    static void setPluginManager(PluginManager* manager) { s_pluginManager = manager; }

    /**
     * @brief 创建块编辑器
     */
    static BlockEditor* createEditor(const ContentBlock& block, QWidget* parent = nullptr);

    /**
     * @brief 获取所有支持的块类型列表
     */
    static QList<BlockType> supportedTypes();

    /**
     * @brief 获取块类型的显示名称
     */
    static QString typeDisplayName(BlockType type);

private:
    static PluginManager* s_pluginManager;
};

#endif // OTHER_BLOCK_EDITORS_H
