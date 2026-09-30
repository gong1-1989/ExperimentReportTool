#include "ExcelExportAdapter.h"

#include <QFile>
#include <QDateTime>
#include <QDataStream>

// ===========================================================================
// 最小 xlsx 生成器（stored zip + inlineStr）
// ===========================================================================

namespace {

// ---- CRC32（查表法） ----
quint32 g_crcTable[256];
bool g_crcInit = false;

void initCrcTable()
{
    for (quint32 i = 0; i < 256; ++i) {
        quint32 c = i;
        for (int k = 0; k < 8; ++k)
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        g_crcTable[i] = c;
    }
    g_crcInit = true;
}

quint32 crc32(const QByteArray& data)
{
    if (!g_crcInit) initCrcTable();
    quint32 c = 0xFFFFFFFFu;
    for (char ch : data)
        c = g_crcTable[(c ^ static_cast<uchar>(ch)) & 0xFFu] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

// ---- MS-DOS 时间戳（本地时间） ----
quint32 dosDateTime()
{
    const QDateTime now = QDateTime::currentDateTime();
    const quint32 d = (static_cast<quint32>(now.date().year() - 1980) << 9)
                    | (static_cast<quint32>(now.date().month()) << 5)
                    | static_cast<quint32>(now.date().day());
    const quint32 t = (static_cast<quint32>(now.time().hour()) << 11)
                    | (static_cast<quint32>(now.time().minute()) << 5)
                    | (static_cast<quint32>(now.time().second()) / 2);
    return (d << 16) | t;
}

// ---- zip 条目 ----
struct ZipEntry { QString name; QByteArray data; };

QByteArray buildZip(const QList<ZipEntry>& entries)
{
    QByteArray out;
    QList<quint32> offsets;
    quint32 cdOffset = 0;

    for (const ZipEntry& e : entries) {
        const QByteArray nameBytes = e.name.toUtf8();
        const quint32 crc = crc32(e.data);
        const quint32 size = static_cast<quint32>(e.data.size());
        const quint32 dos = dosDateTime();

        offsets.append(cdOffset);
        cdOffset += 30 + nameBytes.size() + size;

        // local file header
        QByteArray hdr;
        QDataStream ds(&hdr, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::LittleEndian);
        ds << quint32(0x04034b50u);              // PK\x03\x04
        ds << quint16(20);                       // version needed
        ds << quint16(0);                        // flags
        ds << quint16(0);                        // method: stored
        ds << quint32(dos);                      // time+date
        ds << quint32(crc);
        ds << quint32(size);                     // compressed size
        ds << quint32(size);                     // uncompressed size
        ds << quint16(static_cast<quint16>(nameBytes.size()));
        ds << quint16(0);                        // extra len
        out.append(hdr);
        out.append(nameBytes);
        out.append(e.data);
    }

    // central directory
    QByteArray cd;
    for (int i = 0; i < entries.size(); ++i) {
        const QByteArray nameBytes = entries[i].name.toUtf8();
        const quint32 crc = crc32(entries[i].data);
        const quint32 size = static_cast<quint32>(entries[i].data.size());
        const quint32 dos = dosDateTime();

        QByteArray hdr;
        QDataStream ds(&hdr, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::LittleEndian);
        ds << quint32(0x02014b50u);              // PK\x01\x02
        ds << quint16(20);                       // version made by
        ds << quint16(20);                       // version needed
        ds << quint16(0);                        // flags
        ds << quint16(0);                        // method
        ds << quint32(dos);
        ds << quint32(crc);
        ds << quint32(size);
        ds << quint32(size);
        ds << quint16(static_cast<quint16>(nameBytes.size()));
        ds << quint16(0);                        // extra
        ds << quint16(0);                        // comment
        ds << quint16(0);                        // disk
        ds << quint16(0);                        // internal attrs
        ds << quint32(0);                        // external attrs
        ds << quint32(offsets[i]);
        cd.append(hdr);
        cd.append(nameBytes);
    }
    const quint32 cdSize = static_cast<quint32>(cd.size());

    // end of central directory
    QByteArray eocd;
    QDataStream ds(&eocd, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds << quint32(0x06054b50u);                  // PK\x05\x06
    ds << quint16(0); ds << quint16(0);          // disk numbers
    ds << quint16(static_cast<quint16>(entries.size()));
    ds << quint16(static_cast<quint16>(entries.size()));
    ds << quint32(cdSize);
    ds << quint32(cdOffset);
    ds << quint16(0);

    out.append(cd);
    out.append(eocd);
    return out;
}

// ---- XML 转义 ----
QString xmlEsc(const QString& s)
{
    QString r = s;
    r.replace('&', QStringLiteral("&amp;"));
    r.replace('<', QStringLiteral("&lt;"));
    r.replace('>', QStringLiteral("&gt;"));
    r.replace('"', QStringLiteral("&quot;"));
    return r;
}

// ---- 列号（1-based → A, B, ..., AA） ----
QString colName(int idx)
{
    QString r;
    while (idx > 0) {
        r.prepend(QChar('A' + (idx - 1) % 26));
        idx = (idx - 1) / 26;
    }
    return r;
}

// ---- sheet 名清洗 ----
QString cleanSheetName(QString name)
{
    if (name.trimmed().isEmpty()) return QStringLiteral("数据表");
    name = name.trimmed();
    name.replace(QStringLiteral("["), QStringLiteral("_"));
    name.replace(QStringLiteral("]"), QStringLiteral("_"));
    name.replace('*', QChar('_')).replace('?', QChar('_'))
        .replace('/', QChar('_')).replace('\\', QChar('_'))
        .replace(':', QChar('_'));
    if (name.size() > 31) name = name.left(31);
    return name;
}

// ---- 单元格（inlineStr） ----
QString cellXml(const QString& ref, const QString& value)
{
    return QStringLiteral("<c r=\"%1\" t=\"inlineStr\"><is><t xml:space=\"preserve\">%2</t></is></c>")
        .arg(ref, xmlEsc(value));
}

QString rowXml(int row, const QStringList& values)
{
    QString xml = QStringLiteral("<row r=\"%1\">").arg(row);
    for (int c = 0; c < values.size(); ++c) {
        xml += cellXml(QStringLiteral("%1%2").arg(colName(c + 1)).arg(row), values.at(c));
    }
    xml += QStringLiteral("</row>");
    return xml;
}

QString sheetXml(const QStringList& headerRow, const QList<QStringList>& dataRows)
{
    QString xml = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\"><sheetData>");
    int row = 1;
    if (!headerRow.isEmpty()) {
        xml += rowXml(row, headerRow);
        ++row;
    }
    for (const QStringList& r : dataRows) {
        xml += rowXml(row, r);
        ++row;
    }
    xml += QStringLiteral("</sheetData></worksheet>");
    return xml;
}

} // namespace

// ===========================================================================
// ExcelExportAdapter
// ===========================================================================

bool ExcelExportAdapter::exportReport(const ReportRenderContext& ctx,
                                      const QString& targetPath,
                                      std::function<void(int)> progress)
{
    // --- 报告信息 sheet ---
    QStringList infoHeader = {QStringLiteral("字段"), QStringLiteral("值")};
    QString body = ctx.plainText.trimmed();
    if (body.size() > 32000) body = body.left(32000) + QStringLiteral("…（正文过长已截断）");
    QList<QStringList> infoRows;
    infoRows << (QStringList{QStringLiteral("标题"), ctx.title.isEmpty() ? QStringLiteral("未命名报告") : ctx.title});
    infoRows << (QStringList{QStringLiteral("创建者"), ctx.creator});
    infoRows << (QStringList{QStringLiteral("状态"), ctx.statusName});
    infoRows << (QStringList{QStringLiteral("创建时间"), ctx.createdAt});
    infoRows << (QStringList{QStringLiteral("更新时间"), ctx.updatedAt});
    if (!body.isEmpty()) infoRows << (QStringList{QStringLiteral("正文"), body});

    // --- 数据表 sheets ---
    QList<ZipEntry> entries;
    entries << ZipEntry{QStringLiteral("[Content_Types].xml"), QByteArray()};
    entries << ZipEntry{QStringLiteral("_rels/.rels"), QByteArray()};
    entries << ZipEntry{QStringLiteral("xl/workbook.xml"), QByteArray()};
    entries << ZipEntry{QStringLiteral("xl/_rels/workbook.xml.rels"), QByteArray()};

    QByteArray contentTypes = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>").toUtf8();
    QByteArray workbookRels = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">").toUtf8();
    QByteArray workbook = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"><sheets>").toUtf8();

    auto appendSheet = [&](int sheetIdx, const QString& sheetName,
                           const QStringList& headerRow, const QList<QStringList>& dataRows) {
        entries << ZipEntry{QStringLiteral("xl/worksheets/sheet%1.xml").arg(sheetIdx),
                            sheetXml(headerRow, dataRows).toUtf8()};
        contentTypes += QStringLiteral("<Override PartName=\"/xl/worksheets/sheet%1.xml\" "
            "ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>").arg(sheetIdx).toUtf8();
        workbookRels += QStringLiteral("<Relationship Id=\"rId%1\" "
            "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" "
            "Target=\"worksheets/sheet%1.xml\"/>").arg(sheetIdx).toUtf8();
        workbook += QStringLiteral("<sheet name=\"%1\" sheetId=\"%2\" r:id=\"rId%2\"/>")
            .arg(xmlEsc(sheetName)).arg(sheetIdx).toUtf8();
    };

    appendSheet(1, QStringLiteral("报告信息"), infoHeader, infoRows);

    for (int i = 0; i < ctx.tables.size(); ++i) {
        const ExportTable& t = ctx.tables.at(i);
        appendSheet(i + 2, cleanSheetName(t.name), t.headers, t.rows);
        if (progress) progress(10 + 80 * (i + 1) / qMax(1, ctx.tables.size()));
    }

    contentTypes += "</Types>";
    workbookRels += "</Relationships>";
    workbook += "</sheets></workbook>";

    // 首部三个条目数据回填
    for (ZipEntry& e : entries) {
        if (e.name == QStringLiteral("[Content_Types].xml")) e.data = contentTypes;
        else if (e.name == QStringLiteral("_rels/.rels")) e.data = QStringLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
            "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" "
            "Target=\"xl/workbook.xml\"/></Relationships>").toUtf8();
        else if (e.name == QStringLiteral("xl/workbook.xml")) e.data = workbook;
        else if (e.name == QStringLiteral("xl/_rels/workbook.xml.rels")) e.data = workbookRels;
    }

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(buildZip(entries));
    file.close();

    if (progress) progress(100);
    return true;
}
