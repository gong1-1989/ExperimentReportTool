/**
 * @file TagManagerPlugin.cpp
 * @brief 标签管理插件实现文件
 */

#include "TagManagerPlugin.h"
#include "core/plugin/CoreService.h"
#include "ui/dialogs/TagManagerDialog.h"
#include "core/utils/Logger.h"

TagManagerPlugin::TagManagerPlugin(QObject* parent)
    : QObject(parent)
    , m_core(nullptr)
{
}

bool TagManagerPlugin::initialize(CoreService* core)
{
    m_core = core;
    if (m_core) m_core->logger()->info("标签管理插件已初始化");
    return true;
}

void TagManagerPlugin::shutdown()
{
    if (m_core) m_core->logger()->info("标签管理插件已关闭");
    m_core = nullptr;
}

bool TagManagerPlugin::execute(QWidget* parent, const QVariantMap& context)
{
    Q_UNUSED(context);
    TagManagerDialog dialog(parent);
    dialog.exec();
    return true;
}
