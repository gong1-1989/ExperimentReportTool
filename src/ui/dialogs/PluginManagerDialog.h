/**
 * @file PluginManagerDialog.h
 * @brief 插件管理对话框头文件
 *
 * 显示已加载的插件列表，查看插件详情。
 */

#ifndef PLUGIN_MANAGER_DIALOG_H
#define PLUGIN_MANAGER_DIALOG_H

#include <QDialog>
#include "core/plugin/PluginManager.h"

namespace Ui {
class PluginManagerDialog;
}

/**
 * @brief 插件管理对话框
 */
class PluginManagerDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param pluginManager 插件管理器指针
     * @param parent 父窗口
     */
    explicit PluginManagerDialog(PluginManager* pluginManager, QWidget* parent = nullptr);

    /// 析构函数
    ~PluginManagerDialog() override;

private slots:
    /// 点击刷新按钮
    void on_btnRefresh_clicked();

    /// 当前选中的插件变化时更新详情
    void on_pluginTable_cellClicked(int row, int column);

private:
    Ui::PluginManagerDialog* ui;  ///< UI 界面对象
    PluginManager* m_pluginManager;  ///< 插件管理器

    /// 刷新插件列表
    void refreshPluginList();

    /// 获取插件类型的显示名称
    QString pluginTypeName(PluginInterface* plugin) const;
};

#endif // PLUGIN_MANAGER_DIALOG_H
