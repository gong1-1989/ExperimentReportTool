/**
 * @file ToolPluginInterface.h
 * @brief 工具插件接口头文件
 *
 * 工具插件提供独立的功能工具，如搜索、标签管理、附件管理、版本管理等。
 * 工具插件通过菜单项或工具栏按钮触发，弹出对话框或执行操作。
 */

#ifndef TOOL_PLUGIN_INTERFACE_H
#define TOOL_PLUGIN_INTERFACE_H

#include "PluginInterface.h"

class QWidget;
class QAction;

/**
 * @brief 工具插件接口
 *
 * 所有工具类插件都需要实现此接口。
 * 工具插件通常提供一个菜单项，点击后执行特定功能。
 */
class ToolPluginInterface : virtual public PluginInterface
{
public:
    virtual ~ToolPluginInterface() = default;

    /**
     * @brief 获取工具在菜单中的显示名称
     * @return 菜单名称，如 "搜索..."
     */
    virtual QString toolName() const = 0;

    /**
     * @brief 获取工具所属的菜单分类
     * @return 分类名称，如 "编辑"、"工具"、"视图"
     */
    virtual QString toolCategory() const = 0;

    /**
     * @brief 获取工具的快捷键
     * @return 快捷键字符串，如 "Ctrl+F"，无则返回空
     */
    virtual QString toolShortcut() const { return QString(); }

    /**
     * @brief 执行工具功能
     * @param parent 父窗口（用于对话框）
     * @param context 上下文数据（如当前报告ID等）
     * @return 成功返回 true
     */
    virtual bool execute(QWidget* parent, const QVariantMap& context = QVariantMap()) = 0;

    /**
     * @brief 工具是否可用（根据上下文判断）
     * @param context 上下文数据
     * @return 可用返回 true
     */
    virtual bool isAvailable(const QVariantMap& context = QVariantMap()) const {
        Q_UNUSED(context);
        return true;
    }
};

#define ToolPluginInterface_iid "com.examplereporttool.plugin.ToolPluginInterface"
Q_DECLARE_INTERFACE(ToolPluginInterface, ToolPluginInterface_iid)

#endif // TOOL_PLUGIN_INTERFACE_H
