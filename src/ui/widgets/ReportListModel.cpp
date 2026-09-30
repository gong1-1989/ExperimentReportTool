/**
 * @file ReportListModel.cpp
 * @brief 报告列表表格模型实现
 */

#include "ReportListModel.h"

#include "service/UserService.h"
#include "service/TagService.h"
#include "core/utils/AppTheme.h"

#include <algorithm>

// ===========================================================================
// 构造
// ===========================================================================

ReportListModel::ReportListModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

// ===========================================================================
// QAbstractTableModel 接口
// ===========================================================================

int ReportListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return m_reports.size();
}

int ReportListModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return ColumnCount;
}

QVariant ReportListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_reports.size()) {
        return QVariant();
    }

    const Report::Ptr& report = m_reports.at(index.row());
    if (!report) return QVariant();

    switch (role) {
    case Qt::DisplayRole: {
        switch (index.column()) {
        case ColTitle:   return report->title();
        case ColStatus:  return report->statusDisplayName();
        case ColCreator: return creatorDisplayName(report->createdBy());
        case ColDate:    return report->experimentDate().isValid()
                            ? report->experimentDate().toString("yyyy-MM-dd") : QString();
        case ColUpdated: return report->updatedAt().isValid()
                            ? report->updatedAt().toString("yyyy-MM-dd hh:mm") : QString();
        case ColWords:   return QString::number(report->wordCount());
        case ColTags: {
            const QStringList tags = m_tagNames.value(report->id());
            if (tags.isEmpty()) return QStringLiteral("-");
            QStringList decorated;
            for (const QString& name : tags) decorated << QStringLiteral("■ %1").arg(name);
            return decorated.join(QChar::Space);
        }
        default: break;
        }
        break;
    }
    case Qt::ToolTipRole: {
        if (index.column() == ColTitle) return report->title();
        if (index.column() == ColTags) {
            const QStringList tags = m_tagNames.value(report->id());
            return tags.isEmpty() ? QString() : tags.join(QChar::Space);
        }
        break;
    }
    case Qt::ForegroundRole: {
        if (index.column() == ColStatus) return AppTheme::statusColor(report->status());
        break;
    }
    case Qt::UserRole: {
        return report->id();  // 兼容原 QTableWidgetItem 的 UserRole 存报告 id
    }
    default:
        break;
    }

    return QVariant();
}

QVariant ReportListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }

    switch (section) {
    case ColTitle:   return tr("标题");
    case ColStatus:  return tr("状态");
    case ColCreator: return tr("创建者");
    case ColDate:    return tr("实验日期");
    case ColUpdated: return tr("更新时间");
    case ColWords:   return tr("字数");
    case ColTags:    return tr("标签");
    default:         return QVariant();
    }
}

Qt::ItemFlags ReportListModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;
    return QAbstractTableModel::flags(index) & ~Qt::ItemIsEditable;
}

// ===========================================================================
// 数据更新
// ===========================================================================

void ReportListModel::setReports(const Report::List& reports)
{
    // id 序列一致 → 内容级局部更新（仅 dataChanged 通知，视图只重绘变化区域）
    if (m_reports.size() == reports.size()) {
        bool sameIds = true;
        for (int i = 0; i < m_reports.size(); ++i) {
            if (!m_reports.at(i) || !reports.at(i)
                || m_reports.at(i)->id() != reports.at(i)->id()) {
                sameIds = false;
                break;
            }
        }
        if (sameIds) {
            m_reports = reports;
            reloadMeta();
            // 空列表无需通知（index(0,0) 为无效索引，会触发 Qt 警告）
            if (!m_reports.isEmpty()) {
                emit dataChanged(index(0, 0), index(m_reports.size() - 1, ColumnCount - 1));
            }
            return;
        }
    }

    beginResetModel();
    m_reports = reports;
    reloadMeta();
    endResetModel();
}

qint64 ReportListModel::reportIdAt(int row) const
{
    if (row < 0 || row >= m_reports.size() || !m_reports.at(row)) return -1;
    return m_reports.at(row)->id();
}

// ===========================================================================
// 排序
// ===========================================================================

void ReportListModel::sort(int column, Qt::SortOrder order)
{
    beginResetModel();
    std::sort(m_reports.begin(), m_reports.end(),
        [this, column](const Report::Ptr& a, const Report::Ptr& b) {
            if (!a || !b) return a < b;
            bool less = false;
            switch (column) {
            case ColTitle:   less = a->title().localeAwareCompare(b->title()) < 0; break;
            case ColStatus:  less = static_cast<int>(a->status()) < static_cast<int>(b->status()); break;
            case ColCreator: less = creatorDisplayName(a->createdBy())
                                    .localeAwareCompare(creatorDisplayName(b->createdBy())) < 0; break;
            case ColDate:    less = a->experimentDate() < b->experimentDate(); break;
            case ColUpdated: less = a->updatedAt() < b->updatedAt(); break;
            case ColWords:   less = a->wordCount() < b->wordCount(); break;
            case ColTags:    less = m_tagNames.value(a->id()).join(QChar::Space)
                                    .localeAwareCompare(m_tagNames.value(b->id()).join(QChar::Space)) < 0; break;
            default:         less = a->id() < b->id(); break;
            }
            return less;
        });
    if (order == Qt::DescendingOrder) {
        std::reverse(m_reports.begin(), m_reports.end());
    }
    endResetModel();
}

// ===========================================================================
// 私有
// ===========================================================================

void ReportListModel::reloadMeta()
{
    // 创建者显示名：批量一次查询
    QList<qint64> creatorIds;
    QList<qint64> reportIds;
    for (const Report::Ptr& report : m_reports) {
        if (!report) continue;
        if (report->createdBy() > 0) creatorIds.append(report->createdBy());
        reportIds.append(report->id());
    }
    m_userNames = UserService::batchDisplayNames(creatorIds);

    // 标签名：批量一次查询
    m_tagNames = TagService::findReportTagNamesBatch(reportIds);
}

QString ReportListModel::creatorDisplayName(qint64 createdBy) const
{
    if (createdBy <= 0) return tr("未分配");
    return m_userNames.value(createdBy, tr("未知用户"));
}
