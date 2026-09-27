/**
 * @file TemplateManagerPlugin.cpp
 * @brief 模板管理插件实现文件
 */

#include "TemplateManagerPlugin.h"
#include "core/plugin/CoreService.h"
#include "ui/dialogs/TemplateEditorDialog.h"
#include "core/utils/Logger.h"

TemplateManagerPlugin::TemplateManagerPlugin(QObject* parent)
    : QObject(parent)
    , m_core(nullptr)
{
}

bool TemplateManagerPlugin::initialize(CoreService* core)
{
    m_core = core;
    if (m_core) m_core->logger()->info("模板管理插件已初始化");
    return true;
}

void TemplateManagerPlugin::shutdown()
{
    if (m_core) m_core->logger()->info("模板管理插件已关闭");
    m_core = nullptr;
}

bool TemplateManagerPlugin::execute(QWidget* parent, const QVariantMap& context)
{
    Q_UNUSED(context);
    TemplateEditorDialog dialog(parent);
    dialog.exec();
    return true;
}
