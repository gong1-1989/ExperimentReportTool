/**
 * @file ReportService.cpp
 * @brief 报告业务逻辑服务层实现
 */

#include "ReportService.h"
#include "data/repositories/ReportRepository.h"
#include "data/repositories/TagRepository.h"
#include "core/models/Tag.h"
#include "core/utils/Logger.h"

#include <QDateTime>
#include <QObject>

// ============================================================================
// 基础 CRUD
// ============================================================================

Report::Ptr ReportService::getById(qint64 id)
{
    return ReportRepository::findById(id);
}

Report::Ptr ReportService::create(const QString& title, qint64 projectId,
                                   const QString& author)
{
    Report::Ptr report = Report::create();
    report->setTitle(title);
    report->setProjectId(projectId);
    if (!author.isEmpty()) {
        report->setAuthor(author);
    }
    report->setCreatedAt(QDateTime::currentDateTime());
    report->setUpdatedAt(QDateTime::currentDateTime());

    if (ReportRepository::insert(report)) {
        return report;
    }
    return nullptr;
}

bool ReportService::update(const Report::Ptr& report)
{
    if (!report) return false;
    report->setUpdatedAt(QDateTime::currentDateTime());
    return ReportRepository::update(report);
}

bool ReportService::remove(qint64 id)
{
    return ReportRepository::remove(id);
}

// ============================================================================
// 查询
// ============================================================================

Report::List ReportService::query(const ReportQuery& query)
{
    // 先获取项目下的所有报告
    Report::List reports = ReportRepository::findByProject(query.projectId);

    // 关键词筛选（标题/作者/标签）
    if (!query.keyword.isEmpty()) {
        Report::List filtered;
        for (const Report::Ptr& report : reports) {
            bool match = false;
            // 标题匹配
            if (report->title().contains(query.keyword, Qt::CaseInsensitive)) {
                match = true;
            }
            // 作者匹配
            if (!match && report->author().contains(query.keyword, Qt::CaseInsensitive)) {
                match = true;
            }
            // 标签匹配（通过关联表查询）
            if (!match) {
                const QStringList tagNames = TagRepository::findReportTagNames(report->id());
                for (const QString& tagName : tagNames) {
                    if (tagName.contains(query.keyword, Qt::CaseInsensitive)) {
                        match = true;
                        break;
                    }
                }
            }
            if (match) {
                filtered.append(report);
            }
        }
        reports = filtered;
    }

    // 状态筛选（status = -1 表示全部）
    if (query.status != static_cast<ReportStatus>(-1)) {
        Report::List filtered;
        for (const Report::Ptr& report : reports) {
            if (report->status() == query.status) {
                filtered.append(report);
            }
        }
        reports = filtered;
    }

    return reports;
}

Report::List ReportService::listByProject(qint64 projectId)
{
    return ReportRepository::findByProject(projectId);
}

Report::List ReportService::search(const QString& keyword, qint64 projectId)
{
    ReportQuery query;
    query.projectId = projectId;
    query.keyword = keyword;
    return ReportService::query(query);
}

// ============================================================================
// 统计
// ============================================================================

int ReportService::count(qint64 projectId)
{
    return ReportRepository::findByProject(projectId).size();
}

int ReportService::countByStatus(ReportStatus status, qint64 projectId)
{
    Report::List reports = ReportRepository::findByProject(projectId);
    int count = 0;
    for (const Report::Ptr& report : reports) {
        if (report->status() == status) {
            ++count;
        }
    }
    return count;
}

// ============================================================================
// 业务操作
// ============================================================================

Report::Ptr ReportService::duplicate(qint64 id, const QString& newTitle)
{
    Report::Ptr source = ReportRepository::findById(id);
    if (!source) return nullptr;

    Report::Ptr copy = Report::create();
    copy->setTitle(newTitle.isEmpty() ? source->title() + QObject::tr(" (副本)") : newTitle);
    copy->setProjectId(source->projectId());
    copy->setAuthor(source->author());
    copy->setTemplateId(source->templateId());
    copy->setStatus(ReportStatus::Draft);
    copy->setCreatedAt(QDateTime::currentDateTime());
    copy->setUpdatedAt(QDateTime::currentDateTime());

    if (ReportRepository::insert(copy)) {
        return copy;
    }
    return nullptr;
}

bool ReportService::updateStatus(qint64 id, ReportStatus status)
{
    Report::Ptr report = ReportRepository::findById(id);
    if (!report) return false;
    report->setStatus(status);
    report->setUpdatedAt(QDateTime::currentDateTime());
    return ReportRepository::update(report);
}

bool ReportService::updateTag(qint64 id, qint64 tagId)
{
    // 标签通过关联表存储，使用 TagRepository 设置
    QList<qint64> tagIds;
    if (tagId > 0) {
        tagIds.append(tagId);
    }
    return TagRepository::setReportTags(id, tagIds);
}
