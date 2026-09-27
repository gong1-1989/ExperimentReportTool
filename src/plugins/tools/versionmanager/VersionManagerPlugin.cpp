/**
 * @file VersionManagerPlugin.cpp
 * @brief 版本管理插件实现文件
 */

#include "VersionManagerPlugin.h"
#include "core/plugin/CoreService.h"
#include "ui/dialogs/VersionHistoryDialog.h"
#include "core/utils/Logger.h"

VersionManagerPlugin::VersionManagerPlugin(QObject* parent)
    : QObject(parent)
    , m_core(nullptr)
{
}

bool VersionManagerPlugin::initialize(CoreService* core)
{
    m_core = core;
    if (m_core) m_core->logger()->info("版本管理插件已初始化");
    return true;
}

void VersionManagerPlugin::shutdown()
{
    if (m_core) m_core->logger()->info("版本管理插件已关闭");
    m_core = nullptr;
}

bool VersionManagerPlugin::execute(QWidget* parent, const QVariantMap& context)
{
    qint64 reportId = context.value("reportId", 0).toLongLong();
    VersionHistoryDialog dialog(reportId, parent);
    dialog.exec();
    return true;
}

bool VersionManagerPlugin::isAvailable(const QVariantMap& context) const
{
    return context.value("reportId", 0).toLongLong() > 0;
}
