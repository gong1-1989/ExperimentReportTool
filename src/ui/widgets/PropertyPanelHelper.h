#pragma once

#include "core/models/Report.h"
#include "core/models/Project.h"
#include "core/models/Tag.h"
#include "core/models/User.h"

/**
 * @brief 属性面板 HTML 生成工具（纯静态，无状态）
 *
 * 集中管理"报告属性 / 项目属性 / 标签"等富文本 HTML 的生成，
 * 供 MainWindow 属性面板、SearchResultDialog 标签展示等处复用，
 * 消除跨文件的重复格式化逻辑。
 */
class PropertyPanelHelper
{
public:
    /** 生成报告属性 HTML（状态/项目/创建者/标签内部解析） */
    static QString reportHtml(const Report::Ptr& report);

    /** 生成项目属性 HTML */
    static QString projectHtml(const Project::Ptr& project);

    /** 无选中对象时的占位提示 HTML */
    static QString emptyHtml();

    /** 生成标签的彩色圆点+名称 HTML（供报告属性、搜索结果复用） */
    static QString tagsHtml(const Tag::List& tags);
};
