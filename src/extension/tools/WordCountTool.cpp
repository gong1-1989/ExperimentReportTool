#include "WordCountTool.h"

DocumentToolResult WordCountTool::execute(const DocumentContext& ctx, QString* error) const
{
    Q_UNUSED(error);
    DocumentToolResult r;

    const QString text = ctx.text;
    // 字数：不含空白字符数（中文按字、英文按字母计数）
    int nonSpace = 0;
    for (const QChar& ch : text) {
        if (!ch.isSpace()) ++nonSpace;
    }
    // 段落数：按换行分割的非空段
    int paragraphs = 0;
    for (const QString& line : text.split(QLatin1Char('\n'))) {
        if (!line.trimmed().isEmpty()) ++paragraphs;
    }
    // 表格数（调用方未提供时不显示该项）
    if (ctx.tableCount > 0)
        r.stats.insert(QStringLiteral("数据表"), ctx.tableCount);

    r.stats.insert(QStringLiteral("字数"), nonSpace);
    r.stats.insert(QStringLiteral("字符数"), text.length());
    r.stats.insert(QStringLiteral("段落数"), paragraphs);

    QString msg = QStringLiteral("全文约 %1 字、%2 字符、%3 段")
                      .arg(nonSpace).arg(text.length()).arg(paragraphs);
    if (ctx.tableCount > 0) msg += QStringLiteral("、%1 个数据表").arg(ctx.tableCount);
    r.messages << msg;
    return r;
}
