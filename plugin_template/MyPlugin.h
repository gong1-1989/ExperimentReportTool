/**
 * @file MyPlugin.h
 * @brief 插件类模板（新建插件时复制并修改）
 *
 * 所有插件必须：
 * 1. 继承 QObject + PluginInterface（唯一抽象基类，见 core/plugin/PluginInterface.h）
 * 2. 用 Q_PLUGIN_METADATA 声明插件元数据（IID 必须全局唯一）
 * 3. 用 Q_INTERFACES 声明实现的接口
 *
 * 以后需要扩展点（如新块类型、新导入/导出格式）时，
 * 在 core/plugin/ 下定义新的子接口（继承 PluginInterface），
 * 插件实现该子接口即可被框架按类型查询到（PluginManager::pluginsOfType<T>()）。
 */

#ifndef MY_PLUGIN_H
#define MY_PLUGIN_H

#include <QObject>
#include <QtPlugin>

#include "core/plugin/PluginInterface.h"

/**
 * @brief 自定义插件
 */
class MyPlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    // IID 必须是全局唯一的字符串，格式建议：com.examplereporttool.plugins.<插件名>
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugins.myplugin")
    Q_INTERFACES(PluginInterface)

public:
    // ========================================================================
    // 元信息（必须实现）
    // ========================================================================

    QString name() const override { return QStringLiteral("我的插件"); }
    QString iid() const override { return QStringLiteral("com.examplereporttool.plugins.myplugin"); }
    QString version() const override { return QStringLiteral("1.0.0"); }
    QString description() const override { return QStringLiteral("插件描述"); }
    QString author() const override { return QStringLiteral("作者"); }
    QString category() const override { return QStringLiteral("General"); }

    // ========================================================================
    // 生命周期（必须实现 initialize，shutdown 可选）
    // ========================================================================

    bool initialize() override;
    void shutdown() override;
};

#endif // MY_PLUGIN_H
