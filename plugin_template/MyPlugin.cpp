/**
 * @file MyPlugin.cpp
 * @brief 插件类模板实现（新建插件时复制并修改）
 */

#include "MyPlugin.h"

#include "core/utils/Logger.h"

bool MyPlugin::initialize()
{
    // 插件加载后由 PluginManager 调用，可在此执行资源准备、注册扩展点等
    Logger::instance().info(QString("插件初始化完成: %1").arg(name()));
    return true;  // 返回 false 表示初始化失败，插件将不被启用
}

void MyPlugin::shutdown()
{
    // 程序退出前由 PluginManager 调用，进行资源清理
    Logger::instance().info(QString("插件已关闭: %1").arg(name()));
}
