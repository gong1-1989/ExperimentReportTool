/**
 * @file DataTableRepository.cpp
 * @brief 数据表仓储类实现文件
 */

#include "DataTableRepository.h"
#include "data/database/DatabaseManager.h"
#include "core/utils/Logger.h"

#include <QSqlQuery>
#include <QSqlError>

DataTable::Ptr DataTableRepository::mapToDataTable(const QSqlQuery& query)
{
    DataTable::Ptr table = DataTable::create();
    table->setId(query.value("id").toLongLong());
    table->setReportId(query.value("report_id").toLongLong());
    table->setName(query.value("name").toString());
    table->setCreatedAt(query.value("created_at").toDateTime());
    table->setUpdatedAt(query.value("updated_at").toDateTime());
    table->columnsFromJson(query.value("columns").toString());
    table->rowsFromJson(query.value("rows").toString());
    return table;
}

DataTable::Ptr DataTableRepository::findById(qint64 id)
{
    if (id <= 0) return nullptr;
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("SELECT * FROM data_tables WHERE id = :id;");
    query.bindValue(":id", id);
    if (query.exec() && query.next()) {
        return mapToDataTable(query);
    }
    return nullptr;
}

DataTable::List DataTableRepository::findByReport(qint64 reportId)
{
    DataTable::List result;
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("SELECT * FROM data_tables WHERE report_id = :reportId ORDER BY created_at;");
    query.bindValue(":reportId", reportId);
    if (query.exec()) {
        while (query.next()) {
            result.append(mapToDataTable(query));
        }
    }
    return result;
}

bool DataTableRepository::insert(DataTable::Ptr table)
{
    if (!table) return false;
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    query.prepare(R"(
        INSERT INTO data_tables (report_id, name, columns, rows, created_at, updated_at)
        VALUES (:report_id, :name, :columns, :rows, :created_at, :updated_at);
    )");

    const QDateTime now = QDateTime::currentDateTime();
    query.bindValue(":report_id", table->reportId());
    query.bindValue(":name", table->name());
    query.bindValue(":columns", table->columnsToJson());
    query.bindValue(":rows", table->rowsToJson());
    query.bindValue(":created_at", now);
    query.bindValue(":updated_at", now);

    if (!query.exec()) {
        LOG_ERROR(QString("insert 失败: %1").arg(query.lastError().text()));
        return false;
    }

    table->setId(query.lastInsertId().toLongLong());
    table->setCreatedAt(now);
    table->setUpdatedAt(now);
    return true;
}

bool DataTableRepository::update(const DataTable::Ptr& table)
{
    if (!table || !table->isPersisted()) return false;
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    query.prepare(R"(
        UPDATE data_tables
        SET name = :name, columns = :columns, rows = :rows, updated_at = :updated_at
        WHERE id = :id;
    )");

    query.bindValue(":name", table->name());
    query.bindValue(":columns", table->columnsToJson());
    query.bindValue(":rows", table->rowsToJson());
    query.bindValue(":updated_at", QDateTime::currentDateTime());
    query.bindValue(":id", table->id());

    if (!query.exec()) {
        LOG_ERROR(QString("update 失败: %1").arg(query.lastError().text()));
        return false;
    }
    table->setUpdatedAt(QDateTime::currentDateTime());
    return true;
}

bool DataTableRepository::remove(qint64 id)
{
    if (id <= 0) return false;
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("DELETE FROM data_tables WHERE id = :id;");
    query.bindValue(":id", id);
    return query.exec();
}

int DataTableRepository::countByReport(qint64 reportId)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM data_tables WHERE report_id = :reportId;");
    query.bindValue(":reportId", reportId);
    return query.exec() && query.next() ? query.value(0).toInt() : 0;
}
