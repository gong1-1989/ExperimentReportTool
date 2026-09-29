/**
 * @file VersionDiffer.cpp
 * @brief 报告版本对比纯逻辑实现
 */
#include "core/utils/VersionDiffer.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextFragment>
#include <QTextCharFormat>
#include <QVector>
#include <QCoreApplication>
#include <QFileInfo>

#include "core/models/Report.h"

// ===========================================================================
// 文本提取
// ===========================================================================

QStringList VersionDiffer::extractBlocks(const QString& contentJson)
{
    QStringList blocks;
    if (contentJson.isEmpty()) return blocks;

    Report::Ptr report = Report::create();
    report->contentFromJson(contentJson);

    // 连续文档：按段落提取纯文本，锚点位置输出对象描述
    QTextDocument htmlParser;
    htmlParser.setHtml(report->document());

    const QList<ContentBlock>& objects = report->objects();

    for (QTextBlock block = htmlParser.begin();
         block != htmlParser.end(); block = block.next()) {
        // 检查块内是否有对象锚点
        QString anchorId;
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            const QTextCharFormat fmt = fragment.charFormat();
            if (fmt.isImageFormat()) {
                const QString imageName = fmt.toImageFormat().name();
                if (imageName.startsWith("object://")) {
                    const int slash = imageName.lastIndexOf('/');
                    if (slash > 0) {
                        anchorId = imageName.mid(slash + 1);
                        break;
                    }
                }
            }
        }

        if (!anchorId.isEmpty()) {
            // 查找对象描述
            for (const ContentBlock& object : objects) {
                if (object.id != anchorId) continue;
                QString line;
                switch (object.type) {
                case BlockType::Image: {
                    const QString caption = object.data.value("caption").toString();
                    const QString path = object.data.value("path").toString();
                    line = QCoreApplication::translate("VersionDiffer", "[图片]");
                    if (!caption.isEmpty()) line += QString(" %1").arg(caption);
                    if (!path.isEmpty())
                        line += QString(" (%1)").arg(QFileInfo(path).fileName());
                    break;
                }
                case BlockType::Table:
                case BlockType::DataReference: {
                    line = QCoreApplication::translate("VersionDiffer", "[表格]");
                    const QString caption = object.data.value("caption").toString();
                    if (!caption.isEmpty()) line += QString(" %1").arg(caption);
                    else line += QString(" id=%1").arg(static_cast<qint64>(object.data.value("tableId").toDouble()));
                    break;
                }
                case BlockType::Chart: {
                    line = QCoreApplication::translate("VersionDiffer", "[图表]");
                    const QJsonObject d = object.data.value("config").toObject();
                    const QString title = d.value("title").toString();
                    if (!title.isEmpty()) line += QString(" %1").arg(title);
                    break;
                }
                case BlockType::Formula: {
                    line = QCoreApplication::translate("VersionDiffer", "[公式] %1")
                               .arg(object.data.value("latex").toString());
                    break;
                }
                default:
                    break;
                }
                if (!line.isEmpty()) blocks.append(line);
                break;
            }
            continue;
        }

        // 纯文本段落
        const QString text = block.text().trimmed();
        if (!text.isEmpty()) {
            blocks.append(text);
        }
    }

    return blocks;
}

// ===========================================================================
// 块级相似度（bigram Jaccard）
// ===========================================================================

double VersionDiffer::blockSimilarity(const QString& a, const QString& b)
{
    const QString sa = a.simplified();
    const QString sb = b.simplified();
    if (sa.isEmpty() || sb.isEmpty()) return 0.0;
    if (sa == sb) return 1.0;

    QSet<QString> ga, gb;
    for (int k = 0; k + 1 < sa.size(); ++k) ga.insert(sa.mid(k, 2));
    for (int k = 0; k + 1 < sb.size(); ++k) gb.insert(sb.mid(k, 2));
    if (ga.isEmpty() || gb.isEmpty()) return 0.0;

    const int inter = ga.intersect(gb).size();
    const int uni = ga.size() + gb.size() - inter;
    return uni > 0 ? static_cast<double>(inter) / uni : 0.0;
}

// ===========================================================================
// 块级差异对比（LCS 对齐 + 相似度配对）
// ===========================================================================

VersionDiffer::DiffResult VersionDiffer::buildBlockDiff(
    const QStringList& blocksA, const QStringList& blocksB)
{
    const int n = blocksA.size();
    const int m = blocksB.size();

    QVector<QVector<int>> dp(n + 1, QVector<int>(m + 1, 0));
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            if (blocksA.at(i - 1) == blocksB.at(j - 1))
                dp[i][j] = dp[i - 1][j - 1] + 1;
            else
                dp[i][j] = qMax(dp[i - 1][j], dp[i][j - 1]);
        }
    }

    QVector<QPair<int, int>> matched;
    int i = n, j = m;
    while (i > 0 && j > 0) {
        if (blocksA.at(i - 1) == blocksB.at(j - 1)) {
            matched.prepend(qMakePair(i - 1, j - 1));
            --i; --j;
        } else if (dp[i - 1][j] >= dp[i][j - 1]) {
            --i;
        } else {
            --j;
        }
    }

    const QString style =
        "<html><head><meta charset='utf-8'></head><body>"
        "<style>"
        "body{font-family:'Microsoft YaHei',sans-serif;font-size:10.5pt;"
        "line-height:1.6;color:#303133;margin:0;padding:12px;}"
        "pre{font-family:'Microsoft YaHei',sans-serif;margin:0;"
        "padding:2px 8px;white-space:pre-wrap;word-break:break-all;}"
        ".same{background:transparent;}"
        ".del{background:#FDE2E2;color:#C45656;}"
        ".add{background:#E1F3D8;color:#529B2E;}"
        ".mod{background:#FDF6EC;color:#B88230;}"
        ".block-num{color:#C0C4CC;font-size:9pt;margin-right:8px;}"
        "</style>";

    QStringList htmlA, htmlB;
    htmlA << style;
    htmlB << style;

    int sameCount = 0;
    int diffCount = 0;

    auto renderBlock = [](const QString& text, const QString& cls, int num) {
        return QString("<pre class='%1'><span class='block-num'>%2</span>%3</pre>")
            .arg(cls).arg(num).arg(text.toHtmlEscaped());
    };

    int pa = 0, pb = 0;
    int numA = 0, numB = 0;

    auto handleSegment = [&](int aStart, int aEnd, int bStart, int bEnd) {
        const int delCount = aEnd - aStart + 1;
        const int addCount = bEnd - bStart + 1;
        const int pairs = qMin(delCount, addCount);

        for (int k = 0; k < pairs; ++k) {
            const QString& ta = blocksA.at(aStart + k);
            const QString& tb = blocksB.at(bStart + k);
            const double sim = blockSimilarity(ta, tb);
            if (sim >= 0.55) {
                htmlA << renderBlock(ta, "mod", ++numA);
                htmlB << renderBlock(tb, "mod", ++numB);
                ++diffCount;
            } else {
                htmlA << renderBlock(ta, "del", ++numA);
                htmlB << renderBlock(tb, "add", ++numB);
                diffCount += 2;
            }
        }
        for (int k = pairs; k < delCount; ++k) {
            htmlA << renderBlock(blocksA.at(aStart + k), "del", ++numA);
            htmlB << renderBlock(QString(), "add", ++numB);
            ++diffCount;
        }
        for (int k = pairs; k < addCount; ++k) {
            htmlA << renderBlock(QString(), "del", ++numA);
            htmlB << renderBlock(blocksB.at(bStart + k), "add", ++numB);
            ++diffCount;
        }
    };

    for (const auto& pair : matched) {
        const int ai = pair.first;
        const int bj = pair.second;
        handleSegment(pa, ai - 1, pb, bj - 1);
        htmlA << renderBlock(blocksA.at(ai), "same", ++numA);
        htmlB << renderBlock(blocksB.at(bj), "same", ++numB);
        ++sameCount;
        pa = ai + 1;
        pb = bj + 1;
    }
    handleSegment(pa, n - 1, pb, m - 1);

    htmlA << "</body></html>";
    htmlB << "</body></html>";

    DiffResult result;
    result.leftHtml = htmlA.join(QString());
    result.rightHtml = htmlB.join(QString());
    result.sameCount = sameCount;
    result.diffCount = diffCount;
    return result;
}
