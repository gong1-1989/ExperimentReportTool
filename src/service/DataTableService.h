/**
 * @file DataTableService.h
 * @brief 数据表业务逻辑服务层
 */

#ifndef DATA_TABLE_SERVICE_H
#define DATA_TABLE_SERVICE_H

#include "core/models/DataTable.h"

class DataTableService
{
public:
    static DataTable::Ptr getById(qint64 id);
    static DataTable::List findGlobal();
    static DataTable::List findByReport(qint64 reportId);
    static DataTable::Ptr create(const QString& name);
    static bool save(const DataTable::Ptr& table);
    static bool update(const DataTable::Ptr& table);
    static bool remove(qint64 id);

private:
    DataTableService() = delete;
    ~DataTableService() = delete;
};

#endif // DATA_TABLE_SERVICE_H
