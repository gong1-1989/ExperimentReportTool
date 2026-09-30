/**
 * @file SearchResultDialog.cpp
 * @brief 搜索结果对话框实现文件
 */

#include "SearchResultDialog.h"
#include "ui_SearchResultDialog.h"  // 由 uic 工具从 .ui 文件自动生成
#include "service/ProjectService.h"
#include "service/TagService.h"
#include "core/models/Tag.h"
#include "core/utils/Logger.h"
#include "core/utils/AppDimensions.h"
#include "core/utils/AppTheme.h"

#include <QMessageBox>
#include "ui/UiHelper.h"
#include "data/repositories/ProjectRepository.h"
#include "data/repositories/TagRepository.h"
#include <QDateTime>
#include <QLabel>

// ===========================================================================
// 构造与析构
// ===========================================================================

SearchResultDialog::SearchResultDialog(QWidget* parent, const QString& initialKeyword)
    : BaseDialog(parent)
    , ui(new Ui::SearchResultDialog)
    , m_searchService(nullptr)
{
    m_searchService = new SearchService(this);
    ui->setupUi(this);

    // 初始化项目过滤下拉
    ui->m_projectFilter->addItem(tr("全部项目"), -1);
    {
        const Project::List projects = ProjectService::listAll();
        for (const Project::Ptr& project : projects) {
            ui->m_projectFilter->addItem(project->name(), project->id());
        }
    }

    // 初始化搜索历史
    updateHistory();

    // 搜索防抖：输入停顿 300ms 后自动搜索，连续输入不触发
    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(300);
    connect(m_searchTimer, &QTimer::timeout, this, &SearchResultDialog::debouncedSearch);
    connect(ui->m_searchEdit, &QLineEdit::textChanged,
            this, &SearchResultDialog::on_m_searchEdit_textChanged);

    if (!initialKeyword.isEmpty()) {
        ui->m_searchEdit->setText(initialKeyword);
        performSearch();
    }

    setWindowTitle(tr("搜索报告"));
    resize(AppDimensions::Window::DialogMediumWidth, AppDimensions::Window::DialogMediumHeight);

    // 设置分割器初始比例（左侧 40%，右侧 60%）
    ui->m_splitter->setSizes({320, 500});
}

SearchResultDialog::~SearchResultDialog()
{
    delete ui;
}

// ===========================================================================
// 搜索
// ===========================================================================

void SearchResultDialog::on_m_searchEdit_textChanged(const QString& text)
{
    // 空关键词不自动搜索（避免频繁弹提示/刷结果）
    if (text.trimmed().isEmpty()) {
        m_searchTimer->stop();
        return;
    }
    m_searchTimer->start();   // 重新计时，实现防抖
}

void SearchResultDialog::debouncedSearch()
{
    if (ui->m_searchEdit->text().trimmed().isEmpty()) return;
    performSearch();
}

void SearchResultDialog::on_m_searchBtn_clicked()
{
    performSearch();
}

void SearchResultDialog::performSearch()
{
    const QString keyword = ui->m_searchEdit->text().trimmed();
    if (keyword.isEmpty()) {
        UiHelper::info(this, tr("提示"), tr("请输入搜索关键词"));
        return;
    }

    ui->m_statusLabel->setText(tr("正在搜索..."));
    QApplication::setOverrideCursor(Qt::WaitCursor);

    SearchQuery query;
    query.keyword = keyword;
    query.projectId = ui->m_projectFilter->currentData().toLongLong();
    query.maxResults = 50;

    m_results = m_searchService->search(query);

    // 保存搜索历史
    m_searchService->addToHistory(keyword);
    updateHistory();

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
            QString("<div style='color: %1; text-align: center; margin-top: 50px;'>"
                    "<p style='font-size: %2px;'>&#128269;</p>"
                    "<p>未找到匹配的报告</p>"
                    "<p style='font-size: %3px;'>尝试使用其他关键词</p>"
                    "</div>")
                .arg(AppTheme::Color::TextSecondary)
                .arg(AppTheme::FontSize::Massive)
                .arg(AppTheme::FontSize::Small));
        return;
    }

    ui->m_statusLabel->setText(tr("找到 %1 个结果").arg(results.size()));

    // 批量加载本次结果全部标签（一次 SQL），避免逐结果查询
    QList<qint64> reportIds;
    for (const SearchResultItem& item : results) reportIds.append(item.report->id());
    const QHash<qint64, Tag::List> tagsMap = TagService::findReportTagsBatch(reportIds);

    for (const SearchResultItem& item : results) {
        QListWidgetItem* listItem = new QListWidgetItem(ui->m_resultList);

        // 状态文字和颜色（使用 AppTheme 统一管理）
        const QString statusText = AppTheme::statusName(item.report->status());
        const QString statusColor = AppTheme::statusColor(item.report->status()).name();

        // 标签（取前3个）
        const Tag::List tags = tagsMap.value(item.report->id());
        QString tagsHtml;
        for (int i = 0; i < qMin(3, tags.size()); ++i) {
            const QColor color = tags[i]->effectiveColor();
            tagsHtml += QString(
                "<span style='display: inline-block; background: %1; color: white; "
                "padding: %2px %3px; border-radius: %4px; font-size: %5px; margin-right: %6px;'>"
                "%7</span>"
            ).arg(color.name())
             .arg(AppTheme::Spacing::Tiny)
             .arg(AppTheme::Spacing::Normal)
             .arg(AppTheme::Radius::Pill)
             .arg(AppTheme::FontSize::ExtraSmall)
             .arg(AppTheme::Spacing::Small)
             .arg(tags[i]->name().toHtmlEscaped());
        }

        // 匹配字段（用标签样式）
        const QString highlightText = item.highlight.isEmpty()
            ? tr("匹配元数据") : item.highlight;

        // 构建显示文本
        const QString displayText = QString(
            "<div style='padding: %1px %2px;'>"
            "<div style='font-size: %3px; font-weight: bold; color: %4; margin-bottom: %5px;'>%6</div>"
            "<div style='font-size: %7px; color: %8; margin-bottom: %5px;'>"
            "<span style='color: %9; font-weight: bold;'>[%10]</span>"
            "&nbsp;&nbsp;%11&nbsp;&nbsp;|&nbsp;&nbsp;%12"
            "</div>"
            "<div style='margin-bottom: %5px;'>%13</div>"
            "<div style='font-size: %14px; color: %15; background: %16; "
            "padding: %17px %18px; border-radius: %19px; display: inline-block;'>%20</div>"
            "</div>"
        ).arg(AppTheme::Spacing::Small).arg(AppTheme::Spacing::Tiny)
         .arg(AppTheme::FontSize::Large).arg(AppTheme::Color::TextPrimary)
         .arg(AppTheme::Spacing::Small)
         .arg(item.report->title().toHtmlEscaped())
         .arg(AppTheme::FontSize::Small).arg(AppTheme::Color::TextRegular)
         .arg(statusColor).arg(statusText)
         .arg(item.projectName.isEmpty() ? tr("未分类") : item.projectName.toHtmlEscaped())
         .arg(item.report->experimentDate().isValid()
              ? item.report->experimentDate().toString("yyyy-MM-dd")
              : tr("未设置日期"))
         .arg(tagsHtml.isEmpty()
              ? QString("<span style='color: %1; font-size: %2px;'>无标签</span>")
                    .arg(AppTheme::Color::TextPlaceholder).arg(AppTheme::FontSize::ExtraSmall)
              : tagsHtml)
         .arg(AppTheme::FontSize::ExtraSmall)
         .arg(AppTheme::Color::TextSecondary)
         .arg(AppTheme::Color::BorderExtraLight)
         .arg(AppTheme::Spacing::Tiny).arg(AppTheme::Spacing::Normal)
         .arg(AppTheme::Radius::Small)
         .arg(highlightText);

        // 使用 QLabel 作为 item widget 以支持 HTML 富文本渲染
        QLabel* label = new QLabel(displayText);
        label->setTextFormat(Qt::RichText);
        label->setWordWrap(true);
        label->setStyleSheet(
            QString("padding: %1px %2px; background: transparent;")
                .arg(AppTheme::Spacing::Normal).arg(AppTheme::Spacing::Large));
        label->setAttribute(Qt::WA_TransparentForMouseEvents);
        ui->m_resultList->setItemWidget(listItem, label);

        listItem->setData(Qt::UserRole, item.report->id());
        listItem->setSizeHint(QSize(0, 100));
    }

    // 选中第一个结果
    if (ui->m_resultList->count() > 0) {
        ui->m_resultList->setCurrentRow(0);
        on_m_resultList_itemClicked(ui->m_resultList->currentItem());
    }
}

// ===========================================================================
// 结果点击
// ===========================================================================

void SearchResultDialog::on_m_resultList_itemClicked(QListWidgetItem* item)
{
    if (!item) {
        ui->m_openBtn->setEnabled(false);
        return;
    }

    const qint64 reportId = item->data(Qt::UserRole).toLongLong();

    // 查找对应的搜索结果
    for (const SearchResultItem& result : m_results) {
        if (result.report->id() == reportId) {
            // 状态文字和颜色（使用 AppTheme 统一管理）
            const QString statusText = AppTheme::statusName(result.report->status());
            const QString statusColor = AppTheme::statusColor(result.report->status()).name();
            const QString statusBg = AppTheme::statusColor(result.report->status()).lighter(180).name();

            // 标签
            const Tag::List tags = TagService::findByReport(result.report->id());
            QString tagsHtml;
            for (const Tag::Ptr& tag : tags) {
                const QColor color = tag->effectiveColor();
                tagsHtml += QString(
                    "<span style='display: inline-block; background: %1; color: white; "
                    "padding: %2px %3px; border-radius: %4px; font-size: %5px; margin-right: %6px;'>"
                    "%7</span>"
                ).arg(color.name())
                 .arg(AppTheme::Spacing::Tiny).arg(AppTheme::Spacing::Large)
                 .arg(AppTheme::Radius::Pill)
                 .arg(AppTheme::FontSize::Small)
                 .arg(AppTheme::Spacing::Medium)
                 .arg(tag->name().toHtmlEscaped());
            }
            if (tagsHtml.isEmpty()) {
                tagsHtml = QString("<span style='color: %1; font-size: %2px;'>无标签</span>")
                               .arg(AppTheme::Color::TextPlaceholder).arg(AppTheme::FontSize::Small);
            }

            // 匹配字段描述
            const QString highlightText = result.highlight.isEmpty()
                ? tr("匹配元数据") : result.highlight;

            // 显示详情（卡片式布局）
            const QString html = QString(
                "<div style='padding: %1px;'>"
                // 标题行
                "<div style='margin-bottom: %1px;'>"
                "<div style='color: %2; margin: 0 0 %3px 0; font-size: %4px; font-weight: bold;'>%5</div>"
                "<span style='display: inline-block; background: %6; color: %7; "
                "padding: %3px %8px; border-radius: %9px; font-size: %10px; font-weight: bold;'>%11</span>"
                "</div>"
                // 基本信息卡片
                "<div style='background: %12; border-radius: %13px; padding: %1px; margin-bottom: %1px;'>"
                "<table style='width: 100%; font-size: %14px; border-collapse: collapse;'>"
                "<tr><td style='color: %15; padding: %3px %8px; width: 80px;'>项目</td>"
                "<td style='color: %2; padding: %3px %8px;'>%16</td></tr>"
                "<tr><td style='color: %15; padding: %3px %8px;'>作者</td>"
                "<td style='color: %2; padding: %3px %8px;'>%17</td></tr>"
                "<tr><td style='color: %15; padding: %3px %8px;'>实验日期</td>"
                "<td style='color: %2; padding: %3px %8px;'>%18</td></tr>"
                "<tr><td style='color: %15; padding: %3px %8px;'>更新时间</td>"
                "<td style='color: %2; padding: %3px %8px;'>%19</td></tr>"
                "<tr><td style='color: %15; padding: %3px %8px;'>标签</td>"
                "<td style='color: %2; padding: %3px %8px;'>%20</td></tr>"
                "</table>"
                "</div>"
                // 匹配信息
                "<div style='margin-bottom: %1px;'>"
                "<div style='font-size: %21px; font-weight: bold; color: %22; margin-bottom: %3px;'>"
                "&#128269; 匹配信息</div>"
                "<div style='background: %23; border-left: 4px solid %22; "
                "padding: %8px %1px; border-radius: %13px; font-size: %21px; color: %2;'>%24</div>"
                "</div>"
                // 提示
                "<div style='color: %15; font-size: %10px; text-align: center; margin-top: %25px;'>"
                "双击结果或按 Enter 键打开报告</div>"
                "</div>"
            ).arg(AppTheme::Spacing::Large)
             .arg(AppTheme::Color::TextPrimary)
             .arg(AppTheme::Spacing::Small)
             .arg(AppTheme::FontSize::ExtraLarge)
             .arg(result.report->title().isEmpty() ? tr("未命名报告") : result.report->title().toHtmlEscaped())
             .arg(statusBg).arg(statusColor)
             .arg(AppTheme::Spacing::Normal)
             .arg(AppTheme::Radius::Pill)
             .arg(AppTheme::FontSize::Small)
             .arg(statusText)
             .arg(AppTheme::Color::BorderExtraLight)
             .arg(AppTheme::Radius::Medium)
             .arg(AppTheme::FontSize::Medium)
             .arg(AppTheme::Color::TextSecondary)
             .arg(result.projectName.isEmpty() ? tr("未分类") : result.projectName.toHtmlEscaped())
             .arg(result.report->author().isEmpty() ? tr("未设置") : result.report->author().toHtmlEscaped())
             .arg(result.report->experimentDate().isValid()
                  ? result.report->experimentDate().toString("yyyy-MM-dd") : tr("未设置"))
             .arg(result.report->updatedAt().isValid()
                  ? result.report->updatedAt().toString("yyyy-MM-dd HH:mm") : tr("未知"))
             .arg(tagsHtml)
             .arg(AppTheme::FontSize::Normal)
             .arg(AppTheme::Color::Primary)
             .arg(AppTheme::Color::PrimaryLight)
             .arg(highlightText)
             .arg(AppTheme::Spacing::Huge);

            ui->m_detailBrowser->setHtml(html);
            ui->m_openBtn->setEnabled(true);
            break;
        }
    }
}

void SearchResultDialog::on_m_resultList_itemDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    const qint64 reportId = item->data(Qt::UserRole).toLongLong();
    emit reportOpenRequested(reportId);
    accept();
}

// ===========================================================================
// 搜索历史
// ===========================================================================

void SearchResultDialog::on_m_historyCombo_currentIndexChanged(const QString& text)
{
    if (text.isEmpty()) return;
    ui->m_searchEdit->setText(text);
    performSearch();
}

void SearchResultDialog::on_m_clearHistoryBtn_clicked()
{
    m_searchService->clearHistory();
    updateHistory();
}

void SearchResultDialog::updateHistory()
{
    ui->m_historyCombo->blockSignals(true);
    ui->m_historyCombo->clear();
    const QStringList history = m_searchService->searchHistory();
    for (const QString& keyword : history) {
        ui->m_historyCombo->addItem(keyword);
    }
    ui->m_historyCombo->setCurrentIndex(-1);
    ui->m_historyCombo->blockSignals(false);
}

// ===========================================================================
// 筛选变化
// ===========================================================================

void SearchResultDialog::on_m_projectFilter_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    // 如果有搜索关键词，重新搜索
    if (!ui->m_searchEdit->text().trimmed().isEmpty()) {
        performSearch();
    }
}

// ===========================================================================
// 打开/关闭按钮
// ===========================================================================

void SearchResultDialog::on_m_openBtn_clicked()
{
    QListWidgetItem* item = ui->m_resultList->currentItem();
    if (!item) return;
    const qint64 reportId = item->data(Qt::UserRole).toLongLong();
    if (reportId > 0) {
        emit reportOpenRequested(reportId);
        accept();
    }
}

void SearchResultDialog::on_m_closeBtn_clicked()
{
    reject();
}
