/**
 * @file ReportListWidget.h
 * @brief 报告列表组件头文件
 *
 * 中间栏的报告列表，展示当前选中项目下的所有报告。
 * 支持表格视图和卡片视图，双击打开报告编辑器。
 */

#ifndef REPORT_LIST_WIDGET_H
#define REPORT_LIST_WIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QListWidget>
#include <QStackedWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QAction>
#include <QMenu>

#include "core/models/Report.h"

/**
 * @brief 报告列表组件
 *
 * 顶部为工具栏（搜索框、筛选、视图切换、新建按钮），
 * 下方为报告列表（表格视图 / 卡片视图切换）。
 */
class ReportListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ReportListWidget(QWidget* parent = nullptr);

    /**
     * @brief 设置当前项目 ID，刷新报告列表
     * @param projectId 项目 ID（-1 表示所有项目）
     */
    void setProjectId(qint64 projectId);

    /**
     * @brief 刷新报告列表
     */
    void refreshList();

    /**
     * @brief 获取当前选中的报告 ID
     * @return 报告 ID，未选中返回 -1
     */
    qint64 currentReportId() const;

signals:
    /**
     * @brief 请求打开报告
     * @param reportId 报告 ID
     */
    void reportOpenRequested(qint64 reportId);

    /**
     * @brief 请求新建报告
     * @param projectId 所属项目 ID
     */
    void reportNewRequested(qint64 projectId);

    /**
     * @brief 请求编辑报告
     * @param reportId 报告 ID
     */
    void reportEditRequested(qint64 reportId);

    /**
     * @brief 请求删除报告
     * @param reportId 报告 ID
     */
    void reportDeleteRequested(qint64 reportId);

    /**
     * @brief 报告列表变化信号
     */
    void reportListChanged();

private slots:
    /// 搜索框文本变化
    void onSearchTextChanged(const QString& text);
    /// 状态筛选变化
    void onStatusFilterChanged(int index);
    /// 表格双击
    void onTableDoubleClicked(int row, int column);
    /// 表格右键菜单
    void onTableCustomContextMenu(const QPoint& pos);
    /// 新建报告
    void onNewReport();
    /// 编辑报告
    void onEditReport();
    /// 删除报告
    void onDeleteReport();
    /// 切换视图
    void onToggleView();

private:
    /// 初始化 UI
    void setupUi();

    /// 加载报告到表格
    void loadReportsToTable(const Report::List& reports);

    /// 获取筛选后的报告列表
    Report::List getFilteredReports();

    /// 状态显示名称
    QString statusDisplayName(ReportStatus status) const;

    // -----------------------------------------------------------------------
    // UI 控件
    // -----------------------------------------------------------------------

    // 顶部工具栏
    QLineEdit* m_searchEdit;       ///< 搜索框
    QComboBox* m_statusFilter;     ///< 状态筛选
    QPushButton* m_newButton;      ///< 新建按钮
    QPushButton* m_viewToggleButton; ///< 视图切换按钮
    QLabel* m_countLabel;          ///< 数量标签

    // 列表区域
    QStackedWidget* m_stackWidget; ///< 视图切换容器
    QTableWidget* m_tableWidget;   ///< 表格视图
    QListWidget* m_cardWidget;     ///< 卡片视图（占位，后续实现）

    // -----------------------------------------------------------------------
    // 数据
    // -----------------------------------------------------------------------

    qint64 m_currentProjectId;  ///< 当前项目 ID
    QString m_searchKeyword;    ///< 搜索关键词
    int m_statusFilterIndex;    ///< 状态筛选索引（0=全部）
};

#endif // REPORT_LIST_WIDGET_H
