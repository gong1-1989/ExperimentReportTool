/**
 * @file AutoSaveManager.cpp
 * @brief 自动保存管理器实现文件
 */

#include "AutoSaveManager.h"
#include "core/utils/Logger.h"
#include "core/utils/AppConstants.h"

// ===========================================================================
// 构造与析构
// ===========================================================================

AutoSaveManager::AutoSaveManager(QObject* parent)
    : QObject(parent)
    , m_autoSaveTimer(nullptr)
    , m_autoSaveInterval(AppConstants::AUTO_SAVE_INTERVAL_MS)
    , m_enabled(true)
    , m_versionSnapshot(false)
    , m_saveState(SaveState::Idle)
    , m_saveRetryCount(0)
{
    // 创建防抖定时器
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setSingleShot(true);  // 单次触发（防抖）
    m_autoSaveTimer->setInterval(m_autoSaveInterval);
    connect(m_autoSaveTimer, &QTimer::timeout,
            this, &AutoSaveManager::onTimerTimeout);
}

AutoSaveManager::~AutoSaveManager()
{
    if (m_autoSaveTimer->isActive()) {
        m_autoSaveTimer->stop();
    }
}

// ===========================================================================
// 公共槽函数
// ===========================================================================

void AutoSaveManager::notifyContentChanged()
{
    if (!m_enabled) return;

    // 重置防抖定时器（多次调用只触发一次保存）
    m_autoSaveTimer->stop();
    m_autoSaveTimer->start(m_autoSaveInterval);

    setSaveState(SaveState::Waiting);
}

void AutoSaveManager::saveNow()
{
    // 停止等待定时器，立即保存
    if (m_autoSaveTimer->isActive()) {
        m_autoSaveTimer->stop();
    }

    onTimerTimeout();
}

void AutoSaveManager::markSaveSuccess()
{
    m_saveRetryCount = 0;
    m_lastSaveTime = QDateTime::currentDateTime();
    setSaveState(SaveState::Idle);
    LOG_INFO("自动保存成功");
}

void AutoSaveManager::markSaveFailed(const QString& error)
{
    ++m_saveRetryCount;
    setSaveState(SaveState::Failed);
    LOG_ERROR(QString("自动保存失败: %1 (重试次数: %2)").arg(error).arg(m_saveRetryCount));
    emit saveFailed(error);

    // 如果未超过最大重试次数，延迟后重试
    if (m_saveRetryCount < MAX_RETRY) {
        QTimer::singleShot(2000, this, &AutoSaveManager::saveNow);
    }
}

void AutoSaveManager::cancelPendingSave()
{
    if (m_autoSaveTimer->isActive()) {
        m_autoSaveTimer->stop();
        setSaveState(SaveState::Idle);
    }
}

// ===========================================================================
// 私有槽函数
// ===========================================================================

void AutoSaveManager::onTimerTimeout()
{
    if (!m_enabled) return;

    setSaveState(SaveState::Saving);
    LOG_INFO("触发自动保存");
    emit saveTriggered();
}

// ===========================================================================
// 私有方法
// ===========================================================================

void AutoSaveManager::setSaveState(SaveState state)
{
    if (m_saveState == state) return;

    m_saveState = state;
    emit saveStateDetailedChanged(state);
    emit saveStateChanged(state == SaveState::Idle);
}
