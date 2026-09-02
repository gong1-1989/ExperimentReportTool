/**
 * @file ReportListWidget.cpp
 * @brief 报告列表组件实现文件
 */

#include "ReportListWidget.h"
#include "data/repositories/ReportRepository.h"
#include "data/repositories/ProjectRepository.h"
#include "core/utils/Logger.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QDateTime>

// ===========================================================================
// 构造函数
// ===========================================================================

ReportListWidget::ReportListWidget(QWidget* parent)
    : QWidget(parent)
    , m_currentProjectId(-1)
    , m_statusFilterIndex(0)
{
    setupUi();
}

// ===========================================================================
// UI 初始化
// ===========================================================================

void ReportListWidget::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // -----------------------------------------------------------------------
    // 顶部工具栏
    // -----------------------------------------------------------------------
    QHBoxLayout* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(8);

    // 搜索框
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("搜索报告标题..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setMaximumWidth(250);
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &ReportListWidget::onSearchTextChanged);
    toolbarLayout->addWidget(m_searchEdit);

    // 状态筛选
    m_statusFilter = new QComboBox(this);
    m_statusFilter->addItem(tr("全部状态"));
    m_statusFilter->addItem(tr("草稿"));
    m_statusFilter->addItem(tr("已提交"));
    m_statusFilter->addItem(tr("已审核"));
    connect(m_statusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReportListWidget::onStatusFilterChanged);
    toolbarLayout->addWidget(m_statusFilter);

    toolbarLayout->addStretch();

    // 数量标签
    m_countLabel = new QLabel(tr("共 0 份报告"), this);
    m_countLabel->setStyleSheet("color: #666; font-size: 12px;");
    toolbarLayout->addWidget(m_countLabel);

    // 视图切换按钮
    m_viewToggleButton = new QPushButton(tr("卡片视图"), this);
    m_viewToggleButton->setCheckable(true);
    connect(m_viewToggleButton, &QPushButton::clicked,
            this, &ReportListWidget::onToggleView);
    toolbarLayout->addWidget(m_viewToggleButton);

    // 新建按钮
    m_newButton = new QPushButton(tr("新建报告"), this);
    m_newButton->setStyleSheet(
        "QPushButton { background-color: #4A90D9; color: white; padding: 6px 16px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #357ABD; }");
    connect(m_newButton, &QPushButton::clicked,
            this, &ReportListWidget::onNewReport);
    toolbarLayout->addWidget(m_newButton);

    mainLayout->addLayout(toolbarLayout);

    // -----------------------------------------------------------------------
    // 列表区域
    // -----------------------------------------------------------------------
    m_stackWidget = new QStackedWidget(this);

    // 表格视图
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(6);
    m_tableWidget->setHorizontalHeaderLabels({
        tr("标题"), tr("状态"), tr("作者"),
        tr("实验日期"), tr("更新时间"), tr("字数")
    });
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->horizontalHeader()->setStretchLastSection(true);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tableWidget->setSortingEnabled(true);

    connect(m_tableWidget, &QTableWidget::cellDoubleClicked,
            this, &ReportListWidget::onTableDoubleClicked);
    connect(m_tableWidget, &QTableWidget::customContextMenuRequested,
            this, &ReportListWidget::onTableCustomContextMenu);

    m_stackWidget->addWidget(m_tableWidget);

    // 卡片视图（占位）
    m_cardWidget = new QListWidget(this);
    m_cardWidget->setViewMode(QListView::IconMode);
    m_cardWidget->setIconSize(QSize(120, 90));
    m_cardWidget->setGridSize(QSize(160, 140));
    m_cardWidget->setResizeMode(QListView::Adjust);
    m_cardWidget->setMovement(QListView::Static);
    m_stackWidget->addWidget(m_cardWidget);

    mainLayout->addWidget(m_stackWidget);

    // 初始加载
    refreshList();
}

// ===========================================================================
// 公共方法
// ===========================================================================

void ReportListWidget::setProjectId(qint64 projectId)
{
    m_currentProjectId = projectId;
    refreshList();
}

void ReportListWidget::refreshList()
{
    const Report::List reports = getFilteredReports();
    loadReportsToTable(reports);
    m_countLabel->setText(tr("共 %1 份报告").arg(reports.size()));
}

qint64 ReportListWidget::currentReportId() const
{
    const int row = m_tableWidget->currentRow();
    if (row < 0) return -1;

    QTableWidgetItem* item = m_tableWidget->item(row, 0);
    if (!item) return -1;

    return item->data(Qt::UserRole).toLongLong();
}

// ===========================================================================
// 私有槽函数
// ===========================================================================

void ReportListWidget::onSearchTextChanged(const QString& text)
{
    m_searchKeyword = text;
    refreshList();
}

void ReportListWidget::onStatusFilterChanged(int index)
{
    m_statusFilterIndex = index;
    refreshList();
}

void ReportListWidget::onTableDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    if (row < 0) return;

    QTableWidgetItem* item = m_tableWidget->item(row, 0);
    if (!item) return;

    const qint64 reportId = item->data(Qt::UserRole).toLongLong();
    emit reportOpenRequested(reportId);
}

void ReportListWidget::onTableCustomContextMenu(const QPoint& pos)
{
    QTableWidgetItem* item = m_tableWidget->itemAt(pos);
    if (!item) return;

    QMenu menu(this);
    QAction* actionOpen = menu.addAction(tr("打开报告"));
    QAction* actionEdit = menu.addAction(tr("编辑报告"));
    menu.addSeparator();
    QAction* actionDelete = menu.addAction(tr("删除报告"));

    QAction* selected = menu.exec(m_tableWidget->viewport()->mapToGlobal(pos));

    const qint64 reportId = item->data(Qt::UserRole).toLongLong();

    if (selected == actionOpen) {
        emit reportOpenRequested(reportId);
    } else if (selected == actionEdit) {
        emit reportEditRequested(reportId);
    } else if (selected == actionDelete) {
        emit reportDeleteRequested(reportId);
    }
}

void ReportListWidget::onNewReport()
{
    if (m_currentProjectId <= 0) {
        QMessageBox::information(this, tr("提示"), tr("请先在左侧选择一个项目"));
        return;
    }
    emit reportNewRequested(m_currentProjectId);
}

void ReportListWidget::onEditReport()
{
    const qint64 reportId = currentReportId();
    if (reportId > 0) {
        emit reportEditRequested(reportId);
    }
}

void ReportListWidget::onDeleteReport()
{
    const qint64 reportId = currentReportId();
    if (reportId > 0) {
        emit reportDeleteRequested(reportId);
    }
}

void ReportListWidget::onToggleView()
{
    if (m_stackWidget->currentIndex() == 0) {
        m_stackWidget->setCurrentIndex(1);
        m_viewToggleButton->setText(tr("表格视图"));
    } else {
        m_stackWidget->setCurrentIndex(0);
        m_viewToggleButton->setText(tr("卡片视图"));
    }
}

// ===========================================================================
// 私有方法
// ===========================================================================

Report::List ReportListWidget::getFilteredReports()
{
    ReportQuery query;
    query.projectId = m_currentProjectId;
    query.sortBy = "updated_at";
    query.sortOrder = Qt::DescendingOrder;

    // 状态筛选
    if (m_statusFilterIndex == 1) query.status = ReportStatus::Draft;
    else if (m_statusFilterIndex == 2) query.status = ReportStatus::Submitted;
    else if (m_statusFilterIndex == 3) query.status = ReportStatus::Reviewed;

    Report::List reports = ReportRepository::findAll(query);

    // 关键词筛选（在内存中过滤，因为 FTS 搜索是独立接口）
    if (!m_searchKeyword.isEmpty()) {
        Report::List filtered;
        for (const Report::Ptr& report : reports) {
            if (report->title().contains(m_searchKeyword, Qt::CaseInsensitive)) {
                filtered.append(report);
            }
        }
        reports = filtered;
    }

    return reports;
}

void ReportListWidget::loadReportsToTable(const Report::List& reports)
{
    // 暂时禁用排序，避免插入时排序出错
    m_tableWidget->setSortingEnabled(false);
    m_tableWidget->setRowCount(reports.size());

    for (int row = 0; row < reports.size(); ++row) {
        const Report::Ptr& report = reports.at(row);

        // 标题列（存储 ID 在 UserRole）
        QTableWidgetItem* titleItem = new QTableWidgetItem(report->title());
        titleItem->setData(Qt::UserRole, report->id());
        titleItem->setToolTip(report->title());
        m_tableWidget->setItem(row, 0, titleItem);

        // 状态列
        QTableWidgetItem* statusItem = new QTableWidgetItem(statusDisplayName(report->status()));
        // 根据状态设置颜色
        QColor statusColor;
        switch (report->status()) {
        case ReportStatus::Draft:     statusColor = QColor("#888888"); break;
        case ReportStatus::Submitted: statusColor = QColor("#E6A23C"); break;
        case ReportStatus::Reviewed:  statusColor = QColor("#67C23A"); break;
        }
        statusItem->setForeground(statusColor);
        m_tableWidget->setItem(row, 1, statusItem);

        // 作者列
        m_tableWidget->setItem(row, 2, new QTableWidgetItem(report->author()));

        // 实验日期列
        const QString dateStr = report->experimentDate().isValid()
            ? report->experimentDate().toString("yyyy-MM-dd")
            : tr("未设置");
        m_tableWidget->setItem(row, 3, new QTableWidgetItem(dateStr));

        // 更新时间列
        m_tableWidget->setItem(row, 4,
            new QTableWidgetItem(report->updatedAt().toString("yyyy-MM-dd hh:mm")));

        // 字数列
        m_tableWidget->setItem(row, 5,
            new QTableWidgetItem(QString::number(report->wordCount())));
    }

    m_tableWidget->setSortingEnabled(true);
}

QString ReportListWidget::statusDisplayName(ReportStatus status) const
{
    switch (status) {
    case ReportStatus::Draft:     return tr("草稿");
    case ReportStatus::Submitted: return tr("已提交");
    case ReportStatus::Reviewed:  return tr("已审核");
    }
    return tr("未知");
}
