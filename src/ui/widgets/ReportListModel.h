/**
 * @file ReportListModel.h
 * @brief 报告列表表格模型头文件
 *
 * QTableView + QAbstractTableModel：视图按需渲染可见行（虚拟化），
 * 数据变化时通过 dataChanged / reset 局部通知，替代 QTableWidget 全量重建 item。
 * 创建者与标签采用批量查询（一次 SQL），消除逐行 N+1。
 */

#ifndef REPORT_LIST_MODEL_H
#define REPORT_LIST_MODEL_H

#include <QAbstractTableModel>
#include <QMap>
#include <QHash>

#include "core/models/Report.h"

/**
 * @brief 报告列表表格模型
 */
class ReportListModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        ColTitle = 0,
        ColStatus,
        ColCreator,
        ColDate,
        ColUpdated,
        ColWords,
        ColTags,
        ColumnCount   ///< 7 列
    };

    explicit ReportListModel(QObject* parent = nullptr);

    // QAbstractTableModel 接口
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    void sort(int column, Qt::SortOrder order) override;

    /**
     * @brief 替换报告数据
     * @param reports 新的报告列表
     *
     * 若新列表与当前列表的 id 序列一致（内容级更新），仅发送 dataChanged
     * 局部通知；否则 begin/endResetModel 整体重置（QTableView 虚拟化下同样轻量）。
     */
    void setReports(const Report::List& reports);

    /// 行号 → 报告 id（-1 表示无效）
    qint64 reportIdAt(int row) const;

private:
    /// 批量加载创建者显示名与标签名（一次 SQL 各一）
    void reloadMeta();

    /// 获取创建者显示名（未分配 / 未知用户 兜底）
    QString creatorDisplayName(qint64 createdBy) const;

    Report::List m_reports;                 ///< 当前报告列表
    QMap<qint64, QString> m_userNames;      ///< 创建者 id → 显示名
    QHash<qint64, QStringList> m_tagNames;  ///< 报告 id → 标签名列表
};

#endif // REPORT_LIST_MODEL_H
