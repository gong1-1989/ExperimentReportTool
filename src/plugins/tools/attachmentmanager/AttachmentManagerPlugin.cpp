/**
 * @file AttachmentManagerPlugin.cpp
 * @brief 附件管理插件实现文件
 */

#include "AttachmentManagerPlugin.h"
#include "core/plugin/CoreService.h"
#include "ui/dialogs/AttachmentManagerDialog.h"
#include "core/utils/Logger.h"

AttachmentManagerPlugin::AttachmentManagerPlugin(QObject* parent)
    : QObject(parent)
    , m_core(nullptr)
{
}

bool AttachmentManagerPlugin::initialize(CoreService* core)
{
    m_core = core;
    if (m_core) m_core->logger()->info("附件管理插件已初始化");
    return true;
}

void AttachmentManagerPlugin::shutdown()
{
    if (m_core) m_core->logger()->info("附件管理插件已关闭");
    m_core = nullptr;
}

bool AttachmentManagerPlugin::execute(QWidget* parent, const QVariantMap& context)
{
    qint64 reportId = context.value("reportId", 0).toLongLong();
    AttachmentManagerDialog dialog(reportId, parent);
    dialog.exec();
    return true;
}

bool AttachmentManagerPlugin::isAvailable(const QVariantMap& context) const
{
    return context.value("reportId", 0).toLongLong() > 0;
}
