#ifndef FIND_TEXT_DIALOG_H
#define FIND_TEXT_DIALOG_H

#include "ui/dialogs/BaseDialog.h"

class QListWidgetItem;

namespace Ui { class FindTextDialog; }

/**
 * @brief 报告编辑器内查找对话框
 *
 * 输入关键词后遍历报告的所有块，列出匹配结果；
 * 双击结果行可跳转到对应块（通过 jumpRequested 信号）。
 */
class FindTextDialog : public BaseDialog
{
    Q_OBJECT

public:
    explicit FindTextDialog(QWidget* parent = nullptr);
    ~FindTextDialog() override;

    /**
     * @brief 设置待查找的文本块列表
     * @param blocks 块纯文本内容列表（索引即块索引）
     */
    void setBlocks(const QStringList& blocks);

signals:
    /// 请求跳转到指定块（双击结果或点击跳转时发出）
    void jumpRequested(int blockIndex);

private slots:
    void onFind();
    void onResultActivated(QListWidgetItem* item);

private:
    Ui::FindTextDialog* ui;  ///< UI 界面对象（从 .ui 文件自动生成）

    QStringList m_blocks;
    QList<int> m_matchBlockIndexes;  ///< 匹配的块索引（与 m_blocks 对应）
};

#endif // FIND_TEXT_DIALOG_H
