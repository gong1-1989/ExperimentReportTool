/**
 * @file VersionDiffer.h
 * @brief 报告版本对比纯逻辑（块提取 / 相似度 / LCS 差异对齐）
 *
 * 从 VersionCompareDialog 提取，便于单元测试与复用，不依赖 UI。
 */
#ifndef VERSION_DIFFER_H
#define VERSION_DIFFER_H

#include <QString>
#include <QStringList>

/**
 * @brief 版本对比纯逻辑工具
 */
class VersionDiffer
{
public:
    /// 块级差异对比结果
    struct DiffResult
    {
        QString leftHtml;    ///< 左侧（版本 A）高亮 HTML
        QString rightHtml;   ///< 右侧（版本 B）高亮 HTML
        int sameCount = 0;   ///< 完全相同的块数
        int diffCount = 0;   ///< 差异块数（删除/新增/修改）
    };

    /// 从版本 JSON 内容提取块序列（每个块一个完整文本，表格/图表整块展示）
    static QStringList extractBlocks(const QString& contentJson);

    /// 块级相似度（bigram Jaccard），用于判断"删除+新增"相邻块是否为修改
    static double blockSimilarity(const QString& a, const QString& b);

    /// 构建块级差异对比 HTML（LCS 对齐，返回左右两侧）
    static DiffResult buildBlockDiff(const QStringList& blocksA,
                                     const QStringList& blocksB);
};

#endif // VERSION_DIFFER_H
