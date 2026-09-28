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

    // 表格视图
    ui->tableWidget->setColumnCount(7);
    ui->tableWidget->setHorizontalHeaderLabels({
        tr("标题"), tr("状态"), tr("创建者"),
        tr("实验日期"), tr("更新时间"), tr("字数"), tr("标签")
    });
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setAlternatingRowColors(true);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tableWidget->setSortingEnabled(true);

    // 卡片视图
    ui->cardWidget->setViewMode(QListView::IconMode);
    ui->cardWidget->setIconSize(QSize(120, 90));
    ui->cardWidget->setGridSize(QSize(160, 140));
    ui->cardWidget->setResizeMode(QListView::Adjust);
    ui->cardWidget->setMovement(QListView::Static);

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
    const int row = ui->tableWidget->currentRow();
    if (row < 0) return -1;

    QTableWidgetItem* item = ui->tableWidget->item(row, 0);
    if (!item) return -1;

    return item->data(Qt::UserRole).toLongLong();
}

// ===========================================================================
// 私有槽函数
// ===========================================================================

void ReportListWidget::on_searchEdit_textChanged(const QString& text)
{
    m_searchKeyword = text;
    refreshList();
}

void ReportListWidget::on_statusFilter_currentIndexChanged(int index)
{
    m_statusFilterIndex = index;
    refreshList();
}

void ReportListWidget::on_tableWidget_cellDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    if (row < 0) return;

    QTableWidgetItem* item = ui->tableWidget->item(row, 0);
    if (!item) return;

    const qint64 reportId = item->data(Qt::UserRole).toLongLong();
    emit reportOpenRequested(reportId);
}

void ReportListWidget::on_tableWidget_customContextMenuRequested(const QPoint& pos)
{
    QTableWidgetItem* item = ui->tableWidget->itemAt(pos);
    if (!item) return;

    QMenu menu(this);
    QAction* actionOpen = menu.addAction(tr("打开报告"));
    QAction* actionEdit = menu.addAction(tr("编辑报告"));
    menu.addSeparator();
    QAction* actionDelete = menu.addAction(tr("删除报告"));

    QAction* selected = menu.exec(ui->tableWidget->viewport()->mapToGlobal(pos));

    const qint64 reportId = item->data(Qt::UserRole).toLongLong();

    if (selected == actionOpen) {
        emit reportOpenRequested(reportId);
    } else if (selected == actionEdit) {
        emit reportEditRequested(reportId);
    } else if (selected == actionDelete) {
        emit reportDeleteRequested(reportId);
    }
}

void ReportListWidget::on_tableWidget_itemSelectionChanged()
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
    // 暂时禁用排序，避免插入时排序出错
    ui->tableWidget->setSortingEnabled(false);
    ui->tableWidget->setRowCount(reports.size());

    // 用户信息缓存（避免重复查询）
    QMap<qint64, QString> userNameCache;

    for (int row = 0; row < reports.size(); ++row) {
        const Report::Ptr& report = reports.at(row);

        // 标题列（存储 ID 在 UserRole）
        QTableWidgetItem* titleItem = new QTableWidgetItem(report->title());
        titleItem->setData(Qt::UserRole, report->id());
        titleItem->setToolTip(report->title());
        ui->tableWidget->setItem(row, 0, titleItem);

        // 状态列
        QTableWidgetItem* statusItem = new QTableWidgetItem(statusDisplayName(report->status()));
        statusItem->setForeground(AppTheme::statusColor(report->status()));
        ui->tableWidget->setItem(row, 1, statusItem);

        // 创建者列
        QString creatorName = tr("未分配");
        if (report->createdBy() > 0) {
            if (userNameCache.contains(report->createdBy())) {
                creatorName = userNameCache.value(report->createdBy());
            } else {
                User::Ptr user = UserService::getById(report->createdBy());
                if (user) {
                    creatorName = user->displayNameOrUsername();
                    userNameCache.insert(report->createdBy(), creatorName);
                } else {
                    creatorName = tr("未知用户");
                }
            }
        }
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(creatorName));

        // 实验日期列
        const QString dateStr = report->experimentDate().isValid()
            ? report->experimentDate().toString("yyyy-MM-dd")
            : tr("未设置");
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(dateStr));

        // 更新时间列
        ui->tableWidget->setItem(row, 4,
            new QTableWidgetItem(report->updatedAt().toString("yyyy-MM-dd hh:mm")));

        // 字数列
        ui->tableWidget->setItem(row, 5,
            new QTableWidgetItem(QString::number(report->wordCount())));

        // 标签列（显示颜色方块 + 标签名）
        const Tag::List tags = TagService::findByReport(report->id());
        if (!tags.isEmpty()) {
            QString tagText;
            for (int i = 0; i < tags.size(); ++i) {
                if (i > 0) tagText += " ";
                tagText += QString("■ %1").arg(tags[i]->name());
            }
            QTableWidgetItem* tagItem = new QTableWidgetItem(tagText);
            tagItem->setForeground(tags.first()->effectiveColor());
            tagItem->setToolTip(tagText);
            ui->tableWidget->setItem(row, 6, tagItem);
        } else {
            ui->tableWidget->setItem(row, 6, new QTableWidgetItem("-"));
        }
    }

    ui->tableWidget->setSortingEnabled(true);
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
