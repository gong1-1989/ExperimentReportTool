/**
 * @file DemoPlugin.cpp
 * @brief 独立 .dll 示例插件实现
 */

#include "DemoPlugin.h"

#include "core/utils/Logger.h"

bool DemoPlugin::initialize()
{
    // 插件加载后由 PluginManager 调用，可在此执行资源准备
    Logger::instance().info("示例插件初始化完成 (DemoPlugin)");
    return true;
}

void DemoPlugin::shutdown()
{
    // 程序退出前由 PluginManager 调用，进行资源清理
    Logger::instance().info("示例插件已关闭 (DemoPlugin)");
}
