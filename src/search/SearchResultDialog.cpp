/**
 * @file SearchResultDialog.cpp
 * @brief 搜索结果对话框实现文件
 */

#include "SearchResultDialog.h"
#include "ui_SearchResultDialog.h"  // 由 uic 工具从 .ui 文件自动生成
#include "data/repositories/ProjectRepository.h"
#include "core/utils/Logger.h"

#include <QMessageBox>
#include <QDateTime>

// ===========================================================================
// 构造与析构
// ===========================================================================

SearchResultDialog::SearchResultDialog(QWidget* parent, const QString& initialKeyword)
    : ui(new Ui::SearchResultDialog),  QDialog(parent)
    
    
    
    
    
    
    
    
    
    
{
    m_searchService = new SearchService(this);
    ui->setupUi(this);

    if (!initialKeyword.isEmpty()) {
        ui->m_searchEdit->setText(initialKeyword);
        performSearch();
    }

    setWindowTitle(tr("全文搜索"));
    resize(900, 600);
}

SearchResultDialog::~SearchResultDialog()
{
    delete ui;
}

// ===========================================================================
// 搜索
// ===========================================================================

void SearchResultDialog::onSearch()
{
    performSearch();
}

void SearchResultDialog::performSearch()
{
    const QString keyword = ui->m_searchEdit->text().trimmed();
    if (keyword.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请输入搜索关键词"));
        return;
    }

    ui->m_statusLabel->setText(tr("正在搜索..."));
    QApplication::setOverrideCursor(Qt::WaitCursor);

    SearchQuery query;
    query.keyword = keyword;
    query.projectId = ui->m_projectFilter->currentData().toLongLong();
    query.maxResults = 50;

    m_results = m_searchService->search(query);

    QApplication::restoreOverrideCursor();
    displayResults(m_results);
}

// ===========================================================================
// 结果展示
// ===========================================================================

void SearchResultDialog::displayResults(const QList<SearchResultItem>& results)
{
    ui->m_resultList->clear();

    if (results.isEmpty()) {
        ui->m_statusLabel->setText(tr("未找到匹配的报告"));
        ui->m_detailBrowser->clear();
        ui->m_detailBrowser->setHtml(
            "<div style='color: #999; text-align: center; margin-top: 50px;'>"
            "<p style='font-size: 48px;'>🔍</p>"
            "<p>未找到匹配的报告</p>"
            "<p style='font-size: 12px;'>尝试使用其他关键词</p>"
            "</div>");
        return;
    }

    ui->m_statusLabel->setText(tr("找到 %1 个结果").arg(results.size()));

    for (const SearchResultItem& item : results) {
        QListWidgetItem* listItem = new QListWidgetItem(ui->m_resultList);

        // 构建显示文本
        QString displayText = QString(
            "<div style='padding: 4px 0;'>"
            "<div style='font-weight: bold; font-size: 14px; color: #1a1a1a;'>%1</div>"
            "<div style='font-size: 12px; color: #666; margin-top: 4px;'>"
            "📁 %2 &nbsp;|&nbsp; 👤 %3 &nbsp;|&nbsp; 📅 %4"
            "</div>"
            "<div style='font-size: 12px; color: #888; margin-top: 4px;'>%5</div>"
            "</div>"
        ).arg(item.report->title().toHtmlEscaped())
         .arg(item.projectName.isEmpty() ? tr("未知项目") : item.projectName.toHtmlEscaped())
         .arg(item.report->author().isEmpty() ? tr("未知") : item.report->author().toHtmlEscaped())
         .arg(item.report->experimentDate().isValid()
              ? item.report->experimentDate().toString("yyyy-MM-dd")
              : tr("未设置"))
         .arg(item.highlight.isEmpty() ? tr("点击查看详情") : item.highlight);

        listItem->setText(displayText);
        listItem->setData(Qt::UserRole, item.report->id());
        listItem->setSizeHint(QSize(0, 80));
    }

    // 选中第一个结果
    if (ui->m_resultList->count() > 0) {
        ui->m_resultList->setCurrentRow(0);
        onResultClicked(ui->m_resultList->currentItem());
    }
}

// ===========================================================================
// 结果点击
// ===========================================================================

void SearchResultDialog::onResultClicked(QListWidgetItem* item)
{
    if (!item) return;

    const qint64 reportId = item->data(Qt::UserRole).toLongLong();

    // 查找对应的搜索结果
    for (const SearchResultItem& result : m_results) {
        if (result.report->id() == reportId) {
            // 显示详情
            QString html = QString(
                "<div style='padding: 8px;'>"
                "<h2 style='color: #1a1a1a; border-bottom: 2px solid #4A90D9; padding-bottom: 8px;'>%1</h2>"
                "<p style='color: #666; font-size: 13px;'>"
                "<strong>项目:</strong> %2<br>"
                "<strong>作者:</strong> %3<br>"
                "<strong>实验日期:</strong> %4<br>"
                "<strong>更新时间:</strong> %5"
                "</p>"
                "<hr style='border: none; border-top: 1px solid #eee; margin: 16px 0;'>"
                "<h3 style='color: #333;'>匹配内容</h3>"
                "<div style='background: #f8f9fa; padding: 12px; border-radius: 6px; "
                "font-size: 14px; line-height: 1.8; color: #333;'>%6</div>"
                "<p style='color: #999; font-size: 12px; margin-top: 20px;'>"
                "双击结果或点击下方按钮打开报告</p>"
                "</div>"
            ).arg(result.report->title().toHtmlEscaped())
             .arg(result.projectName.isEmpty() ? tr("未知") : result.projectName.toHtmlEscaped())
             .arg(result.report->author().isEmpty() ? tr("未知") : result.report->author().toHtmlEscaped())
             .arg(result.report->experimentDate().isValid()
                  ? result.report->experimentDate().toString("yyyy-MM-dd")
                  : tr("未设置"))
             .arg(result.report->updatedAt().toString("yyyy-MM-dd hh:mm"))
             .arg(result.highlight.isEmpty() ? tr("无匹配摘要") : result.highlight);

            ui->m_detailBrowser->setHtml(html);
            break;
        }
    }
}

void SearchResultDialog::onResultDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    const qint64 reportId = item->data(Qt::UserRole).toLongLong();
    emit reportOpenRequested(reportId);
    accept();
}

// ===========================================================================
// 搜索历史
// ===========================================================================

void SearchResultDialog::onHistorySelected(const QString& text)
{
    ui->m_searchEdit->setText(text);
    performSearch();
}

void SearchResultDialog::onClearHistory()
{
    m_searchService->clearHistory();
    updateHistory();
}

void SearchResultDialog::updateHistory()
{
    ui->m_historyCombo->clear();
    const QStringList history = m_searchService->searchHistory();
    for (const QString& keyword : history) {
        ui->m_historyCombo->addItem(keyword);
    }
    ui->m_historyCombo->setCurrentIndex(-1);
}

// ===========================================================================
// 筛选变化
// ===========================================================================

void SearchResultDialog::onFilterChanged(int index)
{
    Q_UNUSED(index);
    // 如果有搜索结果，重新搜索
    if (!ui->m_searchEdit->text().trimmed().isEmpty()) {
        performSearch();
    }
}
