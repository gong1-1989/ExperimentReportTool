/**
 * @file SamplePlugin.cpp
 * @brief 示例插件实现文件
 */

#include "SamplePlugin.h"
#include "core/plugin/CoreService.h"
#include "core/utils/Logger.h"

#include <QMessageBox>

// ============================================================================
// 构造
// ============================================================================

SamplePlugin::SamplePlugin(QObject* parent)
    : QObject(parent)
    , m_core(nullptr)
{
}

// ============================================================================
// PluginInterface 实现
// ============================================================================

bool SamplePlugin::initialize(CoreService* core)
{
    m_core = core;

    if (m_core) {
        m_core->logger()->info("示例插件已初始化");

        // 示例：通过核心服务访问设置
        // m_core->settings()->setValue("sample/enabled", true);

        // 示例：通过核心服务访问数据库
        // m_core->database()->...
    }

    return true;
}

void SamplePlugin::shutdown()
{
    if (m_core) {
        m_core->logger()->info("示例插件已关闭");
    }
    m_core = nullptr;
}
