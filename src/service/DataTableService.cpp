/**
 * @file DataTableService.cpp
 * @brief 数据表业务逻辑服务层实现
 */

#include "DataTableService.h"
#include "data/repositories/DataTableRepository.h"
#include "core/utils/Logger.h"
#include <QtGlobal>

DataTable::Ptr DataTableService::getById(qint64 id)
{
    return DataTableRepository::findById(id);
}

DataTable::List DataTableService::findGlobal()
{
    return DataTableRepository::findGlobal();
}

DataTable::List DataTableService::findByReport(qint64 reportId)
{
    return DataTableRepository::findByReport(reportId);
}

DataTable::Ptr DataTableService::create(const QString& name)
{
    DataTable::Ptr table = DataTable::create();
    table->setName(name);

    if (DataTableRepository::insert(table)) {
        return table;
    }
    LOG_ERROR(QString("创建数据表失败: name=%1").arg(name));
    return nullptr;
}

bool DataTableService::save(const DataTable::Ptr& table)
{
    Q_ASSERT(table);
    if (!table) return false;
    bool ok = false;
    if (table->id() > 0) {
        ok = DataTableRepository::update(table);
    } else {
        ok = DataTableRepository::insert(table);
    }
    if (!ok) {
        LOG_ERROR(QString("保存数据表失败: id=%1, name=%2").arg(table->id()).arg(table->name()));
    }
    return ok;
}

bool DataTableService::update(const DataTable::Ptr& table)
{
    if (!table) return false;
    return DataTableRepository::update(table);
}

bool DataTableService::remove(qint64 id)
{
    return DataTableRepository::remove(id);
}
