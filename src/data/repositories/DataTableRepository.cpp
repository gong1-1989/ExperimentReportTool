/**
 * @file DataTableRepository.cpp
 * @brief 数据表仓储类实现文件
 *
 * 本文件实现了 DataTableRepository 类，负责数据表实体的持久化操作。
 * 数据表用于存储实验中记录的原始数据，支持动态行列和多种数据类型。
 *
 * 主要功能：
 * - 数据表的 CRUD（创建、读取、更新、删除）操作
 * - 按报告 ID 查询数据表列表
 * - 数据表列和行数据的 JSON 序列化/反序列化
 * - 数据表数量统计
 *
 * 设计说明：
 * - 采用 Repository 模式，将数据访问逻辑与业务逻辑分离
 * - 所有 SQL 操作使用预处理语句（prepare + bindValue），防止 SQL 注入
 * - 列定义和行数据以 JSON 字符串形式存储在数据库中
 * - 使用 QSharedPointer 管理数据表对象的生命周期
 */

#include "DataTableRepository.h"
#include "data/database/DatabaseManager.h"
#include "core/utils/Logger.h"

#include <QSqlQuery>
#include <QSqlError>

// ===========================================================================
// 内部辅助方法
// ===========================================================================

/**
 * @brief 将数据库查询结果映射为 DataTable 对象
 *
 * 从 QSqlQuery 的当前行读取各字段值，构建并填充 DataTable 对象。
 * 列定义和行数据从 JSON 字符串反序列化。
 *
 * @param query 已执行的数据库查询对象，当前行包含数据表数据
 * @return DataTable::Ptr 填充好的数据表智能指针
 *
 * @note 调用前需确保 query.next() 已返回 true，即当前行有效
 * @note columns 和 rows 字段存储为 JSON 字符串，需调用对应方法反序列化
 */
DataTable::Ptr DataTableRepository::mapToDataTable(const QSqlQuery& query)
{
    // 创建空的数据表对象
    DataTable::Ptr table = DataTable::create();

    // 映射基本字段
    table->setId(query.value("id").toLongLong());           // 数据表唯一 ID
    table->setReportId(query.value("report_id").toLongLong()); // 所属报告 ID
    table->setName(query.value("name").toString());          // 数据表名称
    table->setCreatedAt(query.value("created_at").toDateTime()); // 创建时间
    table->setUpdatedAt(query.value("updated_at").toDateTime()); // 最后更新时间

    // 映射 JSON 字段：列定义和行数据
    // 数据库中存储为 JSON 字符串，需要反序列化为内存中的数据结构
    table->columnsFromJson(query.value("columns").toString());
    table->rowsFromJson(query.value("rows").toString());

    return table;
}

// ===========================================================================
// 查询操作
// ===========================================================================

/**
 * @brief 根据 ID 查找数据表
 *
 * @param id 数据表唯一 ID，必须大于 0
 * @return DataTable::Ptr 找到时返回数据表对象，未找到或参数无效时返回 nullptr
 *
 * 使用场景：
 * - 编辑数据表前加载完整数据
 * - 验证数据表是否存在
 *
 * 错误处理：
 * - id <= 0 时直接返回 nullptr
 * - 数据库查询失败时记录错误日志并返回 nullptr
 */
DataTable::Ptr DataTableRepository::findById(qint64 id)
{
    // 参数校验：无效 ID 直接返回，避免无效查询
    if (id <= 0) return nullptr;

    // 获取数据库连接
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    // 使用预处理语句查询，防止 SQL 注入
    query.prepare("SELECT * FROM data_tables WHERE id = :id;");
    query.bindValue(":id", id);

    // 执行查询并检查是否有结果
    if (query.exec() && query.next()) {
        return mapToDataTable(query);
    }

    // 未找到或查询失败
    return nullptr;
}

/**
 * @brief 根据报告 ID 查询所有数据表
 *
 * @param reportId 报告 ID
 * @return DataTable::List 数据表列表，按创建时间升序排列
 *
 * 使用场景：
 * - 在报告编辑器中显示该报告的所有数据表
 * - 导出报告时收集所有关联数据表
 *
 * @note 即使 reportId 无效也会返回空列表，不会报错
 */
DataTable::List DataTableRepository::findByReport(qint64 reportId)
{
    DataTable::List result;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    // 按创建时间升序排列，保证显示顺序稳定
    query.prepare("SELECT * FROM data_tables WHERE report_id = :reportId ORDER BY created_at;");
    query.bindValue(":reportId", reportId);

    // 执行查询并遍历所有结果行
    if (query.exec()) {
        while (query.next()) {
            result.append(mapToDataTable(query));
        }
    }

    return result;
}

DataTable::List DataTableRepository::findGlobal()
{
    DataTable::List result;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    // 全局数据表的 report_id 为 0
    query.prepare("SELECT * FROM data_tables WHERE report_id = 0 ORDER BY created_at;");

    if (query.exec()) {
        while (query.next()) {
            result.append(mapToDataTable(query));
        }
    }

    return result;
}

// ===========================================================================
// 写入操作
// ===========================================================================

/**
 * @brief 插入新数据表到数据库
 *
 * @param table 要插入的数据表对象（智能指针）
 * @return bool 插入成功返回 true，失败返回 false
 *
 * 执行流程：
 * 1. 校验指针非空
 * 2. 执行 INSERT 语句
 * 3. 获取数据库自动生成的 ID 并回写到对象
 * 4. 设置创建时间和更新时间
 *
 * @note 插入成功后，table 对象的 id、createdAt、updatedAt 会被更新
 * @note 列定义和行数据会被序列化为 JSON 字符串存储
 */
bool DataTableRepository::insert(DataTable::Ptr table)
{
    // 空指针检查
    if (!table) return false;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    // 准备插入语句，使用 R"(...)" 原始字符串字面量避免转义
    query.prepare(R"(
        INSERT INTO data_tables (report_id, name, columns, rows, created_at, updated_at)
        VALUES (:report_id, :name, :columns, :rows, :created_at, :updated_at);
    )");

    // 绑定参数值
    const QDateTime now = QDateTime::currentDateTime();
    query.bindValue(":report_id", table->reportId());          // 所属报告 ID
    query.bindValue(":name", table->name());                    // 数据表名称
    query.bindValue(":columns", table->columnsToJson());        // 列定义（JSON 字符串）
    query.bindValue(":rows", table->rowsToJson());              // 行数据（JSON 字符串）
    query.bindValue(":created_at", now);                        // 创建时间
    query.bindValue(":updated_at", now);                        // 更新时间

    // 执行插入并检查结果
    if (!query.exec()) {
        LOG_ERROR(QString("insert 失败: %1\nSQL: %2").arg(query.lastError().text(), query.lastQuery()));
        return false;
    }

    // 获取数据库自动生成的自增 ID，回写到对象
    table->setId(query.lastInsertId().toLongLong());
    table->setCreatedAt(now);
    table->setUpdatedAt(now);

    return true;
}

/**
 * @brief 更新已存在的数据表
 *
 * @param table 要更新的数据表对象（智能指针引用）
 * @return bool 更新成功返回 true，失败返回 false
 *
 * 执行流程：
 * 1. 校验指针非空且对象已持久化（id > 0）
 * 2. 执行 UPDATE 语句，更新名称、列定义、行数据和更新时间
 * 3. 更新对象的 updatedAt 字段
 *
 * @note 只更新可变字段，id、reportId、createdAt 不可修改
 * @note 未持久化的对象（id <= 0）会被拒绝，应使用 insert 方法
 */
bool DataTableRepository::update(const DataTable::Ptr& table)
{
    // 校验：指针非空且已持久化
    if (!table || !table->isPersisted()) return false;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    // 准备更新语句
    query.prepare(R"(
        UPDATE data_tables
        SET name = :name, columns = :columns, rows = :rows, updated_at = :updated_at
        WHERE id = :id;
    )");

    // 绑定参数值
    query.bindValue(":name", table->name());
    query.bindValue(":columns", table->columnsToJson());
    query.bindValue(":rows", table->rowsToJson());
    query.bindValue(":updated_at", QDateTime::currentDateTime());
    query.bindValue(":id", table->id());

    // 执行更新
    if (!query.exec()) {
        LOG_ERROR(QString("update 失败: %1\nSQL: %2").arg(query.lastError().text(), query.lastQuery()));
        return false;
    }

    // 同步更新对象的时间戳
    table->setUpdatedAt(QDateTime::currentDateTime());
    return true;
}

/**
 * @brief 删除数据表
 *
 * @param id 要删除的数据表 ID
 * @return bool 删除成功返回 true，失败返回 false
 *
 * @warning 此操作不可恢复，删除后数据将永久丢失
 * @note id <= 0 时直接返回 false
 * @note 目前未级联删除关联的图表，调用方需自行处理
 */
bool DataTableRepository::remove(qint64 id)
{
    // 参数校验
    if (id <= 0) return false;

    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    // 准备删除语句
    query.prepare("DELETE FROM data_tables WHERE id = :id;");
    query.bindValue(":id", id);

    // 执行删除并返回结果
    return query.exec();
}

// ===========================================================================
// 统计操作
// ===========================================================================

/**
 * @brief 统计指定报告下的数据表数量
 *
 * @param reportId 报告 ID
 * @return int 数据表数量，查询失败时返回 0
 *
 * 使用场景：
 * - 在项目树或报告列表中显示数据表数量徽标
 * - 验证报告是否包含数据表
 */
int DataTableRepository::countByReport(qint64 reportId)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    // 使用 COUNT(*) 聚合查询
    query.prepare("SELECT COUNT(*) FROM data_tables WHERE report_id = :reportId;");
    query.bindValue(":reportId", reportId);

    // 执行查询并读取第一行第一列的值
    // 使用三元运算符简化：执行成功且有结果时返回计数值，否则返回 0
    return query.exec() && query.next() ? query.value(0).toInt() : 0;
}
