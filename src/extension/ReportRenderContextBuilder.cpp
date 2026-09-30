#include "ReportRenderContextBuilder.h"

#include "core/models/Report.h"
#include "core/models/DataTable.h"
#include "core/models/User.h"
#include "export/HtmlGenerator.h"
#include "export/ExportManager.h"
#include "service/DataTableService.h"
#include "service/UserService.h"

#include <QTextDocument>

static QString typeNameCn(ColumnType t)
{
    switch (t) {
    case ColumnType::Number:  return QStringLiteral("数值");
    case ColumnType::Date:    return QStringLiteral("日期");
    case ColumnType::Boolean: return QStringLiteral("布尔");
    default:                  return QStringLiteral("文本");
    }
}

ReportRenderContext buildReportRenderContext(const Report::Ptr& report)
{
    ReportRenderContext ctx;
    if (!report) return ctx;

    ctx.title = report->title();
    ctx.statusName = report->statusDisplayName();
    ctx.createdAt = report->createdAt().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    ctx.updatedAt = report->updatedAt().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));

    // 创建者用户名（主线程查询安全）
    const User::Ptr creator = UserService::getById(report->createdBy());
    ctx.creator = creator ? creator->displayName() : QStringLiteral("用户#%1").arg(report->createdBy());

    // 正文 HTML + 纯文本（HtmlGenerator 现成渲染管线）
    {
        HtmlGenerator gen;
        ExportConfig cfg;
        ctx.htmlBody = gen.generateBody(report, cfg);
        QTextDocument doc;
        doc.setHtml(ctx.htmlBody);
        ctx.plainText = doc.toPlainText();
    }

    // 关联数据表（快照，纯 Qt 类型）
    const DataTable::List tables = DataTableService::findByReport(report->id());
    for (const DataTable::Ptr& t : tables) {
        if (!t) continue;
        ExportTable et;
        et.name = t->name();
        const QList<ColumnDefinition>& cols = t->columns();
        for (const ColumnDefinition& col : cols) {
            QString h = col.name;
            if (!col.unit.trimmed().isEmpty())
                h += QStringLiteral(" (%1)").arg(col.unit.trimmed());
            et.headers.append(h);
            et.columnTypes.append(typeNameCn(col.type));
        }
        const QList<QVariantList>& rows = t->rows();
        for (const QVariantList& row : rows) {
            QStringList cells;
            for (int c = 0; c < qMax(cols.size(), row.size()); ++c) {
                cells.append(c < row.size() ? row.at(c).toString() : QString());
            }
            et.rows.append(cells);
        }
        ctx.tables.append(et);
    }

    return ctx;
}
