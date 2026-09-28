/**
 * @file VersionHistoryDialog.h
 * @brief 版本历史对话框头文件
 *
 * 展示报告的版本历史，支持版本预览、恢复、删除、保存新版本。
 */

#ifndef VERSION_HISTORY_DIALOG_H
#define VERSION_HISTORY_DIALOG_H

#include "BaseDialog.h"
#include <QListWidget>
#include <QListWidgetItem>
#include <QTextBrowser>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QList>
#include <QDateTime>

#include "core/models/Report.h"
#include "print/PrintManager.h"
#include "data/repositories/ReportRepository.h"


// 前向声明 UI 类（由 uic 工具从 .ui 文件自动生成）
namespace Ui {
class VersionHistoryDialog;
}

/**
 * @brief 版本历史对话框
 */
class VersionHistoryDialog : public BaseDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param reportId 报告 ID
     * @param parent 父窗口
     */
    explicit VersionHistoryDialog(qint64 reportId, QWidget* parent = nullptr);
    ~VersionHistoryDialog() override;

signals:
    /**
     * @brief 版本已恢复信号
     * @param reportId 报告 ID
     * @param versionId 恢复的版本 ID
     */
    void versionRestored(qint64 reportId, qint64 versionId);

    /**
     * @brief 新版本已保存信号
     * @param reportId 报告 ID
     * @param versionId 新版本 ID
     */
    void versionSaved(qint64 reportId, qint64 versionId);

private slots:
    void on_m_versionList_itemClicked(QListWidgetItem* item);
    void on_m_restoreBtn_clicked();
    void on_m_deleteBtn_clicked();
    void on_m_saveBtn_clicked();
    void on_m_refreshBtn_clicked();
    void on_m_compareBtn_clicked();
    void on_m_closeBtn_clicked();

private:

    Ui::VersionHistoryDialog* ui;  ///< UI 界面对象（从 .ui 文件自动生成）
    void loadVersions();
    void displayVersion(const VersionInfo& version);
    QString renderVersionContent(const QString& contentJson);
    void showStatusMessage(const QString& message);

    // -----------------------------------------------------------------------
    // UI 控件
    // -----------------------------------------------------------------------




    // -----------------------------------------------------------------------
    // 数据
    // -----------------------------------------------------------------------

    qint64 m_reportId;                 ///< 报告 ID
    QList<VersionInfo> m_versions;     ///< 版本列表
    VersionInfo m_currentVersion;       ///< 当前选中的版本
};

#endif // VERSION_HISTORY_DIALOG_H
