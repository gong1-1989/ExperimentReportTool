/**
 * @file PluginInterface.h
 * @brief 插件抽象基类接口头文件
 *
 * 所有插件必须继承此接口。插件框架通过此接口管理插件的生命周期。
 *
 * 插件开发流程：
 * 1. 继承 PluginInterface
 * 2. 实现 name()、version()、description() 等元信息方法
 * 3. 实现 initialize() 进行插件初始化
 * 4. 实现 shutdown() 进行插件清理
 * 5. 使用 Q_PLUGIN_METADATA 宏声明插件元数据
 * 6. 使用 Q_INTERFACES 宏声明实现的接口
 *
 * 参考示例：plugins/demo/DemoPlugin.h（独立 .dll 示例插件）、
 *           plugin_template/MyPlugin.h（插件脚手架模板）
 */

#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <QString>
#include <QVariantMap>
#include <QtPlugin>

/**
 * @brief 插件抽象基类接口
 *
 * 这是所有插件的唯一基类。每个插件必须提供：
 * - 唯一标识符（IID）
 * - 名称、版本、描述等元信息
 * - 初始化和清理方法
 *
 * @code
 * class MyPlugin : public QObject, public PluginInterface {
 *     Q_OBJECT
 *     Q_PLUGIN_METADATA(IID "com.example.MyPlugin")
 *     Q_INTERFACES(PluginInterface)
 * public:
 *     QString name() const override { return "My Plugin"; }
 *     // ...
 * };
 * @endcode
 */
class PluginInterface
{
public:
    /// 虚析构函数，确保子类正确析构
    virtual ~PluginInterface() = default;

    // ========================================================================
    // 插件元信息
    // ========================================================================

    /**
     * @brief 插件名称（显示给用户）
     * @return 插件名称
     */
    virtual QString name() const = 0;

    /**
     * @brief 插件唯一标识符（IID）
     * @return 插件 IID，格式如 "com.examplereporttool.plugin.xxx"
     */
    virtual QString iid() const = 0;

    /**
     * @brief 插件版本号
     * @return 版本字符串，如 "1.0.0"
     */
    virtual QString version() const = 0;

    /**
     * @brief 插件描述
     * @return 描述文本
     */
    virtual QString description() const = 0;

    /**
     * @brief 插件作者
     * @return 作者名称
     */
    virtual QString author() const { return QString(); }

    /**
     * @brief 插件类别（用于分类显示）
     * @return 类别名称，如 "Editor"、"Export"、"Tool"
     */
    virtual QString category() const { return QStringLiteral("General"); }

    // ========================================================================
    // 生命周期
    // ========================================================================

    /**
     * @brief 初始化插件
     *
     * 在插件加载后调用，此时可以执行资源准备、注册等操作。
     *
     * @return 初始化成功返回 true，失败返回 false
     */
    virtual bool initialize() = 0;

    /**
     * @brief 插件初始化完成后的回调
     *
     * 所有插件都 initialize() 完成后调用，此时可以依赖其他插件。
     */
    virtual void initialized() {}

    /**
     * @brief 关闭插件
     *
     * 在程序退出前调用，进行资源清理。
     */
    virtual void shutdown() {}

    // ========================================================================
    // 依赖与能力
    // ========================================================================

    /**
     * @brief 插件依赖的其他插件 IID 列表
     * @return 依赖的插件 IID 列表
     */
    virtual QStringList dependencies() const { return QStringList(); }

    /**
     * @brief 插件是否可以被禁用
     * @return true 表示用户可以在设置中禁用此插件
     */
    virtual bool isOptional() const { return true; }

    /**
     * @brief 获取插件配置项（用于设置界面）
     * @return 配置项描述，key 为配置名，value 为默认值
     */
    virtual QVariantMap defaultConfig() const { return QVariantMap(); }
};

// 声明插件接口（Qt 插件系统要求）
// 注意：接口变更（如 initialize 签名变化）时必须升级此版本号，
// 否则旧插件 .dll 会被 qobject_cast 误判为兼容，导致 ABI 崩溃（SIGSEGV）。
#define PLUGIN_INTERFACE_IID "com.examplereporttool.PluginInterface/2.0"
Q_DECLARE_INTERFACE(PluginInterface, PLUGIN_INTERFACE_IID)

#endif // PLUGIN_INTERFACE_H
