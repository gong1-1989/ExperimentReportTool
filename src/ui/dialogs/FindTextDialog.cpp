/**
 * @file FindTextDialog.cpp
 * @brief 报告编辑器内查找对话框实现
 */

#include "FindTextDialog.h"
#include "ui_FindTextDialog.h"
#include "ui/UiHelper.h"

#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QtCore/Qt>

// ===========================================================================
// 构造与析构
// ===========================================================================

FindTextDialog::FindTextDialog(QWidget* parent)
    : BaseDialog(parent)
    , ui(new Ui::FindTextDialog)
{
    ui->setupUi(this);

    // 自动连接槽（on_ 前缀命名规则）
    // on_m_findBtn_clicked / on_m_resultList_itemActivated / on_m_closeBtn_clicked
    ui->m_keywordEdit->setFocus();
}

FindTextDialog::~FindTextDialog()
{
    delete ui;
}

// ===========================================================================
// 数据设置
// ===========================================================================

void FindTextDialog::setBlocks(const QStringList& blocks)
{
    m_blocks = blocks;
    ui->m_resultList->clear();
    m_matchBlockIndexes.clear();
    ui->m_countLabel->clear();
}

// ===========================================================================
// 查找逻辑
// ===========================================================================

void FindTextDialog::onFind()
{
    const QString keyword = ui->m_keywordEdit->text().trimmed();
    if (keyword.isEmpty()) {
        UiHelper::warning(this, tr("提示"), tr("请输入查找内容"));
        return;
    }

    const Qt::CaseSensitivity cs = ui->m_caseSensitiveCheck->isChecked()
        ? Qt::CaseSensitive : Qt::CaseInsensitive;

    ui->m_resultList->clear();
    m_matchBlockIndexes.clear();

    for (int i = 0; i < m_blocks.size(); ++i) {
        const QString& text = m_blocks.at(i);
        if (text.isEmpty()) continue;
        if (!text.contains(keyword, cs)) continue;

        // 提取匹配位置的上下文片段（前后各 20 字符）
        const int pos = text.indexOf(keyword, 0, cs);
        const int ctxStart = qMax(0, pos - 20);
        const int ctxLen = qMin(text.length() - ctxStart, keyword.length() + 40);
        QString snippet = text.mid(ctxStart, ctxLen);
        snippet = snippet.replace('\n', ' ').simplified();
        if (ctxStart > 0) snippet = "…" + snippet;
        if (ctxStart + ctxLen < text.length()) snippet = snippet + "…";

        QListWidgetItem* item = new QListWidgetItem(
            tr("块 %1: %2").arg(i + 1).arg(snippet), ui->m_resultList);
        item->setData(Qt::UserRole, i);
        m_matchBlockIndexes.append(i);
    }

    if (m_matchBlockIndexes.isEmpty()) {
        ui->m_countLabel->setText(tr("未找到匹配内容"));
        return;
    }

    ui->m_countLabel->setText(
        tr("共找到 %1 处匹配（%2 个块）")
            .arg(m_matchBlockIndexes.size()).arg(m_blocks.size()));
    ui->m_resultList->setCurrentRow(0);
}

void FindTextDialog::onResultActivated(QListWidgetItem* item)
{
    if (!item) return;
    const int blockIndex = item->data(Qt::UserRole).toInt();
    emit jumpRequested(blockIndex);
}
