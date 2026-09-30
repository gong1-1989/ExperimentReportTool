/**
 * @file ImportRegistry.cpp
 * @brief 导入适配器注册表实现
 */

#include "import/ImportRegistry.h"

#include "import/CsvImportAdapter.h"

#include <QFileInfo>

void ImportRegistry::registerAdapter(ImportAdapter* adapter)
{
    if (!adapter) return;
    adapters().append(adapter);
}

ImportAdapter* ImportRegistry::adapterForFile(const QString& filePath)
{
    ensureBuiltinAdapters();
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    const QList<ImportAdapter*> list = adapters();
    for (ImportAdapter* a : list) {
        if (a->extensions().contains(suffix)) return a;
    }
    return nullptr;
}

QList<ImportAdapter*> ImportRegistry::allAdapters()
{
    ensureBuiltinAdapters();
    return adapters();
}

QString ImportRegistry::combinedFileFilter()
{
    ensureBuiltinAdapters();
    const QList<ImportAdapter*> list = adapters();
    QStringList parts;
    QStringList allExt;
    for (ImportAdapter* a : list) {
        parts << a->fileFilter();
        for (const QString& e : a->extensions()) allExt << QStringLiteral("*.%1").arg(e);
    }
    if (parts.size() > 1) {
        parts.insert(0, QStringLiteral("全部支持格式 (%1)").arg(allExt.join(QStringLiteral(" "))));
    }
    return parts.join(QStringLiteral(";;"));
}

void ImportRegistry::ensureBuiltinAdapters()
{
    if (!adapters().isEmpty()) return;
    // 内置 CSV 适配器：不拆 .dll，随框架编译
    adapters().append(new CsvImportAdapter);
}

QList<ImportAdapter*>& ImportRegistry::adapters()
{
    static QList<ImportAdapter*> list;
    return list;
}
