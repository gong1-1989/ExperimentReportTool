/**
 * @file OtherBlockEditors.cpp
 * @brief 其他块类型编辑器实现文件（分割线 + 工厂类）
 *
 * 其他块编辑器已拆分为独立文件：
 * - TableBlockEditor.cpp
 * - ImageBlockEditor.cpp
 * - CodeBlockEditor.cpp
 * - ChartBlockEditor.cpp
 *
 * 本文件仅保留 DividerBlockEditor 和 BlockEditorFactory 的实现。
 */

#include "OtherBlockEditors.h"
#include "TextBlockEditor.h"
#include "FormulaBlockEditor.h"
#include "core/plugin/PluginManager.h"
#include "core/plugin/EditorBlockPluginInterface.h"
#include "core/utils/Logger.h"

#include <QVBoxLayout>

// 静态成员变量初始化
PluginManager* BlockEditorFactory::s_pluginManager = nullptr;

// ===========================================================================
// 分割线块编辑器
// ===========================================================================

DividerBlockEditor::DividerBlockEditor(const ContentBlock& block, QWidget* parent)
    : BlockEditor(block, parent)
    , m_divider(nullptr)
{
    setupEditor();

    QVBoxLayout* layout = contentContainer();

    m_divider = new QFrame(this);
    m_divider->setFrameShape(QFrame::HLine);
    m_divider->setFrameShadow(QFrame::Sunken);
    m_divider->setStyleSheet("QFrame { color: #ddd; max-height: 1px; }");
    layout->addWidget(m_divider);

    updateHeight();
}

// ===========================================================================
// 块编辑器工厂
// ===========================================================================

BlockEditor* BlockEditorFactory::createEditor(const ContentBlock& block, QWidget* parent)
{
    // ========================================================================
    // 第一步：优先通过插件创建块编辑器
    // ========================================================================
    if (s_pluginManager) {
        QString typeStr;
        switch (block.type) {
            case BlockType::Paragraph:    typeStr = "paragraph"; break;
            case BlockType::Heading1:     typeStr = "heading1"; break;
            case BlockType::Heading2:     typeStr = "heading2"; break;
            case BlockType::Heading3:     typeStr = "heading3"; break;
            case BlockType::BulletList:   typeStr = "bullet_list"; break;
            case BlockType::NumberedList: typeStr = "numbered_list"; break;
            case BlockType::Quote:        typeStr = "quote"; break;
            case BlockType::Table:        typeStr = "table"; break;
            case BlockType::Image:        typeStr = "image"; break;
            case BlockType::CodeBlock:    typeStr = "code"; break;
            case BlockType::Divider:      typeStr = "divider"; break;
            case BlockType::Chart:        typeStr = "chart"; break;
            case BlockType::DataReference:typeStr = "chart"; break;
            case BlockType::Formula:      typeStr = "formula"; break;
            default:                      typeStr = "paragraph"; break;
        }

        const QList<PluginInterface*> plugins = s_pluginManager->loadedPlugins();
        for (PluginInterface* p : plugins) {
            auto* editorPlugin = dynamic_cast<EditorBlockPluginInterface*>(p);
            if (editorPlugin && editorPlugin->blockType() == typeStr) {
                BlockEditor* editor = editorPlugin->createEditor(block, parent);
                if (editor) {
                    return editor;
                }
            }
        }
    }

    // ========================================================================
    // 第二步：插件未提供时，使用内置实现
    // ========================================================================
    switch (block.type) {
    case BlockType::Heading1:
    case BlockType::Heading2:
    case BlockType::Heading3:
    case BlockType::Paragraph:
    case BlockType::BulletList:
    case BlockType::NumberedList:
    case BlockType::Quote:
        return new TextBlockEditor(block, parent);

    case BlockType::Table:
        return new TableBlockEditor(block, parent);

    case BlockType::Image:
        return new ImageBlockEditor(block, parent);

    case BlockType::CodeBlock:
        return new CodeBlockEditor(block, parent);

    case BlockType::Divider:
        return new DividerBlockEditor(block, parent);

    case BlockType::Chart:
    case BlockType::DataReference:
        return new ChartBlockEditor(block, parent);

    case BlockType::Formula:
        return new FormulaBlockEditor(block, parent);
    }

    return new TextBlockEditor(block, parent);
}

QList<BlockType> BlockEditorFactory::supportedTypes()
{
    return {
        BlockType::Paragraph,
        BlockType::Heading1,
        BlockType::Heading2,
        BlockType::Heading3,
        BlockType::BulletList,
        BlockType::NumberedList,
        BlockType::Quote,
        BlockType::CodeBlock,
        BlockType::Table,
        BlockType::Image,
        BlockType::Chart,
        BlockType::Divider
    };
}

QString BlockEditorFactory::typeDisplayName(BlockType type)
{
    switch (type) {
    case BlockType::Paragraph:     return QObject::tr("正文");
    case BlockType::Heading1:      return QObject::tr("一级标题");
    case BlockType::Heading2:      return QObject::tr("二级标题");
    case BlockType::Heading3:      return QObject::tr("三级标题");
    case BlockType::BulletList:    return QObject::tr("无序列表");
    case BlockType::NumberedList:  return QObject::tr("有序列表");
    case BlockType::Quote:         return QObject::tr("引用");
    case BlockType::CodeBlock:     return QObject::tr("代码块");
    case BlockType::Table:         return QObject::tr("表格");
    case BlockType::Image:         return QObject::tr("图片");
    case BlockType::Chart:         return QObject::tr("图表");
    case BlockType::Divider:       return QObject::tr("分割线");
    case BlockType::Formula:       return QObject::tr("公式");
    case BlockType::DataReference: return QObject::tr("数据引用");
    }
    return QObject::tr("未知");
}
