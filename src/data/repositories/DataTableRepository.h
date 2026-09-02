/**
 * @file DataTableRepository.h
 * @brief 数据表仓储类头文件
 */

#ifndef DATA_TABLE_REPOSITORY_H
#define DATA_TABLE_REPOSITORY_H

#include "core/models/DataTable.h"
#include <QSqlQuery>

class DataTableRepository
{
public:
    static DataTable::Ptr findById(qint64 id);
    static DataTable::List findByReport(qint64 reportId);
    static bool insert(DataTable::Ptr table);
    static bool update(const DataTable::Ptr& table);
    static bool remove(qint64 id);
    static int countByReport(qint64 reportId);

private:
    static DataTable::Ptr mapToDataTable(const QSqlQuery& query);
};

#endif // DATA_TABLE_REPOSITORY_H
