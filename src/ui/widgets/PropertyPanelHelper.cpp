#include "ui/widgets/PropertyPanelHelper.h"
#include "core/utils/AppTheme.h"
#include "service/ReportService.h"
#include "service/ProjectService.h"
#include "service/UserService.h"
#include "service/TagService.h"
#include "core/utils/Logger.h"

#include <QObject>
#include <QCoreApplication>
#include <QColor>

// ===========================================================================
// 报告属性 HTML
// ===========================================================================

QString PropertyPanelHelper::reportHtml(const Report::Ptr& report)
{
    if (!report) {
        return emptyHtml();
    }

    // 状态字符串（统一走模型方法，避免重复 switch）
    const QString statusStr = report->statusDisplayName();

    // 项目名称
    QString projectName = QCoreApplication::translate("PropertyPanelHelper", "未分类");
    if (report->projectId() > 0) {
        const Project::Ptr project = ProjectService::getById(report->projectId());
        if (project) projectName = project->name();
    }

    // 标签
    const Tag::List tags = TagService::findByReport(report->id());
    const QString tagsHtml = PropertyPanelHelper::tagsHtml(tags);

    // 创建者 + 修改者：批量一次查询（消除 2 次单查）
    QString creatorName = QCoreApplication::translate("PropertyPanelHelper", "未分配");
    QString modifierName = QCoreApplication::translate("PropertyPanelHelper", "未分配");
    {
        QList<qint64> userIds;
        if (report->createdBy() > 0) userIds.append(report->createdBy());
        if (report->modifiedBy() > 0 && report->modifiedBy() != report->createdBy()) {
            userIds.append(report->modifiedBy());
        }
        const QMap<qint64, QString> userNames = UserService::batchDisplayNames(userIds);
        if (report->createdBy() > 0) {
            creatorName = userNames.value(report->createdBy(),
                QCoreApplication::translate("PropertyPanelHelper", "未知用户"));
        }
        if (report->modifiedBy() > 0) {
            modifierName = userNames.value(report->modifiedBy(),
                QCoreApplication::translate("PropertyPanelHelper", "未知用户"));
        }
    }

    return QString(
        "<div style='font-size: %1px; line-height: 1.8;'>"
        "<p><b style='color: %2;'>报告属性</b></p>"
        "<p><b>标题：</b>%3</p>"
        "<p><b>状态：</b>%4</p>"
        "<p><b>项目：</b>%5</p>"
        "<p><b>创建者：</b>%6</p>"
        "<p><b>修改者：</b>%13</p>"
        "<p><b>实验日期：</b>%7</p>"
        "<p><b>更新时间：</b>%8</p>"
        "<p><b>字数：</b>%9 字</p>"
        "<p><b>内容块：</b>%10 个</p>"
        "<p><b>标签：</b>%11</p>"
        "<p><b>报告ID：</b>%12</p>"
        "</div>"
    ).arg(AppTheme::FontSize::Small)
     .arg(AppTheme::Color::Primary)
     .arg(report->title().isEmpty() ? QCoreApplication::translate("PropertyPanelHelper", "未命名") : report->title().toHtmlEscaped())
     .arg(statusStr)
     .arg(projectName.toHtmlEscaped())
     .arg(creatorName.toHtmlEscaped())
     .arg(report->experimentDate().isValid() ? report->experimentDate().toString("yyyy-MM-dd") : QCoreApplication::translate("PropertyPanelHelper", "未设置"))
     .arg(report->updatedAt().isValid() ? report->updatedAt().toString("yyyy-MM-dd HH:mm") : QCoreApplication::translate("PropertyPanelHelper", "未知"))
     .arg(QString::number(report->wordCount()))
     .arg(QString::number(report->objectCount()))
     .arg(tagsHtml)
     .arg(QString::number(report->id()))
     .arg(modifierName.toHtmlEscaped());
}

// ===========================================================================
// 项目属性 HTML
// ===========================================================================

QString PropertyPanelHelper::projectHtml(const Project::Ptr& project)
{
    if (!project) {
        return emptyHtml();
    }

    const int reportCount = ReportService::count(project->id());

    return QString(
        "<div style='font-size: %1px; line-height: 1.8;'>"
        "<p><b style='color: %2;'>项目属性</b></p>"
        "<p><b>名称：</b>%3</p>"
        "<p><b>描述：</b>%4</p>"
        "<p><b>报告数：</b>%5 份</p>"
        "<p><b>创建时间：</b>%6</p>"
        "<p><b>项目ID：</b>%7</p>"
        "</div>"
    ).arg(AppTheme::FontSize::Small)
     .arg(AppTheme::Color::Success)
     .arg(project->name().toHtmlEscaped())
     .arg(project->description().isEmpty() ? QCoreApplication::translate("PropertyPanelHelper", "无描述") : project->description().toHtmlEscaped())
     .arg(QString::number(reportCount))
     .arg(project->createdAt().isValid() ? project->createdAt().toString("yyyy-MM-dd HH:mm") : QCoreApplication::translate("PropertyPanelHelper", "未知"))
     .arg(QString::number(project->id()));
}

// ===========================================================================
// 占位提示 HTML
// ===========================================================================

QString PropertyPanelHelper::emptyHtml()
{
    return QString(
        "<div style='color: %1; font-size: %2px; margin-top: %3px;'>"
        "选择项目或报告后，<br>此处将显示其属性信息。"
        "</div>").arg(AppTheme::Color::TextSecondary)
                  .arg(AppTheme::FontSize::Small)
                  .arg(AppTheme::Spacing::Large);
}

// ===========================================================================
// 标签 HTML
// ===========================================================================

QString PropertyPanelHelper::tagsHtml(const Tag::List& tags)
{
    if (tags.isEmpty()) {
        return QCoreApplication::translate("PropertyPanelHelper", "无标签");
    }

    QString html;
    for (int i = 0; i < tags.size(); ++i) {
        const QColor color = tags[i]->effectiveColor();
        if (i > 0) html += "&nbsp;&nbsp;";
        html += QString("<span style='color: %1;'>■</span>&nbsp;%2")
                    .arg(color.name())
                    .arg(tags[i]->name().toHtmlEscaped());
    }
    return html;
}
