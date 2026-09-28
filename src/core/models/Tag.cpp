/**
 * @file Tag.cpp
 * @brief 标签实体实现文件
 */

#include "Tag.h"
#include "core/utils/AppTheme.h"
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

    // 根据名称生成默认颜色（使用 AppTheme 中的预设颜色）
    const uint hash = qHash(m_name);
    const int index = hash % AppTheme::TagColors::Count;
    return QColor(AppTheme::TagColors::Presets[index]);
}

// ===========================================================================
// 预设颜色
// ===========================================================================

QStringList Tag::presetColors()
{
    QStringList colors;
    for (int i = 0; i < AppTheme::TagColors::Count; ++i) {
        colors.append(QString::fromLatin1(AppTheme::TagColors::Presets[i]));
    }
    return colors;
}
