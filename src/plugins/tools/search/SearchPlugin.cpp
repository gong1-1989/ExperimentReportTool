/**
 * @file SearchPlugin.cpp
 * @brief 搜索功能插件实现文件
 */

#include "SearchPlugin.h"
#include "core/plugin/CoreService.h"
#include "ui/dialogs/SearchResultDialog.h"
#include "core/utils/Logger.h"

SearchPlugin::SearchPlugin(QObject* parent)
    : QObject(parent)
    , m_core(nullptr)
{
}

bool SearchPlugin::initialize(CoreService* core)
{
    m_core = core;
    if (m_core) m_core->logger()->info("搜索插件已初始化");
    return true;
}

void SearchPlugin::shutdown()
{
    if (m_core) m_core->logger()->info("搜索插件已关闭");
    m_core = nullptr;
}

bool SearchPlugin::execute(QWidget* parent, const QVariantMap& context)
{
    Q_UNUSED(context);
    SearchResultDialog dialog(parent);
    dialog.exec();
    return true;
}
