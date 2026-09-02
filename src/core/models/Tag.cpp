/**
 * @file Tag.cpp
 * @brief 标签实体实现文件
 */

#include "Tag.h"
#include <QHash>
#include <QStringList>

// ===========================================================================
// 构造与析构
// ===========================================================================

Tag::Tag()
    : m_id(-1)
    , m_name("")
    , m_color("")
    , m_description("")
    , m_usageCount(0)
{
    m_createdAt = QDateTime::currentDateTime();
}

Tag::~Tag()
{
}

// ===========================================================================
// 工厂方法
// ===========================================================================

Tag::Ptr Tag::create(const QString& name)
{
    Ptr tag(new Tag());
    tag->setName(name);
    return tag;
}

// ===========================================================================
// 颜色处理
// ===========================================================================

QColor Tag::effectiveColor() const
{
    if (!m_color.isEmpty()) {
        return QColor(m_color);
    }

    // 根据名称生成默认颜色
    static const QStringList defaultColors = {
        "#4A90D9", "#52c41a", "#faad14", "#f5222d",
        "#722ed1", "#13c2c2", "#eb2f96", "#fa8c16",
        "#2f54eb", "#a0d911"
    };

    const uint hash = qHash(m_name);
    const int index = hash % defaultColors.size();
    return QColor(defaultColors.at(index));
}

// ===========================================================================
// 预设颜色
// ===========================================================================

QStringList Tag::presetColors()
{
    return {
        "#4A90D9",  // 蓝
        "#52c41a",  // 绿
        "#faad14",  // 黄
        "#f5222d",  // 红
        "#722ed1",  // 紫
        "#13c2c2",  // 青
        "#eb2f96",  // 粉
        "#fa8c16",  // 橙
        "#2f54eb",  // 深蓝
        "#a0d911",  // 黄绿
        "#8c8c8c",  // 灰
        "#000000"   // 黑
    };
}
