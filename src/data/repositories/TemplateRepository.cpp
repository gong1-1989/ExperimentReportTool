/**
 * @file TemplateRepository.cpp
 * @brief 模板仓储类实现文件
 *
 * 本文件实现了 TemplateRepository 类，负责报告模板的持久化操作。
 * 模板定义了报告的预设结构，包括标题块、段落块、数据块等内容布局。
 *
 * 主要功能：
 * - 模板的 CRUD（创建、读取、更新、删除）操作
 * - 按分类查询模板
 * - 区分内置模板和自定义模板
 * - 获取所有模板分类列表
 * - 模板数量统计
 *
 * 设计说明：
 * - 内置模板（is_builtin = 1）由系统预置，用户不可删除
 * - 自定义模板（is_builtin = 0）由用户创建，可自由管理
 * - 模板结构以 JSON 字符串形式存储在 structure 字段中
 * - 查询结果默认按分类和名称排序，便于显示
 */

#include "TemplateRepository.h"
#include "data/database/DatabaseManager.h"
#include "core/utils/Logger.h"

#include <QSqlQuery>
#include <QSqlError>

// ===========================================================================
// 内部辅助方法
// ===========================================================================

/**
 * @brief 将数据库查询结果映射为 Template 对象
 *
 * 从 QSqlQuery 的当前行读取各字段值，构建并填充 Template 对象。
 * 模板结构从 JSON 字符串反序列化。
 *
 * @param query 已执行的数据库查询对象，当前行包含模板数据
 * @return Template::Ptr 填充好的模板智能指针
 *
 * @note 调用前需确保 query.next() 已返回 true
 * @note structure 字段存储为 JSON 字符串，需调用 structureFromJson 反序列化
 */
Template::Ptr TemplateRepository::mapToTemplate(const QSqlQuery& query)
{
    // 创建空的模板对象
    Template::Ptr temp = Template::create();

    // 映射基本字段
    temp->setId(query.value("id").toLongLong());              // 模板唯一 ID
    temp->setName(query.value("name").toString());             // 模板名称
    temp->setCategory(query.value("category").toString());     // 模板分类（如"物理"、"化学"）
    temp->setDescription(query.value("description").toString()); // 模板描述
    temp->setBuiltin(query.value("is_builtin").toBool());      // 是否为内置模板
    temp->setCreatedAt(query.value("created_at").toDateTime());   // 创建时间
    temp->setUpdatedAt(query.value("updated_at").toDateTime());   // 最后更新时间

    // 映射 JSON 字段：模板结构
    // 数据库中存储为 JSON 字符串，描述模板包含的内容块布局
    temp->structureFromJson(query.value("structure").toString());

    return temp;
}

// ===========================================================================
// 查询操作
// ===========================================================================

/**
 * @brief 根据 ID 查找模板
 *
 * @param id 模板唯一 ID，必须大于 0
 * @return Template::Ptr 找到时返回模板对象，未找到或参数无效时返回 nullptr
 *
 * 使用场景：
 * - 创建新报告时加载指定模板
 * - 编辑模板前加载完整数据
 */
Template::Ptr TemplateRepository::findById(qint64 id)
{
    // 参数校验：无效 ID 直接返回
    if (id <= 0) return nullptr;

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 使用预处理语句查询
    query.prepare("SELECT * FROM templates WHERE id = :id;");
    query.bindValue(":id", id);

    // 执行查询并检查结果
    if (query.exec() && query.next()) {
        return mapToTemplate(query);
    }

    return nullptr;
}

/**
 * @brief 查询所有模板
 *
 * @return Template::List 模板列表，按内置优先、分类、名称排序
 *
 * 排序规则：
 * 1. 内置模板排在前面（is_builtin DESC）
 * 2. 同类型按分类排序（category）
 * 3. 同分类按名称排序（name）
 */
Template::List TemplateRepository::findAll()
{
    Template::List result;

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 静态 SQL（无参数），直接执行
    query.exec("SELECT * FROM templates ORDER BY is_builtin DESC, category, name;");

    // 遍历所有结果行
    while (query.next()) {
        result.append(mapToTemplate(query));
    }

    return result;
}

/**
 * @brief 按分类查询模板
 *
 * @param category 模板分类名称
 * @return Template::List 该分类下的模板列表，按名称排序
 *
 * 使用场景：
 * - 在模板选择对话框中按分类筛选
 * - 按学科分类展示模板
 */
Template::List TemplateRepository::findByCategory(const QString& category)
{
    Template::List result;

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 按分类查询，按名称排序
    query.prepare("SELECT * FROM templates WHERE category = :category ORDER BY name;");
    query.bindValue(":category", category);

    if (query.exec()) {
        while (query.next()) {
            result.append(mapToTemplate(query));
        }
    }

    return result;
}

/**
 * @brief 查询所有内置模板
 *
 * @return Template::List 内置模板列表，按分类和名称排序
 *
 * 内置模板由系统预置，通常包含常见实验类型的标准结构。
 * 用户不能删除或修改内置模板（可另存为自定义模板）。
 */
Template::List TemplateRepository::findBuiltin()
{
    Template::List result;

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // is_builtin = 1 表示内置模板
    query.exec("SELECT * FROM templates WHERE is_builtin = 1 ORDER BY category, name;");

    while (query.next()) {
        result.append(mapToTemplate(query));
    }

    return result;
}

/**
 * @brief 查询所有用户自定义模板
 *
 * @return Template::List 自定义模板列表，按分类和名称排序
 *
 * 自定义模板由用户创建或从内置模板另存而来，用户可自由编辑和删除。
 */
Template::List TemplateRepository::findCustom()
{
    Template::List result;

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // is_builtin = 0 表示自定义模板
    query.exec("SELECT * FROM templates WHERE is_builtin = 0 ORDER BY category, name;");

    while (query.next()) {
        result.append(mapToTemplate(query));
    }

    return result;
}

/**
 * @brief 获取所有模板分类名称
 *
 * @return QStringList 去重后的分类名称列表，按字母排序
 *
 * 使用场景：
 * - 填充模板分类下拉框
 * - 在模板管理界面显示分类筛选器
 *
 * @note 使用 DISTINCT 关键字去重
 */
QStringList TemplateRepository::allCategories()
{
    QStringList categories;

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 查询所有不重复的分类名称
    query.exec("SELECT DISTINCT category FROM templates ORDER BY category;");

    while (query.next()) {
        categories.append(query.value(0).toString());
    }

    return categories;
}

// ===========================================================================
// 写入操作
// ===========================================================================

/**
 * @brief 插入新模板到数据库
 *
 * @param temp 要插入的模板对象（智能指针）
 * @return bool 插入成功返回 true，失败返回 false
 *
 * 执行流程：
 * 1. 校验指针非空
 * 2. 执行 INSERT 语句
 * 3. 获取数据库自动生成的 ID 并回写到对象
 * 4. 设置创建时间和更新时间
 *
 * @note 插入成功后，temp 对象的 id、createdAt、updatedAt 会被更新
 * @note is_builtin 字段存储为整数（1/0），因为 SQLite 没有原生布尔类型
 */
bool TemplateRepository::insert(Template::Ptr temp)
{
    // 空指针检查
    if (!temp) return false;

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 准备插入语句
    query.prepare(R"(
        INSERT INTO templates (name, category, description, structure, is_builtin, created_at, updated_at)
        VALUES (:name, :category, :description, :structure, :is_builtin, :created_at, :updated_at);
    )");

    // 绑定参数值
    const QDateTime now = QDateTime::currentDateTime();
    query.bindValue(":name", temp->name());                    // 模板名称
    query.bindValue(":category", temp->category());            // 模板分类
    query.bindValue(":description", temp->description());      // 模板描述
    query.bindValue(":structure", temp->structureToJson());    // 模板结构（JSON 字符串）
    query.bindValue(":is_builtin", temp->isBuiltin() ? 1 : 0); // 是否内置（SQLite 用整数表示布尔）
    query.bindValue(":created_at", now);                       // 创建时间
    query.bindValue(":updated_at", now);                       // 更新时间

    // 执行插入
    if (!BaseRepository::execChecked(query, "insert ")) return false;

    // 获取自增 ID 并回写到对象
    temp->setId(query.lastInsertId().toLongLong());
    temp->setCreatedAt(now);
    temp->setUpdatedAt(now);

    return true;
}

/**
 * @brief 更新已存在的模板
 *
 * @param temp 要更新的模板对象（智能指针引用）
 * @return bool 更新成功返回 true，失败返回 false
 *
 * @note 只更新可变字段：名称、分类、描述、结构、更新时间
 * @note is_builtin 字段不可通过此方法修改，防止误操作将内置模板改为自定义
 * @note 未持久化的对象（id <= 0）会被拒绝，应使用 insert 方法
 */
bool TemplateRepository::update(const Template::Ptr& temp)
{
    // 校验：指针非空且已持久化
    if (!temp || !temp->isPersisted()) return false;

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 准备更新语句（不更新 is_builtin 字段）
    query.prepare(R"(
        UPDATE templates
        SET name = :name, category = :category, description = :description,
            structure = :structure, updated_at = :updated_at
        WHERE id = :id;
    )");

    // 绑定参数值
    query.bindValue(":name", temp->name());
    query.bindValue(":category", temp->category());
    query.bindValue(":description", temp->description());
    query.bindValue(":structure", temp->structureToJson());
    query.bindValue(":updated_at", QDateTime::currentDateTime());
    query.bindValue(":id", temp->id());

    // 执行更新
    if (!BaseRepository::execChecked(query, "update ")) return false;

    // 同步更新对象的时间戳
    temp->setUpdatedAt(QDateTime::currentDateTime());
    return true;
}

/**
 * @brief 删除模板
 *
 * @param id 要删除的模板 ID
 * @return bool 删除成功返回 true，失败返回 false
 *
 * 安全机制：
 * - 内置模板不可删除，会记录警告并返回 false
 * - id <= 0 时直接返回 false
 *
 * @warning 此操作不可恢复，删除后模板将永久丢失
 * @note 删除模板不会影响已使用该模板创建的报告
 */
bool TemplateRepository::remove(qint64 id)
{
    // 参数校验
    if (id <= 0) return false;

    // 安全检查：内置模板不可删除
    // 先查询模板信息，检查是否为内置模板
    Template::Ptr temp = findById(id);
    if (temp && temp->isBuiltin()) {
        LOG_WARNING("内置模板不可删除");
        return false;
    }

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 准备删除语句
    query.prepare("DELETE FROM templates WHERE id = :id;");
    query.bindValue(":id", id);

    // 执行删除并返回结果
    if (!query.exec()) {
        LOG_ERROR(QString("删除模板失败: id=%1, %2").arg(id).arg(query.lastError().text()));
        return false;
    }
    return true;
}

// ===========================================================================
// 统计操作
// ===========================================================================

/**
 * @brief 统计模板总数
 *
 * @return int 模板总数（包括内置和自定义），查询失败时返回 0
 *
 * 使用场景：
 * - 在模板管理界面显示模板总数
 * - 系统状态统计
 */
int TemplateRepository::count()
{
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 使用 COUNT(*) 聚合查询
    query.exec("SELECT COUNT(*) FROM templates;");

    // 读取第一行第一列的值
    return query.next() ? query.value(0).toInt() : 0;
}
