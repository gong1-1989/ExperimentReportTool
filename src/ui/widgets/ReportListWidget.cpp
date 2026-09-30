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
#include "core/utils/UserSession.h"
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
    , m_reportFilter(ReportFilter::All)
{
    ui->setupUi(this);
    setupUi();   // 行为与样式配置（结构已由 .ui 定义）
    setupFilterButtons();  // 动态创建筛选标签栏
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
    // 支持 Ctrl/Shift 多选（批量导出/批量删除用）；单击单选行为不变
    ui->tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
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
    applyDefaultFilter();
    refreshList();
}

void ReportListWidget::refreshList()
{
    // 待审核/待审批/被退回为全局待办：未选择项目时同样显示
    const bool globalFilter = (m_reportFilter == ReportFilter::ToReview
                               || m_reportFilter == ReportFilter::ToApprove
                               || m_reportFilter == ReportFilter::Rejected);
    if (m_currentProjectId <= 0 && !globalFilter) {
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

QList<qint64> ReportListWidget::selectedReportIds() const
{
    QList<qint64> ids;
    const QModelIndexList rows = ui->tableWidget->selectionModel()->selectedRows();
    for (const QModelIndex& index : rows) {
        const qint64 id = m_tableModel->reportIdAt(index.row());
        if (id > 0) ids.append(id);
    }
    return ids;
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
    User::Ptr currentUser = UserSession::instance().currentUser();

    // 待审核/待审批：全局跨项目，不限制 projectId
    const bool globalFilter = (m_reportFilter == ReportFilter::ToReview
                                || m_reportFilter == ReportFilter::ToApprove
                                || m_reportFilter == ReportFilter::Rejected);

    // 非全局筛选且未选择项目时不显示
    if (!globalFilter && m_currentProjectId <= 0) {
        return Report::List();
    }

    ReportQuery query;
    if (!globalFilter) query.projectId = m_currentProjectId;
    query.sortBy = "updated_at";
    query.sortOrder = Qt::DescendingOrder;

    // 角色筛选决定状态
    if (m_reportFilter == ReportFilter::ToReview) {
        query.status = ReportStatus::Submitted;
    } else if (m_reportFilter == ReportFilter::ToApprove) {
        query.status = ReportStatus::Reviewed;
    } else if (m_statusFilterIndex == 1) {
        query.status = ReportStatus::Draft;
    } else if (m_statusFilterIndex == 2) {
        query.status = ReportStatus::Submitted;
    } else if (m_statusFilterIndex == 3) {
        query.status = ReportStatus::Reviewed;
    }

    Report::List reports = ReportService::query(query);

    // 角色筛选：我的/本组（在内存中过滤）
    if (m_reportFilter == ReportFilter::Mine && currentUser) {
        Report::List filtered;
        for (const Report::Ptr& r : reports) {
            if (r->createdBy() == currentUser->id()) filtered.append(r);
        }
        reports = filtered;
    } else if (m_reportFilter == ReportFilter::Group && currentUser && currentUser->groupId() > 0) {
        // 本组：创建者与当前用户同组（需要查创建者的组，用 PermissionService 辅助）
        Report::List filtered;
        for (const Report::Ptr& r : reports) {
            if (PermissionService::canViewReport(r, currentUser)) filtered.append(r);
        }
        reports = filtered;
    }

    // 被退回筛选：状态=Draft 且 last_action 以 _reject 结尾（统一走公共方法，与默认选中统计判据一致）
    if (m_reportFilter == ReportFilter::Rejected) {
        reports = rejectedReportsFor(currentUser, reports);
    }

    // 优化1：草稿可见性过滤（超管/总管跳过；其他角色按规则过滤）
    if (currentUser && !currentUser->isSuperAdmin() && !currentUser->isManager()) {
        Report::List visible;
        for (const Report::Ptr& r : reports) {
            if (PermissionService::canViewReport(r, currentUser)) visible.append(r);
        }
        reports = visible;
    }

    // 关键词筛选
    if (!m_searchKeyword.isEmpty()) {
        Report::List filtered;
        for (const Report::Ptr& r : reports) {
            if (r->title().contains(m_searchKeyword, Qt::CaseInsensitive)) {
                filtered.append(r);
            }
        }
        reports = filtered;
    }

    return reports;
}

// 公共判据：当前用户可见的被退回报告（组员=自己的；组长=本组且排除自己退回的；总管/超管=全部）
// 统计（默认选中）与列表显示共用此方法，避免两套判据漂移
Report::List ReportListWidget::rejectedReportsFor(const User::Ptr& user, const Report::List& all)
{
    Report::List rejected;
    if (!user) return rejected;

    // 用户批量缓存（一次 SQL 替代循环内 N 次 findById）
    QMap<qint64, User::Ptr> userCache;
    const User::List users = UserRepository::findAll();
    for (const User::Ptr& u : users) userCache.insert(u->id(), u);

    for (const Report::Ptr& r : all) {
        if (r->status() != ReportStatus::Draft || !r->lastAction().endsWith("_reject")) continue;
        if (user->isMember()) {
            if (r->createdBy() == user->id()) rejected.append(r);
        } else if (user->isLeader() && user->groupId() > 0) {
            // 组长看本组被退回，但排除自己退回的（自己已处理，不再作为待办显示）
            if (r->lastActionBy() != user->id()) {
                const User::Ptr creator = userCache.value(r->createdBy());
                if (creator && creator->groupId() == user->groupId()) rejected.append(r);
            }
        } else {
            rejected.append(r);  // 总管/超管
        }
    }
    return rejected;
}

void ReportListWidget::loadReportsToTable(const Report::List& reports)
{
    // 交给模型：id 序列一致时仅 dataChanged 局部通知，否则 reset（QTableView 虚拟化，均轻量）
    m_tableModel->setReports(reports);
}

void ReportListWidget::setupFilterButtons()
{
    // 动态创建筛选标签按钮栏，放在搜索框上方
    auto* bar = new QHBoxLayout();
    bar->setContentsMargins(0, 0, 0, 0);
    bar->setSpacing(4);

    User::Ptr user = UserSession::instance().currentUser();

    // 基础标签（所有角色）：全部、我的、本组
    QStringList labels = {"全部", "我的", "本组"};
    QList<ReportFilter> filters = {ReportFilter::All, ReportFilter::Mine, ReportFilter::Group};

    // 根据角色动态追加待办标签
    if (user) {
        if (user->isMember()) {
            // 组员：被退回
            labels.append("被退回");
            filters.append(ReportFilter::Rejected);
        } else if (user->isLeader()) {
            // 组长：待审核、被退回
            labels.append("待审核");
            filters.append(ReportFilter::ToReview);
            labels.append("被退回");
            filters.append(ReportFilter::Rejected);
        } else {
            // 总管/超管：待审核、待审批
            labels.append("待审核");
            filters.append(ReportFilter::ToReview);
            labels.append("待审批");
            filters.append(ReportFilter::ToApprove);
        }
    }

    for (int i = 0; i < labels.size(); ++i) {
        auto* btn = new QPushButton(labels[i], this);
        btn->setCheckable(true);
        btn->setFixedHeight(26);
        // 按钮携带自身筛选值，供统一高亮同步使用
        btn->setProperty("filter", static_cast<int>(filters[i]));
        btn->setStyleSheet(
            "QPushButton{padding:3px 12px;border-radius:4px;border:1px solid transparent;"
            "background:transparent;color:#333;}"
            "QPushButton:hover{background:#e8f0fe;border:1px solid #c5d9f8;}"
            "QPushButton:checked{background:#2f6fed;color:white;border:1px solid #2f6fed;font-weight:bold;}");
        const ReportFilter f = filters[i];
        connect(btn, &QPushButton::clicked, this, [this, f]() {
            m_reportFilter = f;
            syncFilterButtons();
            refreshList();
        });
        m_filterButtons.append(btn);
        bar->addWidget(btn);
    }
    bar->addStretch();

    // 插入到主布局的顶部（搜索框上方）
    if (auto* mainLayout = qobject_cast<QVBoxLayout*>(layout())) {
        mainLayout->insertLayout(0, bar);
    }

    // 构造时按当前 m_reportFilter 高亮选中按钮
    syncFilterButtons();
}

// 统一高亮当前选中的筛选标签（按按钮携带的筛选值，不依赖位置索引）
void ReportListWidget::syncFilterButtons()
{
    for (QPushButton* b : m_filterButtons) {
        const ReportFilter f = static_cast<ReportFilter>(b->property("filter").toInt());
        b->setChecked(f == m_reportFilter);
    }
}

void ReportListWidget::applyDefaultFilter()
{
    User::Ptr user = UserSession::instance().currentUser();
    if (!user) return;

    // 统计待办数量（待审核/待审批）
    int toReviewCount = 0, toApproveCount = 0;
    {
        ReportQuery q;
        q.status = ReportStatus::Submitted;
        Report::List list = ReportService::query(q);
        for (const Report::Ptr& r : list) {
            if (PermissionService::canReviewReport(r, user)) toReviewCount++;
        }
    }
    {
        ReportQuery q;
        q.status = ReportStatus::Reviewed;
        Report::List list = ReportService::query(q);
        for (const Report::Ptr& r : list) {
            if (PermissionService::canApproveReport(r, user)) toApproveCount++;
        }
    }

    // 统计被退回数量（与列表显示共用同一判据）
    int rejectedCount = 0;
    {
        ReportQuery q;
        q.status = ReportStatus::Draft;
        Report::List list = ReportService::query(q);
        rejectedCount = rejectedReportsFor(user, list).size();
    }

    ReportFilter target = ReportFilter::All;
    if (user->isMember()) {
        // 组员：有被退回则"被退回"，否则"我的"
        target = (rejectedCount > 0) ? ReportFilter::Rejected : ReportFilter::Mine;
    } else if (user->isLeader()) {
        // 组长：有待审核则"待审核"，否则有被退回则"被退回"，否则"本组"
        if (toReviewCount > 0) target = ReportFilter::ToReview;
        else if (rejectedCount > 0) target = ReportFilter::Rejected;
        else target = ReportFilter::Group;
    } else {
        // 总管/超管：有待审批则"待审批"，否则有待审核则"待审核"，否则"全部"
        if (toApproveCount > 0) target = ReportFilter::ToApprove;
        else if (toReviewCount > 0) target = ReportFilter::ToReview;
        else target = ReportFilter::All;
    }

    m_reportFilter = target;
    // 同步按钮高亮（按按钮携带的筛选值匹配，避免不同角色按钮数量不同导致错位）
    syncFilterButtons();
}
