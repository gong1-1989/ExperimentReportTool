/**
 * @file ReportListWidget.cpp
 * @brief 报告列表组件实现文件
 */

#include "ReportListWidget.h"
#include "ui_ReportListWidget.h"
#include "service/ReportService.h"
#include "data/repositories/ReportRepository.h"
#include "data/repositories/ProjectRepository.h"
#include "service/TagService.h"
#include "data/repositories/TagRepository.h"
#include "service/UserService.h"
#include "data/repositories/UserRepository.h"
#include "core/models/Tag.h"
#include "core/models/User.h"
#include "core/utils/Logger.h"
#include "core/utils/AppTheme.h"
#include "core/utils/AppDimensions.h"

#include <QHeaderView>
#include "ui/UiHelper.h"
#include <QDateTime>
#include <QItemSelectionModel>

// ===========================================================================
// 构造函数
// ===========================================================================

ReportListWidget::ReportListWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ReportListWidget)
    , m_currentProjectId(-1)
    , m_statusFilterIndex(0)
{
    ui->setupUi(this);
    setupUi();   // 行为与样式配置（结构已由 .ui 定义）
}

ReportListWidget::~ReportListWidget()
{
    delete ui;
}

// ===========================================================================
// UI 初始化
// ===========================================================================

void ReportListWidget::setupUi()
{
    // 结构由 ReportListWidget.ui 定义，这里仅配置行为与样式

    // 搜索框
    ui->searchEdit->setMaximumWidth(AppDimensions::Widget::ReportSearchMaxWidth);

    // 数量标签
    ui->countLabel->setStyleSheet(
        QString("color: %1; font-size: %2px;")
            .arg(AppTheme::Color::Gray666).arg(AppTheme::FontSize::Small));

    // 新建按钮
    ui->newButton->setStyleSheet(
        QString("QPushButton { background-color: %1; color: white; "
                "padding: %2px %3px; border-radius: %4px; font-weight: bold; }"
                "QPushButton:hover { background-color: %5; }")
            .arg(AppTheme::Color::Primary)
            .arg(AppTheme::Spacing::Medium)
            .arg(AppTheme::Spacing::ExtraLarge)
            .arg(AppTheme::Radius::Medium)
            .arg(AppTheme::Color::PrimaryHover));

    // 表格视图：QTableView + ReportListModel（虚拟化渲染，按需创建可见行）
    m_tableModel = new ReportListModel(this);
    ui->tableWidget->setModel(m_tableModel);
    // QTableView 自身没有 selectionChanged 信号（在 QItemSelectionModel 上），必须手动连接
    connect(ui->tableWidget->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ReportListWidget::handleTableSelectionChanged);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setAlternatingRowColors(true);
    // 表格内容与表头居中
    UiHelper::centerTableWidget(ui->tableWidget);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    // 排序：点击表头按列排序（模型 sort() 实现）
    ui->tableWidget->setSortingEnabled(true);

    // 卡片视图
    ui->cardWidget->setViewMode(QListView::IconMode);
    ui->cardWidget->setIconSize(QSize(120, 90));
    ui->cardWidget->setGridSize(QSize(160, 140));
    ui->cardWidget->setResizeMode(QListView::Adjust);
    ui->cardWidget->setMovement(QListView::Static);

    // 搜索防抖：击键后 Delay::Normal(300ms) 内无新输入才刷新列表，
    // 避免连续击键触发大量 SQL 查询与内存过滤
    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(AppDimensions::Delay::Normal);
    connect(m_searchTimer, &QTimer::timeout, this, &ReportListWidget::refreshList);

    // 信号由 .ui 自动连接（on_searchEdit_textChanged 等）
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
    if (m_currentProjectId <= 0) {
        loadReportsToTable(Report::List());
        ui->countLabel->setText(tr("请在左侧选择一个项目"));
        return;
    }

    const Report::List reports = getFilteredReports();
    loadReportsToTable(reports);
    ui->countLabel->setText(tr("共 %1 份报告").arg(reports.size()));
}

qint64 ReportListWidget::currentReportId() const
{
    const QModelIndex index = ui->tableWidget->currentIndex();
    if (!index.isValid()) return -1;
    return m_tableModel->reportIdAt(index.row());
}

// ===========================================================================
// 私有槽函数
// ===========================================================================

void ReportListWidget::on_searchEdit_textChanged(const QString& text)
{
    m_searchKeyword = text;
    // 防抖：重置计时器，等用户停止输入 300ms 后再刷新
    m_searchTimer->start();
}

void ReportListWidget::on_statusFilter_currentIndexChanged(int index)
{
    m_statusFilterIndex = index;
    refreshList();
}

void ReportListWidget::on_tableWidget_doubleClicked(const QModelIndex& index)
{
    if (!index.isValid()) return;

    const qint64 reportId = m_tableModel->reportIdAt(index.row());
    if (reportId > 0) emit reportOpenRequested(reportId);
}

void ReportListWidget::on_tableWidget_customContextMenuRequested(const QPoint& pos)
{
    const QModelIndex index = ui->tableWidget->indexAt(pos);
    if (!index.isValid()) return;

    QMenu menu(this);
    QAction* actionOpen = menu.addAction(tr("打开报告"));
    QAction* actionEdit = menu.addAction(tr("编辑报告"));
    menu.addSeparator();
    QAction* actionDelete = menu.addAction(tr("删除报告"));

    QAction* selected = menu.exec(ui->tableWidget->viewport()->mapToGlobal(pos));

    const qint64 reportId = m_tableModel->reportIdAt(index.row());

    if (selected == actionOpen) {
        emit reportOpenRequested(reportId);
    } else if (selected == actionEdit) {
        emit reportEditRequested(reportId);
    } else if (selected == actionDelete) {
        emit reportDeleteRequested(reportId);
    }
}

void ReportListWidget::handleTableSelectionChanged()
{
    emit reportSelected(currentReportId());
}

void ReportListWidget::on_newButton_clicked()
{
    if (m_currentProjectId <= 0) {
        UiHelper::info(this, tr("提示"), tr("请先在左侧选择一个项目"));
        return;
    }
    emit reportNewRequested(m_currentProjectId);
}

void ReportListWidget::on_viewToggleButton_clicked()
{
    if (ui->stackWidget->currentIndex() == 0) {
        ui->stackWidget->setCurrentIndex(1);
        ui->viewToggleButton->setText(tr("表格视图"));
    } else {
        ui->stackWidget->setCurrentIndex(0);
        ui->viewToggleButton->setText(tr("卡片视图"));
    }
}

// ===========================================================================
// 私有方法
// ===========================================================================

Report::List ReportListWidget::getFilteredReports()
{
    // 未选择项目时不显示任何报告
    if (m_currentProjectId <= 0) {
        return Report::List();
    }

    ReportQuery query;
    query.projectId = m_currentProjectId;
    query.sortBy = "updated_at";
    query.sortOrder = Qt::DescendingOrder;

    // 状态筛选
    if (m_statusFilterIndex == 1) query.status = ReportStatus::Draft;
    else if (m_statusFilterIndex == 2) query.status = ReportStatus::Submitted;
    else if (m_statusFilterIndex == 3) query.status = ReportStatus::Reviewed;

    Report::List reports = ReportService::query(query);

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
    // 交给模型：id 序列一致时仅 dataChanged 局部通知，否则 reset（QTableView 虚拟化，均轻量）
    m_tableModel->setReports(reports);
}
