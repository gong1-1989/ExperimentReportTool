/**
 * @file SamplePlugin.h
 * @brief 示例插件头文件
 *
 * 展示如何开发一个插件。此插件在初始化时注册一个菜单项，
 * 点击后显示插件信息对话框。
 */

#ifndef SAMPLE_PLUGIN_H
#define SAMPLE_PLUGIN_H

#include <QObject>
#include "core/plugin/PluginInterface.h"

/**
 * @brief 示例插件
 *
 * 这是一个最简单的插件示例，展示插件的基本结构：
 * - 继承 QObject 和 PluginInterface
 * - 使用 Q_INTERFACES 声明实现的接口
 * - 实现 name()、iid()、version() 等元信息方法
 * - 实现 initialize() 和 shutdown() 生命周期方法
 */
class SamplePlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface)
    Q_PLUGIN_METADATA(IID "com.examplereporttool.plugin.Sample")

public:
    /// 构造函数
    explicit SamplePlugin(QObject* parent = nullptr);

    // ========================================================================
    // PluginInterface 实现
    // ========================================================================

    QString name() const override { return tr("示例插件"); }
    QString iid() const override { return "com.examplereporttool.plugin.Sample"; }
    QString version() const override { return "1.0.0"; }
    QString description() const override { return tr("展示插件开发方式的示例插件"); }
    QString author() const override { return "ExperimentReportTool Team"; }
    QString category() const override { return "Sample"; }

    bool initialize(CoreService* core) override;
    void shutdown() override;

private:
    CoreService* m_core;  ///< 核心服务
};

#endif // SAMPLE_PLUGIN_H
