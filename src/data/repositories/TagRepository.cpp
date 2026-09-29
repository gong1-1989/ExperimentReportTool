/**
 * @file TagRepository.cpp
 * @brief 标签数据访问层实现文件
 *
 * 本文件实现了 TagRepository 类，负责标签实体和报告-标签关联的持久化操作。
 * 标签用于对实验报告进行分类和筛选，支持多对多关系。
 *
 * 主要功能：
 * - 标签的 CRUD（创建、读取、更新、删除）操作
 * - 标签搜索（按名称模糊匹配）
 * - 报告-标签关联管理（添加、移除、批量设置）
 * - 按标签名称自动创建新标签
 * - 标签存在性检查
 *
 * 设计说明：
 * - 采用 Repository 模式，将数据访问逻辑与业务逻辑分离
 * - 标签和报告是多对多关系，通过 report_tags 关联表实现
 * - 标签使用次数通过 LEFT JOIN + COUNT 动态计算，不冗余存储
 * - 删除标签时使用事务保证关联表和标签表的原子性
 * - 标签名称唯一（通过 exists() 检查）
 * - 使用 QSharedPointer 管理标签对象的生命周期
 *
 * 数据库表结构：
 * - tags：标签主表（id, name, color, description, created_at）
 * - report_tags：报告-标签关联表（report_id, tag_id）
 */

#include "TagRepository.h"
#include "data/database/DatabaseManager.h"
#include "core/utils/Logger.h"

#include <QSet>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

// ===========================================================================
// 标签 CRUD 操作
// ===========================================================================

/**
 * @brief 根据 ID 查找标签
 *
 * @param tagId 标签唯一 ID
 * @return Tag::Ptr 找到时返回标签对象，未找到或查询失败时返回空指针
 *
 * 使用场景：
 * - 编辑标签前加载完整信息
 * - 验证标签是否存在
 *
 * @note 查询失败和未找到都返回空指针，调用方无法区分
 */
Tag::Ptr TagRepository::findById(qint64 tagId)
{
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 使用预处理语句查询
    query.prepare("SELECT * FROM tags WHERE id = :id;");
    query.bindValue(":id", tagId);

    // 执行查询并检查是否有结果
    // 使用短路求值：exec() 失败或 next() 无结果都返回空指针
    if (!query.exec() || !query.next()) {
        return Tag::Ptr();
    }

    // 从查询结果创建标签对象
    return createFromQuery(query);
}

/**
 * @brief 根据名称查找标签
 *
 * @param name 标签名称（精确匹配）
 * @return Tag::Ptr 找到时返回标签对象，未找到或查询失败时返回空指针
 *
 * 使用场景：
 * - 按名称设置标签时查找现有标签
 * - 检查标签是否已存在
 *
 * @note 名称匹配是精确匹配，区分大小写
 * @note 如果需要模糊搜索，请使用 search() 方法
 */
Tag::Ptr TagRepository::findByName(const QString& name)
{
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 使用预处理语句查询
    query.prepare("SELECT * FROM tags WHERE name = :name;");
    query.bindValue(":name", name);

    // 执行查询并检查结果
    if (!query.exec() || !query.next()) {
        return Tag::Ptr();
    }

    return createFromQuery(query);
}

/**
 * @brief 查询所有标签
 *
 * 返回所有标签，并附带每个标签的使用次数（关联的报告数量）。
 * 结果按使用次数降序、名称升序排列。
 *
 * @return Tag::List 标签列表，按名称排序
 *
 * SQL 说明：
 * - 使用 LIKE %keyword% 模糊匹配标签名称
 * - 使用 ORDER BY t.name ASC 排序
 *
 * 使用场景：
 * - 标签管理界面显示所有标签
 * - 标签选择下拉框
 */
Tag::List TagRepository::findAll()
{
    Tag::List tags;
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 查询所有标签（不再统计使用次数）
    query.exec(R"(
        SELECT t.*
        FROM tags t
        ORDER BY t.name ASC;
    )");

    // 遍历所有结果行
    while (query.next()) {
        Tag::Ptr tag = createFromQuery(query);
        tags.append(tag);
    }

    return tags;
}

/**
 * @brief 按关键词搜索标签
 *
 * @param keyword 搜索关键词（模糊匹配标签名称）
 * @return Tag::List 匹配的标签列表，按使用次数和名称排序
 *
 * 使用场景：
 * - 标签搜索框实时搜索
 * - 标签选择器的筛选功能
 *
 * @note 关键词为空时返回所有标签（调用 findAll()）
 * @note 使用 LIKE %keyword% 进行模糊匹配
 */
Tag::List TagRepository::search(const QString& keyword)
{
    Tag::List tags;

    // 关键词为空时返回所有标签
    if (keyword.isEmpty()) return findAll();

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 模糊搜索标签名称
    query.prepare(R"(
        SELECT t.*
        FROM tags t
        WHERE t.name LIKE :keyword
        ORDER BY t.name ASC;
    )");
    // 使用 %keyword% 进行前后模糊匹配
    query.bindValue(":keyword", "%" + keyword + "%");

    // 执行查询并遍历结果
    if (query.exec()) {
        while (query.next()) {
            Tag::Ptr tag = createFromQuery(query);
            tags.append(tag);
        }
    }

    return tags;
}

/**
 * @brief 保存标签（新建或更新）
 *
 * 根据标签对象的状态自动判断是新建还是更新：
 * - isNew() 返回 true（id <= 0）：执行 INSERT
 * - isNew() 返回 false（id > 0）：执行 UPDATE
 *
 * @param tag 要保存的标签对象（智能指针）
 * @return bool 保存成功返回 true，失败返回 false
 *
 * @note 新建成功后，tag 对象的 id 会被更新为数据库生成的自增 ID
 * @note 空指针直接返回 false
 * @note 此方法不检查标签名称是否重复，调用方应先调用 exists() 检查
 */
bool TagRepository::save(Tag::Ptr tag)
{
    // 空指针检查
    if (!tag) return false;

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    if (tag->isNew()) {
        // ================================================================
        // 新建标签
        // ================================================================
        query.prepare(R"(
            INSERT INTO tags (name, color, description, created_at)
            VALUES (:name, :color, :description, :created_at);
        )");
        query.bindValue(":name", tag->name());
        query.bindValue(":color", tag->color());
        query.bindValue(":description", tag->description());
        query.bindValue(":created_at", tag->createdAt());

        // 执行插入
        if (!BaseRepository::execChecked(query, "创建标签")) return false;

        // 获取自增 ID 并回写到对象
        tag->setId(query.lastInsertId().toLongLong());
    } else {
        // ================================================================
        // 更新标签
        // ================================================================
        query.prepare(R"(
            UPDATE tags SET name = :name, color = :color,
                   description = :description WHERE id = :id;
        )");
        query.bindValue(":name", tag->name());
        query.bindValue(":color", tag->color());
        query.bindValue(":description", tag->description());
        query.bindValue(":id", tag->id());

        // 执行更新
        if (!BaseRepository::execChecked(query, "更新标签")) return false;
    }

    return true;
}

/**
 * @brief 删除标签
 *
 * 删除标签时会同时删除该标签与所有报告的关联关系。
 * 使用事务保证操作的原子性，避免数据不一致。
 *
 * @param tagId 要删除的标签 ID
 * @return bool 删除成功返回 true，失败返回 false
 *
 * 执行流程：
 * 1. 校验 tagId > 0
 * 2. 开启事务
 * 3. 删除 report_tags 表中该标签的所有关联记录
 * 4. 删除 tags 表中的标签记录
 * 5. 提交事务
 *
 * @warning 此操作不可恢复，删除后标签及其所有关联将永久丢失
 * @note tagId <= 0 时直接返回 false
 * @note 返回 numRowsAffected > 0，表示标签确实存在且已删除
 * @note 任何一步失败都会回滚事务，保证数据一致性
 */
bool TagRepository::remove(qint64 tagId)
{
    // 参数校验
    if (tagId <= 0) return false;

    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 使用事务保证删除操作的原子性
    // 要么关联表和标签都删除成功，要么都不删除，避免数据不一致
    if (!db.transaction()) {
        LOG_ERROR("开启事务失败");
        return false;
    }

    // 第一步：删除报告-标签关联表记录
    query.prepare("DELETE FROM report_tags WHERE tag_id = :tagId;");
    query.bindValue(":tagId", tagId);
    if (!query.exec()) {
        LOG_ERROR(QString("删除标签关联失败: %1\nSQL: %2").arg(query.lastError().text(), query.lastQuery()));
        db.rollback();
        return false;
    }

    // 第二步：删除标签本身
    query.prepare("DELETE FROM tags WHERE id = :id;");
    query.bindValue(":id", tagId);
    if (!query.exec()) {
        LOG_ERROR(QString("删除标签失败: %1\nSQL: %2").arg(query.lastError().text(), query.lastQuery()));
        db.rollback();
        return false;
    }

    // 提交事务
    if (!db.commit()) {
        LOG_ERROR("提交事务失败");
        db.rollback();
        return false;
    }

    // 返回是否实际删除了记录（numRowsAffected > 0 表示标签存在且已删除）
    return query.numRowsAffected() > 0;
}

/**
 * @brief 检查标签名称是否已存在
 *
 * @param name 标签名称
 * @param excludeId 排除的标签 ID（用于编辑时排除自身，0 表示不排除）
 * @return bool 名称已存在返回 true，不存在返回 false
 *
 * 使用场景：
 * - 新建标签前检查名称是否重复
 * - 编辑标签时检查新名称是否与其他标签冲突
 *
 * @note excludeId > 0 时，查询条件会加上 AND id != :id
 * @note 名称匹配是精确匹配，区分大小写
 */
bool TagRepository::exists(const QString& name, qint64 excludeId)
{
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 根据是否需要排除自身构建不同的 SQL
    if (excludeId > 0) {
        // 编辑场景：排除当前标签自身
        query.prepare("SELECT COUNT(*) FROM tags WHERE name = :name AND id != :id;");
        query.bindValue(":id", excludeId);
    } else {
        // 新建场景：直接检查名称是否存在
        query.prepare("SELECT COUNT(*) FROM tags WHERE name = :name;");
    }
    query.bindValue(":name", name);

    // 执行查询，COUNT(*) > 0 表示已存在
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

// ===========================================================================
// 报告-标签关联管理
// ===========================================================================

/**
 * @brief 查询指定报告的所有标签
 *
 * @param reportId 报告 ID
 * @return Tag::List 该报告的标签列表，按名称升序排列
 *
 * 使用场景：
 * - 报告编辑器中显示已添加的标签
 * - 报告详情页显示标签
 *
 * SQL 说明：
 * - 使用 JOIN（内连接）只返回有关联的标签
 * - 按标签名称升序排列
 */
Tag::List TagRepository::findByReport(qint64 reportId)
{
    Tag::List tags;
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 通过关联表查询报告的标签
    query.prepare(R"(
        SELECT t.* FROM tags t
        JOIN report_tags rt ON rt.tag_id = t.id
        WHERE rt.report_id = :reportId
        ORDER BY t.name ASC;
    )");
    query.bindValue(":reportId", reportId);

    // 执行查询并遍历结果
    if (query.exec()) {
        while (query.next()) {
            tags.append(createFromQuery(query));
        }
    }

    return tags;
}

/**
 * @brief 查询使用指定标签的所有报告 ID
 *
 * @param tagId 标签 ID
 * @return QList<qint64> 报告 ID 列表
 *
 * 使用场景：
 * - 按标签筛选报告列表
 * - 标签详情页显示关联的报告
 */
QList<qint64> TagRepository::findReportIdsByTag(qint64 tagId)
{
    QList<qint64> reportIds;
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 查询关联表中该标签的所有报告 ID
    query.prepare("SELECT report_id FROM report_tags WHERE tag_id = :tagId;");
    query.bindValue(":tagId", tagId);

    // 执行查询并遍历结果
    if (query.exec()) {
        while (query.next()) {
            reportIds.append(query.value("report_id").toLongLong());
        }
    }

    return reportIds;
}

/**
 * @brief 给报告添加标签
 *
 * @param reportId 报告 ID
 * @param tagId 标签 ID
 * @return bool 添加成功返回 true，失败返回 false
 *
 * 幂等性：
 * - 如果该报告已经关联了该标签，直接返回 true（不重复添加）
 * - 先查询是否已存在，再决定是否插入
 *
 */
bool TagRepository::addToReport(qint64 reportId, qint64 tagId)
{
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 检查是否已存在关联（幂等性保证）
    query.prepare("SELECT COUNT(*) FROM report_tags WHERE report_id = :reportId AND tag_id = :tagId;");
    query.bindValue(":reportId", reportId);
    query.bindValue(":tagId", tagId);
    if (query.exec() && query.next() && query.value(0).toInt() > 0) {
        return true;  // 已存在，直接返回成功
    }

    // 插入新的关联记录
    query.prepare("INSERT INTO report_tags (report_id, tag_id) VALUES (:reportId, :tagId);");
    query.bindValue(":reportId", reportId);
    query.bindValue(":tagId", tagId);

    if (!BaseRepository::execChecked(query, "添加报告标签")) return false;

    return true;
}

/**
 * @brief 从报告移除标签
 *
 * @param reportId 报告 ID
 * @param tagId 标签 ID
 * @return bool 移除成功返回 true，失败返回 false
 *
 * @note 如果该报告没有关联该标签，删除 0 行也视为成功
 */
bool TagRepository::removeFromReport(qint64 reportId, qint64 tagId)
{
    QSqlDatabase db = BaseRepository::db();
    QSqlQuery query(db);

    // 删除关联记录
    query.prepare("DELETE FROM report_tags WHERE report_id = :reportId AND tag_id = :tagId;");
    query.bindValue(":reportId", reportId);
    query.bindValue(":tagId", tagId);

    if (!BaseRepository::execChecked(query, "移除报告标签")) return false;

    // 更新使用次数
    return true;
}

/**
 * @brief 批量设置报告的标签
 *
 * 先清除报告的所有现有标签，再添加新的标签列表。
 * 使用事务保证操作的原子性。
 *
 * @param reportId 报告 ID
 * @param tagIds 标签 ID 列表
 * @return bool 设置成功返回 true，失败返回 false
 *
 * 执行流程：
 * 1. 开启事务
 * 2. 删除该报告的所有现有标签关联
 * 3. 遍历新标签列表，逐个插入关联记录
 * 4. 提交事务
 *
 * @note 任何一步失败都会回滚事务，保证数据一致性
 * @note 空列表会清除报告的所有标签
 * @note 此方法会覆盖现有标签，不是追加
 */
bool TagRepository::setReportTags(qint64 reportId, const QList<qint64>& tagIds)
{
    QSqlDatabase db = BaseRepository::db();

    // 开启事务，保证清除和添加的原子性
    if (!db.transaction()) {
        LOG_ERROR(QString("设置报告标签事务启动失败: %1").arg(db.lastError().text()));
        return false;
    }

    // 第一步：清除现有标签关联
    QSqlQuery query(db);
    query.prepare("DELETE FROM report_tags WHERE report_id = :reportId;");
    query.bindValue(":reportId", reportId);
    if (!query.exec()) {
        db.rollback();
        return false;
    }

    // 第二步：添加新标签
    for (qint64 tagId : tagIds) {
        query.prepare("INSERT INTO report_tags (report_id, tag_id) VALUES (:reportId, :tagId);");
        query.bindValue(":reportId", reportId);
        query.bindValue(":tagId", tagId);
        if (!query.exec()) {
            db.rollback();
            return false;
        }
    }

    // 提交事务
    if (!db.commit()) {
        LOG_ERROR(QString("设置报告标签事务提交失败: %1").arg(db.lastError().text()));
        db.rollback();
        return false;
    }
    return true;
}

/**
 * @brief 按标签名称批量设置报告的标签
 *
 * 与 setReportTags 类似，但接受标签名称列表而不是 ID 列表。
 * 对于不存在的标签名称，会自动创建新标签。
 *
 * @param reportId 报告 ID
 * @param tagNames 标签名称列表
 * @return bool 设置成功返回 true，失败返回 false
 *
 * 执行流程：
 * 1. 遍历标签名称列表
 * 2. 对每个名称，先去除首尾空格
 * 3. 按名称查找现有标签
 * 4. 如果不存在，自动创建新标签
 * 5. 收集所有标签 ID
 * 6. 调用 setReportTags 批量设置
 *
 * 使用场景：
 * - 用户在标签输入框中输入逗号分隔的标签名称
 * - 从其他系统导入标签
 *
 * @note 空名称会被跳过
 * @note 自动创建的标签使用默认颜色和空描述
 * @note 创建失败的标签会被跳过，不影响其他标签
 */
bool TagRepository::setReportTagsByName(qint64 reportId, const QStringList& tagNames)
{
    QList<qint64> tagIds;

    // 遍历标签名称列表
    for (const QString& name : tagNames) {
        // 去除首尾空格
        const QString trimmed = name.trimmed();
        // 跳过空名称
        if (trimmed.isEmpty()) continue;

        // 按名称查找现有标签
        Tag::Ptr tag = findByName(trimmed);
        if (!tag) {
            // 标签不存在，自动创建新标签
            tag = Tag::create(trimmed);
            if (!save(tag)) {
                // 创建失败，跳过此标签
                continue;
            }
        }
        // 收集标签 ID
        tagIds.append(tag->id());
    }

    // 调用批量设置方法
    return setReportTags(reportId, tagIds);
}

/**
 * @brief 获取报告的标签名称列表
 *
 * @param reportId 报告 ID
 * @return QStringList 标签名称列表
 *
 * 使用场景：
 * - 报告导出时包含标签信息
 * - 显示标签名称字符串（如 "标签1, 标签2"）
 *
 * @note 这是 findByReport 的便捷方法，只返回名称不返回完整对象
 */
QStringList TagRepository::findReportTagNames(qint64 reportId)
{
    QStringList names;

    // 查询报告的所有标签
    const Tag::List tags = findByReport(reportId);

    // 提取标签名称
    for (const Tag::Ptr& tag : tags) {
        names.append(tag->name());
    }

    return names;
}


/**
 * @brief 批量查询多个报告的标签名（一条 SQL 完成，避免循环内逐报告查询的 N+1 问题）
 * @param reportIds 报告 ID 列表
 * @return 映射：report_id -> 标签名列表（无标签的报告不在映射中）
 */
QHash<qint64, QStringList> TagRepository::findReportTagNamesBatch(const QList<qint64>& reportIds)
{
    QHash<qint64, QStringList> result;
    if (reportIds.isEmpty()) return result;

    // 去重（同一列表可能含重复 id）
    // Qt6 已移除 QList::toSet()，改用 QSet 迭代器构造去重
    for (const QList<qint64>& batch : BaseRepository::chunkIds(BaseRepository::dedupeIds(reportIds))) {
        QSqlDatabase db = BaseRepository::db();
        QSqlQuery query(db);        query.prepare(QString(
            "SELECT rt.report_id, t.name FROM tags t "
            "JOIN report_tags rt ON rt.tag_id = t.id "
            "WHERE rt.report_id IN (%1) "
            "ORDER BY rt.report_id, t.name").arg(BaseRepository::inPlaceholders(batch.size())));
        for (int i = 0; i < batch.size(); ++i) {
            query.bindValue(QString(":id%1").arg(i), batch.at(i));
        }
        if (!BaseRepository::execChecked(query, "批量查询报告标签名")) continue;

        while (query.next()) {
            const qint64 reportId = query.value(0).toLongLong();
            result[reportId].append(query.value(1).toString());
        }
    }
    return result;
}
QHash<qint64, Tag::List> TagRepository::findReportTagsBatch(const QList<qint64>& reportIds)
{
    QHash<qint64, Tag::List> result;
    if (reportIds.isEmpty()) return result;

    // 去重 + 500 一批分块（同 findReportTagNamesBatch 惯例）
    for (const QList<qint64>& batch : BaseRepository::chunkIds(BaseRepository::dedupeIds(reportIds))) {
        QSqlDatabase db = BaseRepository::db();
        QSqlQuery query(db);        query.prepare(QString(
            "SELECT rt.report_id, t.* FROM tags t "
            "JOIN report_tags rt ON rt.tag_id = t.id "
            "WHERE rt.report_id IN (%1) "
            "ORDER BY rt.report_id, t.name").arg(BaseRepository::inPlaceholders(batch.size())));
        for (int i = 0; i < batch.size(); ++i) {
            query.bindValue(QString(":id%1").arg(i), batch.at(i));
        }
        if (!BaseRepository::execChecked(query, "批量查询报告标签")) continue;

        while (query.next()) {
            const qint64 reportId = query.value(0).toLongLong();
            result[reportId].append(createFromQuery(query));
        }
    }
    return result;
}

// ===========================================================================
// 内部辅助方法
// ===========================================================================

/**
 * @brief 从数据库查询结果创建标签对象
 *
 * @param query 已执行的数据库查询对象，当前行包含标签数据
 * @return Tag::Ptr 填充好的标签智能指针
 *
 * @note 调用前需确保 query.next() 已返回 true
 */
Tag::Ptr TagRepository::createFromQuery(const QSqlQuery& query)
{
    // 创建空的标签对象
    Tag::Ptr tag(new Tag());

    // 映射各字段
    tag->setId(query.value("id").toLongLong());              // 标签唯一 ID
    tag->setName(query.value("name").toString());             // 标签名称
    tag->setColor(query.value("color").toString());           // 标签颜色（十六进制字符串，如 "#FF0000"）
    tag->setDescription(query.value("description").toString()); // 标签描述
    tag->setCreatedAt(query.value("created_at").toDateTime()); // 创建时间

    return tag;
}
