/**
 * @file VersionHistoryDialog.h
 * @brief 版本历史对话框头文件
 *
 * 展示报告的版本历史，支持版本预览、恢复、删除、保存新版本。
 */

#ifndef VERSION_HISTORY_DIALOG_H
#define VERSION_HISTORY_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTextBrowser>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>

#include "core/models/Report.h"

/**
 * @brief 版本信息结构体
 */
struct VersionInfo {
    qint64 versionId;       ///< 版本 ID
    qint64 reportId;        ///< 报告 ID
    QString snapshotName;    ///< 版本名称/备注
    QDateTime createdAt;     ///< 创建时间
    QString content;         ///< 版本内容（JSON）

    VersionInfo() : versionId(-1), reportId(-1) {}
};

/**
 * @brief 版本历史对话框
 */
class VersionHistoryDialog : public QDialog
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
    void onVersionSelected(QListWidgetItem* item);
    void onRestore();
    void onDelete();
    void onSaveNewVersion();
    void onRefresh();
    void onCompare();

private:
    void setupUi();
    void loadVersions();
    void displayVersion(const VersionInfo& version);
    QString formatVersionPreview(const QString& contentJson);
    QString extractPlainText(const QString& contentJson);
    void showStatusMessage(const QString& message);

    // -----------------------------------------------------------------------
    // UI 控件
    // -----------------------------------------------------------------------

    QListWidget* m_versionList;       ///< 版本列表
    QTextBrowser* m_previewBrowser;   ///< 版本预览
    QSplitter* m_splitter;             ///< 分割器

    QPushButton* m_restoreBtn;         ///< 恢复按钮
    QPushButton* m_deleteBtn;          ///< 删除按钮
    QPushButton* m_saveBtn;            ///< 保存新版本按钮
    QPushButton* m_compareBtn;         ///< 对比按钮
    QPushButton* m_refreshBtn;         ///< 刷新按钮
    QPushButton* m_closeBtn;           ///< 关闭按钮

    QLineEdit* m_versionNameEdit;      ///< 新版本名称输入
    QLabel* m_statusLabel;             ///< 状态标签

    // -----------------------------------------------------------------------
    // 数据
    // -----------------------------------------------------------------------

    qint64 m_reportId;                 ///< 报告 ID
    QList<VersionInfo> m_versions;     ///< 版本列表
    VersionInfo m_currentVersion;       ///< 当前选中的版本
};

#endif // VERSION_HISTORY_DIALOG_H
