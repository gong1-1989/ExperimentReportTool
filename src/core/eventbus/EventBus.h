/**
 * @file EventBus.h
 * @brief 事件总线头文件
 *
 * 事件总线用于插件之间、插件与主程序之间的解耦通信。
 * 基于 Qt 的信号槽机制实现。
 */

#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include <QObject>
#include <QVariant>
#include <QString>
#include <QHash>
#include <QList>

/**
 * @brief 事件总线类
 *
 * 使用方式：
 * @code
 *   // 发送事件
 *   EventBus::instance().publish("report.saved", reportId);
 *
 *   // 订阅事件
 *   EventBus::instance().subscribe("report.saved", this, &MyClass::onReportSaved);
 * @endcode
 */
class EventBus : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     * @return 事件总线实例
     */
    static EventBus& instance();

    /**
     * @brief 发布事件
     * @param eventType 事件类型字符串
     * @param data 事件数据
     */
    void publish(const QString& eventType, const QVariant& data = QVariant());

    /**
     * @brief 订阅事件
     * @param eventType 事件类型
     * @param receiver 接收者
     * @param member 槽函数（使用 SLOT() 宏）
     */
    void subscribe(const QString& eventType, QObject* receiver, const char* member);

    /**
     * @brief 取消订阅
     * @param eventType 事件类型
     * @param receiver 接收者
     */
    void unsubscribe(const QString& eventType, QObject* receiver);

signals:
    /**
     * @brief 通用事件信号
     * @param eventType 事件类型
     * @param data 事件数据
     */
    void event(const QString& eventType, const QVariant& data);

private:
    /// 私有构造函数（单例）
    explicit EventBus(QObject* parent = nullptr);

    /// 事件订阅映射
    QHash<QString, QList<QObject*>> m_subscribers;
};

#endif // EVENT_BUS_H
