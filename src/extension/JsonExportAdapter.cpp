#include "JsonExportAdapter.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

bool JsonExportAdapter::exportReport(const ReportRenderContext& ctx,
                                     const QString& targetPath,
                                     std::function<void(int)> progress)
{
    QJsonObject root;
    root["title"] = ctx.title;
    root["creator"] = ctx.creator;
    root["status"] = ctx.statusName;
    root["createdAt"] = ctx.createdAt;
    root["updatedAt"] = ctx.updatedAt;
    root["content"] = ctx.plainText;

    QJsonArray tables;
    for (const ExportTable& t : ctx.tables) {
        QJsonObject tObj;
        tObj["name"] = t.name;
        tObj["columns"] = QJsonArray::fromStringList(t.headers);
        tObj["columnTypes"] = QJsonArray::fromStringList(t.columnTypes);
        QJsonArray rows;
        for (const QStringList& row : t.rows) {
            QJsonArray cells;
            for (const QString& v : row) cells.append(v);
            rows.append(cells);
        }
        tObj["rows"] = rows;
        tables.append(tObj);
    }
    root["tables"] = tables;

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    if (progress) progress(100);
    return true;
}
