/**
 * @file PluginManagerPlugin.cpp
 * @brief 插件管理工具插件实现文件
 */

#include "PluginManagerPlugin.h"
#include "core/plugin/CoreService.h"
#include "core/plugin/PluginManager.h"
#include "ui/dialogs/PluginManagerDialog.h"
#include "core/utils/Logger.h"
#include <QMessageBox>
#include <QWidget>

PluginManagerPlugin::PluginManagerPlugin(QObject* parent)
    : QObject(parent)
    , m_core(nullptr)
{
}

bool PluginManagerPlugin::initialize(CoreService* core)
{
    m_core = core;
    if (m_core) {
        m_core->logger()->info("插件管理插件已初始化");
    }
    return true;
}

void PluginManagerPlugin::shutdown()
{
    if (m_core) {
        m_core->logger()->info("插件管理插件已关闭");
    }
    m_core = nullptr;
}

bool PluginManagerPlugin::execute(QWidget* parent, const QVariantMap& context)
{
    // 从上下文中获取插件管理器
    PluginManager* pluginManager = context.value("pluginManager").value<PluginManager*>();
    if (!pluginManager) {
        QMessageBox::warning(parent, tr("插件管理"), tr("插件管理器未初始化"));
        return false;
    }

    PluginManagerDialog dialog(pluginManager, parent);
    dialog.exec();
    return true;
}
