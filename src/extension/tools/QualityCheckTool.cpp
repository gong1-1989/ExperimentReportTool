#include "QualityCheckTool.h"

#include <QRegularExpression>

namespace {

/// 由全文位置计算 1-based 行号
int lineOf(const QString& text, int pos)
{
    int line = 1;
    for (int i = 0; i < pos && i < text.length(); ++i) {
        if (text.at(i) == QLatin1Char('\n')) ++line;
    }
    return line;
}

}  // namespace

DocumentToolResult QualityCheckTool::execute(const DocumentContext& ctx, QString* error) const
{
    Q_UNUSED(error);
    DocumentToolResult r;
    const QString text = ctx.text;

    // 1) 重复 2 字词（如“我们我们”“因为因为”）
    static const QRegularExpression reWord(
        QStringLiteral("([\u4e00-\u9fa5]{2})\\1"));
    {
        QRegularExpressionMatchIterator it = reWord.globalMatch(text);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            r.messages << QStringLiteral("第 %1 行附近：疑似词语重复“%2%3”")
                              .arg(lineOf(text, m.capturedStart()))
                              .arg(m.captured(1), m.captured(1));
        }
    }

    // 2) 超长句（按 。！？； 与换行分句，超过 80 字提示）
    {
        int sentenceStart = 0;
        for (int i = 0; i <= text.length(); ++i) {
            const quint16 uc = (i < text.length()) ? text.at(i).unicode() : 0;
            const bool end = (i == text.length())
                             || uc == 0x0A        // \n
                             || uc == 0x3002      // 。
                             || uc == 0xFF01      // ！
                             || uc == 0xFF1F      // ？
                             || uc == 0xFF1B;     // ；
            if (end) {
                const int len = i - sentenceStart;
                if (len > 80) {
                    r.messages << QStringLiteral("第 %1 行附近：句子过长（%2 字，建议不超过 80 字）")
                                      .arg(lineOf(text, sentenceStart)).arg(len);
                }
                sentenceStart = i + 1;
            }
        }
    }

    // 3) 中英混排缺空格（中文与 ASCII 字母/数字直接相邻）
    static const QRegularExpression reMix(
        QStringLiteral("([\u4e00-\u9fa5])([A-Za-z0-9])|([A-Za-z0-9])([\u4e00-\u9fa5])"));
    {
        QRegularExpressionMatchIterator it = reMix.globalMatch(text);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            r.messages << QStringLiteral("第 %1 行附近：中英文之间建议加空格（如“中文 A 组”）")
                              .arg(lineOf(text, m.capturedStart()));
        }
    }

    // 4) 连续空行（>=2 个空行，即 3 个及以上连续换行）
    static const QRegularExpression reBlank(QStringLiteral("\\n\\n\\n+"));
    {
        QRegularExpressionMatchIterator it = reBlank.globalMatch(text);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            r.messages << QStringLiteral("第 %1 行附近：存在连续空行，建议删除多余空行")
                              .arg(lineOf(text, m.capturedStart()));
        }
    }

    if (r.messages.isEmpty()) {
        r.messages << QStringLiteral("未发现明显问题（重复词 / 超长句 / 中英混排 / 连续空行）");
        r.stats.insert(QStringLiteral("summary"),
                       QStringLiteral("检查 4 项：未发现问题"));
    } else {
        r.stats.insert(QStringLiteral("summary"),
                       QStringLiteral("检查 4 项：发现 %1 处问题").arg(r.messages.size()));
    }
    return r;
}
