#include "ExtensionManager.h"

ExtensionManager* ExtensionManager::s_instance = nullptr;

ExtensionManager& ExtensionManager::instance()
{
    if (!s_instance) s_instance = new ExtensionManager();
    return *s_instance;
}
