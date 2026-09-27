/**
 * @file PluginManagerDialog.cpp
 * @brief 插件管理对话框实现文件
 */

#include "PluginManagerDialog.h"
#include "ui_PluginManagerDialog.h"
#include "core/plugin/EditorBlockPluginInterface.h"
#include "core/plugin/ExportPluginInterface.h"
#include "core/plugin/ToolPluginInterface.h"
#include "core/plugin/ImportPluginInterface.h"

#include <QHeaderView>
#include <QTableWidgetItem>

// ============================================================================
// 构造与析构
// ============================================================================

PluginManagerDialog::PluginManagerDialog(PluginManager* pluginManager, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::PluginManagerDialog)
    , m_pluginManager(pluginManager)
{
    ui->setupUi(this);

    // 设置表格列宽
    ui->pluginTable->setColumnWidth(0, 150);
    ui->pluginTable->setColumnWidth(1, 80);
    ui->pluginTable->setColumnWidth(2, 100);
    ui->pluginTable->setColumnWidth(3, 60);
    ui->pluginTable->horizontalHeader()->setStretchLastSection(true);

    // 槽函数通过 uic 自动连接（on_btnRefresh_clicked、on_pluginTable_cellClicked）

    // 加载插件列表
    refreshPluginList();
}

PluginManagerDialog::~PluginManagerDialog()
{
    delete ui;
}

// ============================================================================
// 刷新插件列表
// ============================================================================

void PluginManagerDialog::refreshPluginList()
{
    if (!m_pluginManager) return;

    const QList<PluginInterface*> plugins = m_pluginManager->loadedPlugins();
    ui->pluginTable->setRowCount(plugins.size());

    for (int i = 0; i < plugins.size(); ++i) {
        PluginInterface* plugin = plugins[i];

        // 插件名称
        auto* nameItem = new QTableWidgetItem(plugin->name());
        nameItem->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(plugin)));
        ui->pluginTable->setItem(i, 0, nameItem);

        // 版本
        ui->pluginTable->setItem(i, 1, new QTableWidgetItem(plugin->version()));

        // 类型
        ui->pluginTable->setItem(i, 2, new QTableWidgetItem(pluginTypeName(plugin)));

        // 状态
        ui->pluginTable->setItem(i, 3, new QTableWidgetItem(tr("已加载")));

        // 描述
        ui->pluginTable->setItem(i, 4, new QTableWidgetItem(plugin->description()));
    }

    // 清空详情
    ui->detailName->setText("-");
    ui->detailIid->setText("-");
    ui->detailAuthor->setText("-");
    ui->detailDesc->setText("-");
}

// ============================================================================
// 选中插件时显示详情
// ============================================================================

void PluginManagerDialog::on_btnRefresh_clicked()
{
    refreshPluginList();
}

void PluginManagerDialog::on_pluginTable_cellClicked(int row, int column)
{
    Q_UNUSED(column);
    if (row < 0 || !m_pluginManager) return;

    QTableWidgetItem* item = ui->pluginTable->item(row, 0);
    if (!item) return;

    auto* plugin = static_cast<PluginInterface*>(item->data(Qt::UserRole).value<void*>());
    if (!plugin) return;

    ui->detailName->setText(plugin->name());
    ui->detailIid->setText(plugin->iid());
    ui->detailAuthor->setText(plugin->author());
    ui->detailDesc->setText(plugin->description());
}

// ============================================================================
// 获取插件类型名称
// ============================================================================

QString PluginManagerDialog::pluginTypeName(PluginInterface* plugin) const
{
    if (dynamic_cast<EditorBlockPluginInterface*>(plugin)) return tr("编辑器块");
    if (dynamic_cast<ExportPluginInterface*>(plugin)) return tr("导出");
    if (dynamic_cast<ToolPluginInterface*>(plugin)) return tr("工具");
    if (dynamic_cast<ImportPluginInterface*>(plugin)) return tr("导入");
    return tr("基础");
}
