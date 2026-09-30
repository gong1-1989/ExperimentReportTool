#include "ExportRegistry.h"
#include "ExportAdapter.h"
#include "ExcelExportAdapter.h"
#include "JsonExportAdapter.h"
#include "MarkdownExportAdapter.h"

#include <QFileInfo>

ExportRegistry* ExportRegistry::s_instance = nullptr;

ExportRegistry& ExportRegistry::instance()
{
    if (!s_instance) s_instance = new ExportRegistry();
    return *s_instance;
}

void ExportRegistry::registerAdapter(ExportAdapter* adapter)
{
    if (!adapter || m_adapters.contains(adapter)) return;
    m_adapters.append(adapter);
}

ExportAdapter* ExportRegistry::adapterForFile(const QString& filePath) const
{
    const QString ext = QFileInfo(filePath).suffix().toLower();
    if (ext.isEmpty()) return nullptr;
    for (ExportAdapter* a : m_adapters) {
        if (a->extensions().contains(ext)) return a;
    }
    return nullptr;
}

QList<ExportAdapter*> ExportRegistry::allAdapters() const
{
    return m_adapters;
}

void ExportRegistry::ensureBuiltinAdapters()
{
    static bool done = false;
    if (done) return;
    done = true;
    // 静态局部对象：生命周期至进程结束，注册表不接管所有权
    static MarkdownExportAdapter s_md;
    static JsonExportAdapter s_json;
    static ExcelExportAdapter s_xlsx;
    registerAdapter(&s_md);
    registerAdapter(&s_json);
    registerAdapter(&s_xlsx);
}

QString ExportRegistry::combinedFileFilter() const
{
    QStringList filters;
    for (ExportAdapter* a : m_adapters) filters << a->fileFilter();
    return filters.join(QStringLiteral(";;"));
}
