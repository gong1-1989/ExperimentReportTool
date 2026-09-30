#include "MarkdownExportAdapter.h"

#include <QFile>
#include <QTextStream>

// Markdown 管道表格中的 | 需转义为 \|
static QString escapeMd(const QString& s)
{
    QString r = s;
    r.replace('|', QStringLiteral("\\|"));
    return r;
}

bool MarkdownExportAdapter::exportReport(const ReportRenderContext& ctx,
                                         const QString& targetPath,
                                         std::function<void(int)> progress)
{
    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QStringList lines;
    lines << QStringLiteral("# %1").arg(ctx.title.isEmpty() ? QStringLiteral("未命名报告") : ctx.title);
    lines << QStringLiteral("");
    lines << QStringLiteral("- 创建者：%1").arg(ctx.creator);
    lines << QStringLiteral("- 状态：%1").arg(ctx.statusName);
    lines << QStringLiteral("- 创建时间：%1").arg(ctx.createdAt);
    lines << QStringLiteral("- 更新时间：%1").arg(ctx.updatedAt);
    lines << QStringLiteral("");

    if (!ctx.plainText.trimmed().isEmpty()) {
        lines << ctx.plainText.trimmed();
        lines << QStringLiteral("");
    }

    for (const ExportTable& t : ctx.tables) {
        lines << QStringLiteral("## 数据表：%1").arg(t.name);
        lines << QStringLiteral("");
        if (t.headers.isEmpty()) { lines << QStringLiteral("（空表）"); lines << QStringLiteral(""); continue; }
        // 表头行 + 分隔行
        QStringList headCells;
        for (const QString& h : t.headers) headCells << escapeMd(h);
        lines << QStringLiteral("| %1 |").arg(headCells.join(QStringLiteral(" | ")));
        QStringList sepCells;
        for (int i = 0; i < t.headers.size(); ++i) sepCells << QStringLiteral("---");
        lines << QStringLiteral("| %1 |").arg(sepCells.join(QStringLiteral(" | ")));
        for (const QStringList& row : t.rows) {
            QStringList cells;
            for (const QString& v : row) cells << escapeMd(v);
            lines << QStringLiteral("| %1 |").arg(cells.join(QStringLiteral(" | ")));
        }
        lines << QStringLiteral("");
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << lines.join(QStringLiteral("\n"));
    file.close();

    if (progress) progress(100);
    return true;
}
