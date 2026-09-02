/**
 * @file AutoSaveManager.h
 * @brief 自动保存管理器头文件
 *
 * AutoSaveManager 负责报告的自动保存功能：
 * - 内容变化后防抖延迟保存（默认 3 秒）
 * - 保存状态指示（保存中 / 已保存 / 保存失败）
 * - 手动保存触发
 * - 版本快照（可选）
 */

#ifndef AUTO_SAVE_MANAGER_H
#define AUTO_SAVE_MANAGER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>

/**
 * @brief 自动保存管理器
 *
 * 使用方式：
 * @code
 *   AutoSaveManager* autoSave = new AutoSaveManager(this);
 *   autoSave->setAutoSaveInterval(3000);  // 3 秒
 *   connect(editor, &ReportEditor::contentChanged,
 *           autoSave, &AutoSaveManager::notifyContentChanged);
 *   connect(autoSave, &AutoSaveManager::saveTriggered,
 *           editor, &ReportEditor::saveToReport);
 * @endcode
 */
class AutoSaveManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 保存状态枚举
     */
    enum class SaveState {
        Idle,       ///< 空闲（已保存）
        Waiting,    ///< 等待防抖定时器
        Saving,     ///< 正在保存
        Failed      ///< 保存失败
    };

    explicit AutoSaveManager(QObject* parent = nullptr);
    ~AutoSaveManager() override;

    // -----------------------------------------------------------------------
    // 配置
    // -----------------------------------------------------------------------

    /// 设置自动保存间隔（毫秒）
    void setAutoSaveInterval(int ms) { m_autoSaveInterval = ms; }
    /// 获取自动保存间隔
    int autoSaveInterval() const { return m_autoSaveInterval; }

    /// 设置是否启用自动保存
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

    /// 设置是否在保存时创建版本快照
    void setVersionSnapshotEnabled(bool enabled) { m_versionSnapshot = enabled; }
    bool isVersionSnapshotEnabled() const { return m_versionSnapshot; }

    // -----------------------------------------------------------------------
    // 状态查询
    // -----------------------------------------------------------------------

    /// 当前保存状态
    SaveState saveState() const { return m_saveState; }

    /// 最后保存时间
    QDateTime lastSaveTime() const { return m_lastSaveTime; }

    /// 是否有未保存的更改
    bool hasUnsavedChanges() const { return m_saveState != SaveState::Idle; }

public slots:
    /**
     * @brief 通知内容变化（启动防抖定时器）
     *
     * 编辑器内容变化时调用此方法，会在延迟后触发 saveTriggered 信号。
     * 多次调用会重置定时器（防抖）。
     */
    void notifyContentChanged();

    /**
     * @brief 立即保存（跳过防抖等待）
     */
    void saveNow();

    /**
     * @brief 标记保存成功
     */
    void markSaveSuccess();

    /**
     * @brief 标记保存失败
     * @param error 错误信息
     */
    void markSaveFailed(const QString& error);

    /**
     * @brief 取消待执行的自动保存
     */
    void cancelPendingSave();

signals:
    /**
     * @brief 保存触发信号
     *
     * 连接到此信号的槽应执行实际的保存操作，
     * 保存完成后调用 markSaveSuccess() 或 markSaveFailed()。
     */
    void saveTriggered();

    /**
     * @brief 保存状态变化信号
     * @param saved 是否已保存（true = 已保存，false = 未保存/保存中）
     */
    void saveStateChanged(bool saved);

    /**
     * @brief 保存状态详细变化信号
     * @param state 新状态
     */
    void saveStateDetailedChanged(SaveState state);

    /**
     * @brief 保存失败信号
     * @param error 错误信息
     */
    void saveFailed(const QString& error);

private slots:
    /// 防抖定时器超时
    void onTimerTimeout();

private:
    /// 设置保存状态
    void setSaveState(SaveState state);

    // -----------------------------------------------------------------------
    // 成员变量
    // -----------------------------------------------------------------------

    QTimer* m_autoSaveTimer;     ///< 自动保存防抖定时器
    int m_autoSaveInterval;       ///< 自动保存间隔（毫秒）
    bool m_enabled;               ///< 是否启用自动保存
    bool m_versionSnapshot;       ///< 是否创建版本快照
    SaveState m_saveState;        ///< 当前保存状态
    QDateTime m_lastSaveTime;     ///< 最后保存时间
    int m_saveRetryCount;         ///< 保存失败重试次数
    static const int MAX_RETRY = 3;  ///< 最大重试次数
};

#endif // AUTO_SAVE_MANAGER_H
