/**
 * @file ExtensionManager.h
 * @brief 扩展能力统一聚合入口
 *
 * 统一管理各域契约注册表（导出/导入/分析/对象/工具/报表/AI），
 * 主程序只通过本类访问插件能力，向 UI 提供聚合菜单/格式列表。
 *
 * 当前已接入：ExportRegistry（A 域）。
 * 后续批次并入：AnalyzerRegistry（B）、ObjectRegistry（D）、
 * ToolRegistry（F）、StatsRegistry（G）、AiRegistry（I）。
 */

#ifndef EXTENSION_MANAGER_H
#define EXTENSION_MANAGER_H

#include "ExportRegistry.h"
#include "AnalyzerRegistry.h"
#include "StatsRegistry.h"
#include "ToolRegistry.h"
#include "ObjectRegistry.h"

class ExtensionManager
{
public:
    static ExtensionManager& instance();

    ExportRegistry& exportRegistry() { return ExportRegistry::instance(); }
    AnalyzerRegistry& analyzerRegistry() { return AnalyzerRegistry::instance(); }
    StatsRegistry& statsRegistry() { return StatsRegistry::instance(); }
    ToolRegistry& toolRegistry() { return ToolRegistry::instance(); }
    ObjectRegistry& objectRegistry() { return ObjectRegistry::instance(); }

private:
    ExtensionManager() = default;
    static ExtensionManager* s_instance;
};

#endif // EXTENSION_MANAGER_H
