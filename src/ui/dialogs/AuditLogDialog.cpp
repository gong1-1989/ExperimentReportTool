/**
 * @file AuditLogDialog.cpp
 * @brief 审计日志查看对话框实现
 *
 * 表格展示：时间 / 用户 / 操作 / 对象 / 详情，倒序最近 500 条。
 * 仅供超级管理员/总管查看（入口已在主窗口按权限控制）。
 */

#include "AuditLogDialog.h"
#include "core/utils/AppDimensions.h"
#include "core/utils/AppTheme.h"
#include "data/repositories/AuditRepository.h"
#include "core/models/AuditLog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>

AuditLogDialog::AuditLogDialog(QWidget* parent)
    : BaseDialog(parent)
    , m_table(nullptr)
{
    setWindowTitle(tr("审计日志"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels(
        {tr("时间"), tr("用户"), tr("操作"), tr("对象"), tr("详情")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    layout->addWidget(m_table, 1);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch(1);
    auto* refreshBtn = new QPushButton(tr("刷新"), this);
    refreshBtn->setProperty("variant", "secondary");
    auto* closeBtn = new QPushButton(tr("关闭"), this);
    btnRow->addWidget(refreshBtn);
    btnRow->addWidget(closeBtn);
    layout->addLayout(btnRow);

    connect(refreshBtn, &QPushButton::clicked, this, &AuditLogDialog::refresh);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    resize(AppDimensions::Window::DialogLargeWidth, AppDimensions::Window::DialogMediumHeight);
    refresh();
}

void AuditLogDialog::refresh()
{
    const QList<AuditLog::Ptr> logs = AuditRepository::listRecent(500);
    m_table->setRowCount(logs.size());
    for (int row = 0; row < logs.size(); ++row) {
        const AuditLog::Ptr& log = logs[row];
        m_table->setItem(row, 0, new QTableWidgetItem(
            log->createdAt().toString("yyyy-MM-dd HH:mm:ss")));
        m_table->setItem(row, 1, new QTableWidgetItem(log->username()));
        m_table->setItem(row, 2, new QTableWidgetItem(log->action()));
        m_table->setItem(row, 3, new QTableWidgetItem(
            log->reportId() > 0 ? QString::number(log->reportId()) : QString()));
        m_table->setItem(row, 4, new QTableWidgetItem(log->detail()));
    }
}
