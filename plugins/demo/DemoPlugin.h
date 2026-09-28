/**
 * @file DemoPlugin.h
 * @brief 独立 .dll 示例插件
 *
 * 演示完整的插件加载链路：
 * 1. 编译为独立动态库（.dll），输出到 exe 同级 plugins/ 目录
 * 2. PluginManager 启动时扫描 plugins/ 目录并加载
 * 3. 插件管理对话框显示该插件
 *
 * 本插件不提供任何业务扩展点（扩展点以后需要时再定义），
 * 仅用于验证插件框架的"加载→查询→调用"链路是否畅通。
 */

#ifndef DEMO_PLUGIN_H
#define DEMO_PLUGIN_H

#include <QObject>
#include <QtPlugin>

#include "core/plugin/PluginInterface.h"

/**
 * @brief 示例插件（独立动态库）
 *
 * 插件 .dll 的入口类：继承 QObject + PluginInterface，
 * 用 Q_PLUGIN_METADATA 声明插件元数据，用 Q_INTERFACES 声明实现的接口。
 */
class DemoPlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugins.demo")
    Q_INTERFACES(PluginInterface)

public:
    // ========================================================================
    // PluginInterface 元信息
    // ========================================================================

    QString name() const override { return QStringLiteral("示例插件"); }
    QString iid() const override { return QStringLiteral("com.examplereporttool.plugins.demo"); }
    QString version() const override { return QStringLiteral("1.0.0"); }
    QString description() const override
    {
        return QStringLiteral("演示独立 .dll 插件的加载、查询与生命周期调用链路");
    }
    QString author() const override { return QStringLiteral("ExperimentReportTool"); }
    QString category() const override { return QStringLiteral("General"); }

    // ========================================================================
    // 生命周期
    // ========================================================================

    bool initialize() override;
    void shutdown() override;
};

#endif // DEMO_PLUGIN_H
