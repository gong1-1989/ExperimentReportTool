/**
 * @file EditorBlockPluginInterface.h
 * @brief 编辑器块插件接口头文件
 *
 * 编辑器块插件用于扩展报告编辑器中的内容块类型，
 * 如文本块、表格块、图表块、公式块等。
 *
 * 新增一种内容块只需要实现此接口并注册为插件即可，
 * 不需要修改编辑器核心代码。
 */

#ifndef EDITOR_BLOCK_PLUGIN_INTERFACE_H
#define EDITOR_BLOCK_PLUGIN_INTERFACE_H

#include "PluginInterface.h"

// 前向声明
class ContentBlock;
class BlockEditor;
class QWidget;

/**
 * @brief 编辑器块插件接口
 *
 * 实现此接口的插件可以为报告编辑器添加新的内容块类型。
 *
 * @code
 * class ChartBlockPlugin : public QObject, public EditorBlockPluginInterface {
 *     Q_OBJECT
 *     Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.ChartBlock")
 *     Q_INTERFACES(PluginInterface EditorBlockPluginInterface)
 * public:
 *     QString blockType() const override { return "chart"; }
 *     QString blockDisplayName() const override { return "图表"; }
 *     BlockEditor* createEditor(const ContentBlock& block, QWidget* parent) override;
 * };
 * @endcode
 */
class EditorBlockPluginInterface : virtual public PluginInterface
{
public:
    virtual ~EditorBlockPluginInterface() = default;

    // ========================================================================
    // 块类型信息
    // ========================================================================

    /**
     * @brief 块类型标识符
     * @return 块类型字符串，如 "paragraph"、"table"、"chart"
     */
    virtual QString blockType() const = 0;

    /**
     * @brief 块类型显示名称
     * @return 显示名称，如 "段落"、"表格"、"图表"
     */
    virtual QString blockDisplayName() const = 0;

    /**
     * @brief 块类型图标
     * @return 图标名称或路径
     */
    virtual QString blockIcon() const { return QString(); }

    /**
     * @brief 块类型描述
     * @return 描述文本
     */
    virtual QString blockDescription() const { return QString(); }

    // ========================================================================
    // 块创建与编辑
    // ========================================================================

    /**
     * @brief 创建默认的内容块
     * @return 新创建的内容块
     */
    virtual ContentBlock createDefaultBlock() const = 0;

    /**
     * @brief 创建块编辑器控件
     * @param block 要编辑的内容块
     * @param parent 父窗口
     * @return 块编辑器实例
     */
    virtual BlockEditor* createEditor(const ContentBlock& block, QWidget* parent) = 0;

    // ========================================================================
    // 渲染与导出
    // ========================================================================

    /**
     * @brief 将块渲染为 HTML（用于打印和导出）
     * @param block 内容块
     * @return HTML 字符串
     */
    virtual QString renderToHtml(const ContentBlock& block) const {
        Q_UNUSED(block);
        return QString();
    }

    /**
     * @brief 获取块的纯文本内容（用于全文搜索和字数统计）
     * @param block 内容块
     * @return 纯文本
     */
    virtual QString plainText(const ContentBlock& block) const {
        Q_UNUSED(block);
        return QString();
    }

    // ========================================================================
    // 工具栏
    // ========================================================================

    /**
     * @brief 是否在"添加块"菜单中显示
     * @return true 表示显示
     */
    virtual bool showInAddMenu() const { return true; }
};

#define EDITOR_BLOCK_PLUGIN_INTERFACE_IID "com.examplereporttool.EditorBlockPluginInterface/1.0"
Q_DECLARE_INTERFACE(EditorBlockPluginInterface, EDITOR_BLOCK_PLUGIN_INTERFACE_IID)

#endif // EDITOR_BLOCK_PLUGIN_INTERFACE_H
