/**
 * @file EventBus.cpp
 * @brief 事件总线实现文件
 */

#include "EventBus.h"

// ============================================================================
// 单例
// ============================================================================

EventBus& EventBus::instance()
{
    static EventBus inst;
    return inst;
}

EventBus::EventBus(QObject* parent)
    : QObject(parent)
{
}

// ============================================================================
// 发布与订阅
// ============================================================================

void EventBus::publish(const QString& eventType, const QVariant& data)
{
    emit event(eventType, data);
}

void EventBus::subscribe(const QString& eventType, QObject* receiver, const char* member)
{
    // 连接到通用事件信号（使用字符串语法，因为 member 是 const char*）
    connect(this, SIGNAL(event(QString,QVariant)), receiver, member);

    // 记录订阅者
    if (!m_subscribers.contains(eventType)) {
        m_subscribers[eventType] = QList<QObject*>();
    }
    if (!m_subscribers[eventType].contains(receiver)) {
        m_subscribers[eventType].append(receiver);
    }
}

void EventBus::unsubscribe(const QString& eventType, QObject* receiver)
{
    if (m_subscribers.contains(eventType)) {
        m_subscribers[eventType].removeAll(receiver);
    }
    // 注意：Qt 的信号槽连接需要 receiver 自己 disconnect
}
