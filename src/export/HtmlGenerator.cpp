/**
 * @file HtmlGenerator.cpp
 * @brief HTML 生成器实现文件
 *
 * 从 ExportManager 中分离，负责将报告内容转换为 HTML。
 */

#include "HtmlGenerator.h"
#include "ExportManager.h"
#include "export/ObjectRenderer.h"
#include "core/utils/Logger.h"
#include "service/DataTableService.h"
#include <QTextDocument>
#include <QTextCursor>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QTextList>
#include <QTextTable>
#include <QBuffer>
#include <QRegularExpression>
#include <QVector>
#include "chart/ChartRenderer.h"
#include "chart/ChartConfigDialog.h"
#include "core/models/DataTable.h"
#include "core/utils/AppConfig.h"
#include <QImage>
#include <QPixmap>
#include <QMimeDatabase>
#include <QMimeType>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>

// ===========================================================================
// 构造与析构
// ===========================================================================

HtmlGenerator::HtmlGenerator()
{
}

HtmlGenerator::~HtmlGenerator()
{
}

static QString htmlToPlainText(const QString& html)
{
    QTextDocument doc;
    doc.setHtml(html);
    return doc.toPlainText().trimmed();
}

/**
 * @brief 从完整 HTML 文档中提取 body 标签内的内容
 *
 * TextBlockEditor 存储的是 QTextEdit::toHtml() 的完整 HTML 文档，
 * 包含 <!DOCTYPE>、<html>、<head>、<body> 等标签。
 * 在导出时，需要提取 <body> 内的内容嵌入到主文档中，
 * 否则会显示为 HTML 源码。
 *
 * @param html 完整的 HTML 文档
 * @return body 标签内的 HTML 片段，如果不是完整文档则返回原文本
 */
static QString extractHtmlBody(const QString& html)
{
    if (html.isEmpty()) return QString();

    // 如果不包含 <body 标签，说明是 HTML 片段，直接返回
    if (!html.contains("<body", Qt::CaseInsensitive)) {
        return html;
    }

    // 查找 <body 开始位置
    const int bodyStart = html.indexOf("<body", 0, Qt::CaseInsensitive);
    if (bodyStart < 0) return html;

    // 查找 <body> 标签结束位置（> 字符）
    const int bodyTagEnd = html.indexOf('>', bodyStart);
    if (bodyTagEnd < 0) return html;

    // 查找 </body> 位置
    int bodyEnd = html.indexOf("</body>", bodyTagEnd, Qt::CaseInsensitive);
    if (bodyEnd < 0) {
        // 没有 </body>，取到文档末尾
        bodyEnd = html.size();
    }

    QString result = html.mid(bodyTagEnd + 1, bodyEnd - bodyTagEnd - 1).trimmed();

    // 如果提取结果为空，尝试用 QTextDocument 解析整个 HTML 并提取 body 内容
    if (result.isEmpty()) {
        QTextDocument doc;
        doc.setHtml(html);
        result = doc.toHtml();
        // 再次提取 body
        if (result.contains("<body", Qt::CaseInsensitive)) {
            const int s = result.indexOf("<body", 0, Qt::CaseInsensitive);
            const int e = result.indexOf('>', s);
            const int end = result.indexOf("</body>", e, Qt::CaseInsensitive);
            if (e >= 0 && end > e) {
                result = result.mid(e + 1, end - e - 1).trimmed();
            }
        }
    }

    return result;
}

/**
 * @brief 安全获取文本块的 HTML 内容
 *
 * 优先使用 text 字段（完整 HTML），提取 body 内容。
 * 如果提取结果为空，降级使用 plain_text 字段。
 *
 * @param block 内容块
 * @return HTML 片段
 */

// ===========================================================================
// HTML 生成
// ===========================================================================

QString HtmlGenerator::generate(const Report::Ptr& report, const ExportConfig& config)
{
    return reportToHtml(report, config);
}

QString HtmlGenerator::generateBody(const Report::Ptr& report, const ExportConfig& config)
{
    const QString fullHtml = reportToHtml(report, config);
    const int bodyStart = fullHtml.indexOf("<body>");
    const int bodyEnd = fullHtml.indexOf("</body>");
    if (bodyStart >= 0 && bodyEnd > bodyStart) {
        return fullHtml.mid(bodyStart + 6, bodyEnd - bodyStart - 6);
    }
    return fullHtml;
}

QString HtmlGenerator::reportToHtml(const Report::Ptr& report, const ExportConfig& config)
{
    LOG_DEBUG(QStringLiteral("导出HTML: title=%1 docLen=%2 objects=%3 format=%4")
                  .arg(report->title())
                  .arg(report->document().length())
                  .arg(report->objects().size())
                  .arg(static_cast<int>(config.format)));
    QString html;
    html += "<!DOCTYPE html>\n<html>\n<head>\n";
    html += "<meta charset=\"utf-8\">\n";
    html += QString("<title>%1</title>\n").arg(report->title().toHtmlEscaped());
    html += "<style>\n" + generateCss(config) + "\n</style>\n";
    html += "</head>\n<body>\n";

    // 标题
    if (config.includeTitle) {
        html += QString("<p class=\"report-title\">%1</p>\n")
                    .arg(report->title().toHtmlEscaped());
    }

    // 元信息
    if (config.includeMeta) {
        html += "<div class=\"report-meta\">\n";
        html += QString("<span class=\"meta-item\"><strong>创建者:</strong> %1</span>\n")
                    .arg(report->author().toHtmlEscaped());
        html += QString("<span class=\"meta-item\"><strong>实验日期:</strong> %1</span>\n")
                    .arg(report->experimentDate().toString("yyyy-MM-dd"));
        html += QString("<span class=\"meta-item\"><strong>创建时间:</strong> %1</span>\n")
                    .arg(report->createdAt().toString("yyyy-MM-dd hh:mm"));
        html += "</div>\n";
    }

    // 目录：从连续文档中提取标题（兼容旧 h 标签与新版 p 大字）
    if (config.includeTableOfContents) {
        html += "<div class=\"table-of-contents\">\n";
        html += "<p style=\"font-size:16pt;font-weight:bold;\">目录</p>\n<ul>\n";
        int tocIndex = 1;
        const QString docBody = extractHtmlBody(report->document());
        // 兼容旧版 h1/h2 标题与模板 v4 的 p 大字标题（font-size:20pt）
        const QRegularExpression headingRegex(
            "<(h[12]|p)([^>]*)>(.*?)</\\1>",
            QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator it = headingRegex.globalMatch(docBody);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            const QString tag = m.captured(1);
            const QString text = htmlToPlainText(m.captured(3)).trimmed();
            if (text.isEmpty()) continue;
            // 普通段落不算标题，仅带大字号样式的 p 才作为标题提取
            // （Qt 会把 p 标签的 font-size 移到内部 span，须检查完整匹配）
            if (tag == "p"
                && !m.captured(0).contains("font-size:20pt", Qt::CaseInsensitive)) {
                continue;
            }
            const int level = tag == "h2" ? 2 : 1;
            const QString indent = level == 2 ? "  " : "";
            html += QString("%1<li><a href=\"#heading-%2\">%3</a></li>\n")
                        .arg(indent).arg(tocIndex).arg(text.toHtmlEscaped());
            ++tocIndex;
        }
        html += "</ul>\n</div>\n";
    }

    // 正文内容：连续文档 + 对象锚点替换
    html += "<div class=\"report-content\">\n";
    QString contentHtml = extractHtmlBody(report->document());
    LOG_DEBUG(QStringLiteral("导出HTML: body=%1 anchorImgs=%2")
                  .arg(contentHtml.left(200))
                  .arg(contentHtml.count("object://")));
    if (contentHtml.isEmpty()) {
        contentHtml = "<p class=\"empty-content\">（报告内容为空）</p>\n";
    }

    // 对象锚点 → 对象渲染
    const QRegularExpression anchorRegex(
        "<img[^>]*src=\"object://([^/\"]+)/([^\"]+)\"[^>]*>");
    // 对象锚点段落 → 对齐方式映射（跟随编辑窗段落对齐设置）
    // Qt toHtml：左对齐不输出 align 属性（默认），居中/右对齐输出 align="center"/"right"
    QHash<QString, QString> anchorAligns;
    {
        const QRegularExpression pRe("<p([^>]*)>(.*?)</p>",
                                     QRegularExpression::DotMatchesEverythingOption);
        const QRegularExpression objRe("object://[a-z_0-9]+/([0-9a-fA-F-]+)");
        const QRegularExpression alignRe("align=[\"'](left|center|right|justify)[\"']",
                                         QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator pit = pRe.globalMatch(contentHtml);
        while (pit.hasNext()) {
            const QRegularExpressionMatch pm = pit.next();
            const QRegularExpressionMatch om = objRe.match(pm.captured(2));
            if (!om.hasMatch()) continue;
            QString align = QStringLiteral("left"); // 无 align 属性 = 左对齐
            const QRegularExpressionMatch am = alignRe.match(pm.captured(1));
            if (am.hasMatch()) align = am.captured(1);
            anchorAligns.insert(om.captured(1), align);
        }
    }
    const auto renderObject = [&](const QRegularExpressionMatch& m) -> QString {
        const QString objectId = m.captured(2);
        const QString align = anchorAligns.value(objectId, QStringLiteral("left"));
        ContentBlock object;
        const QList<ContentBlock>& objects = report->objects();
        for (const ContentBlock& obj : objects) {
            if (obj.id == objectId) { object = obj; break; }
        }
        if (object.id.isEmpty()) {
            return QString("<p class=\"empty-content\">[对象已不存在]</p>\n");
        }
        // 统一走共享渲染器（与打印同一套对象渲染逻辑，参数化导出差异）
        return ObjectRenderer::renderObject(object, align, ObjectRenderer::htmlExportOptions(),
                                            report->id(),
                                            [](qint64 tid) { return DataTableService::getById(tid); });
    };
    QRegularExpressionMatchIterator anchorIt = anchorRegex.globalMatch(contentHtml);
    QVector<QPair<int, int> > anchorSpans;
    QVector<QString> anchorHtmls;
    while (anchorIt.hasNext()) {
        const QRegularExpressionMatch m = anchorIt.next();
        anchorSpans.append(qMakePair(m.capturedStart(0), m.capturedLength(0)));
        anchorHtmls.append(renderObject(m));
    }
    for (int ai = anchorSpans.size() - 1; ai >= 0; --ai) {
        contentHtml.replace(anchorSpans.at(ai).first, anchorSpans.at(ai).second,
                            anchorHtmls.at(ai));
    }

    html += "</div>\n";

    // 页脚
    html += QString("<div class=\"report-footer\">由 %1 生成于 %2</div>\n")
                .arg("实验报告记录工具")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

    html += "</body>\n</html>";
    return html;
}

// ===========================================================================
// ===========================================================================

// ===========================================================================
// ===========================================================================

// ===========================================================================
// ===========================================================================

// ===========================================================================
// CSS 样式生成
// ===========================================================================

QString HtmlGenerator::generateCss(const ExportConfig& config)
{
    return QString(R"(
        body {
            font-family: "%1", sans-serif;
            font-size: %2px;
            line-height: 1.8;
            color: #333;
            max-width: 800px;
            margin: 0 auto;
            padding: 40px;
        }
        .report-title {
            text-align: center;
            font-size: 28px;
            color: #1a1a1a;
            border-bottom: 2px solid #4A90D9;
            padding-bottom: 16px;
            margin-bottom: 24px;
        }
        .report-meta {
            background: #f8f9fa;
            padding: 12px 16px;
            border-radius: 6px;
            margin-bottom: 24px;
            font-size: 13px;
            color: #666;
        }
        .report-meta .meta-item {
            display: inline-block;
            margin-right: 20px;
        }
        .table-of-contents {
            background: #f8f9fa;
            padding: 16px 24px;
            border-radius: 6px;
            margin-bottom: 24px;
        }
        .table-of-contents h2 {
            font-size: 18px;
            margin-top: 0;
        }
        .table-of-contents ul {
            list-style: none;
            padding-left: 0;
        }
        .table-of-contents li {
            padding: 4px 0;
        }
        .table-of-contents a {
            color: #4A90D9;
            text-decoration: none;
        }
        h1 { font-size: 24px; color: #1a1a1a; margin-top: 32px; border-bottom: 1px solid #eee; padding-bottom: 8px; }
        h2 { font-size: 20px; color: #2a2a2a; margin-top: 24px; }
        h3 { font-size: 17px; color: #333; margin-top: 20px; }
        p { margin: 12px 0; }   /* 对齐由段落自身 align 属性决定，不全局覆盖 */
        ul, ol { margin: 12px 0; padding-left: 28px; }
        li { margin: 6px 0; }
        blockquote {
            border-left: 4px solid #4A90D9;
            background: #f0f7ff;
            margin: 16px 0;
            padding: 12px 20px;
            color: #555;
            font-style: italic;
        }
        pre {
            background: #1e1e1e;
            color: #d4d4d4;
            padding: 16px;
            border-radius: 6px;
            overflow-x: auto;
            font-family: Consolas, Monaco, monospace;
            font-size: 13px;
            line-height: 1.5;
        }
        code {
            background: #f0f0f0;
            padding: 2px 6px;
            border-radius: 3px;
            font-family: Consolas, Monaco, monospace;
            font-size: 0.9em;
        }
        pre code {
            background: none;
            padding: 0;
        }
        hr {
            border: none;
            border-top: 1px solid #ddd;
            margin: 32px 0;
        }
        .image-block {
            text-align: center;
            margin: 20px 0;
        }
        .image-block img {
            max-width: 100%;
            border-radius: 4px;
            box-shadow: 0 2px 8px rgba(0,0,0,0.1);
        }
        .image-caption {
            font-size: 13px;
            color: #888;
            margin-top: 8px;
        }
        .table-block {
            margin: 16px 0;
        }
        .table-block table {
            border-collapse: collapse;
            width: 100%;
            font-size: 14px;
        }
        .table-block th {
            border: 1px solid #333;
            padding: 8px 12px;
            background-color: #f0f0f0;
            font-weight: bold;
            text-align: left;
        }
        .table-block td {
            border: 1px solid #333;
            padding: 8px 12px;
        }
        .chart-block {
            text-align: center;
            margin: 16px 0;
        }
        .chart-block img {
            max-width: 100%;
        }
        .chart-caption, .image-caption {
            color: #666;
            font-size: 12px;
            margin-top: 8px;
        }
        .report-footer {
            margin-top: 48px;
            padding-top: 16px;
            border-top: 1px solid #eee;
            text-align: center;
            font-size: 12px;
            color: #aaa;
        }
        @media print {
            body { max-width: none; padding: 0; }
            .report-footer { display: none; }
        }
    )").arg(config.fontFamily).arg(config.fontSize);
}
