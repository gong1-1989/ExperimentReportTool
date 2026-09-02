/**
 * @file SearchResultDialog.cpp
 * @brief 搜索结果对话框实现文件
 */

#include "SearchResultDialog.h"
#include "data/repositories/ProjectRepository.h"
#include "core/utils/Logger.h"

#include <QMessageBox>
#include <QDateTime>
#include <QApplication>

// ===========================================================================
// 构造与析构
// ===========================================================================

SearchResultDialog::SearchResultDialog(QWidget* parent, const QString& initialKeyword)
    : QDialog(parent)
    , m_searchEdit(nullptr)
    , m_searchBtn(nullptr)
    , m_projectFilter(nullptr)
    , m_historyCombo(nullptr)
    , m_clearHistoryBtn(nullptr)
    , m_splitter(nullptr)
    , m_resultList(nullptr)
    , m_detailBrowser(nullptr)
    , m_statusLabel(nullptr)
    , m_searchService(nullptr)
{
    m_searchService = new SearchService(this);
    setupUi();

    if (!initialKeyword.isEmpty()) {
        m_searchEdit->setText(initialKeyword);
        performSearch();
    }

    setWindowTitle(tr("全文搜索"));
    resize(900, 600);
}

SearchResultDialog::~SearchResultDialog()
{
}

// ===========================================================================
// UI 初始化
// ===========================================================================

void SearchResultDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // -----------------------------------------------------------------------
    // 搜索栏
    // -----------------------------------------------------------------------
    QHBoxLayout* searchBar = new QHBoxLayout();
    searchBar->setSpacing(8);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("输入关键词搜索报告..."));
    m_searchEdit->setClearButtonEnabled(true);
    searchBar->addWidget(m_searchEdit, 1);

    m_searchBtn = new QPushButton(tr("搜索"), this);
    m_searchBtn->setStyleSheet(
        "QPushButton { background: #4A90D9; color: white; padding: 6px 20px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #357ABD; }");
    searchBar->addWidget(m_searchBtn);

    mainLayout->addLayout(searchBar);

    // -----------------------------------------------------------------------
    // 筛选栏
    // -----------------------------------------------------------------------
    QHBoxLayout* filterBar = new QHBoxLayout();
    filterBar->setSpacing(8);

    filterBar->addWidget(new QLabel(tr("项目:"), this));
    m_projectFilter = new QComboBox(this);
    m_projectFilter->addItem(tr("全部项目"), -1);
    const Project::List projects = ProjectRepository::findAll();
    for (const Project::Ptr& p : projects) {
        m_projectFilter->addItem(p->name(), p->id());
    }
    filterBar->addWidget(m_projectFilter);

    filterBar->addSpacing(20);
    filterBar->addWidget(new QLabel(tr("历史:"), this));
    m_historyCombo = new QComboBox(this);
    m_historyCombo->setEditable(true);
    m_historyCombo->setMinimumWidth(200);
    m_historyCombo->setPlaceholderText(tr("搜索历史"));
    filterBar->addWidget(m_historyCombo, 1);

    m_clearHistoryBtn = new QPushButton(tr("清除"), this);
    m_clearHistoryBtn->setStyleSheet("QPushButton { padding: 4px 10px; font-size: 12px; }");
    filterBar->addWidget(m_clearHistoryBtn);

    mainLayout->addLayout(filterBar);

    // -----------------------------------------------------------------------
    // 结果区域（分割器：左侧列表 + 右侧详情）
    // -----------------------------------------------------------------------
    m_splitter = new QSplitter(Qt::Horizontal, this);

    m_resultList = new QListWidget(this);
    m_resultList->setStyleSheet(
        "QListWidget { border: 1px solid #ddd; border-radius: 4px; }"
        "QListWidget::item { padding: 8px; border-bottom: 1px solid #eee; }"
        "QListWidget::item:selected { background: #e8f0fe; }"
        "QListWidget::item:hover { background: #f5f5f5; }");
    m_splitter->addWidget(m_resultList);

    m_detailBrowser = new QTextBrowser(this);
    m_detailBrowser->setStyleSheet(
        "QTextBrowser { border: 1px solid #ddd; border-radius: 4px; padding: 12px; }");
    m_detailBrowser->setOpenExternalLinks(false);
    m_splitter->addWidget(m_detailBrowser);

    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 2);
    m_splitter->setSizes({350, 550});

    mainLayout->addWidget(m_splitter, 1);

    // -----------------------------------------------------------------------
    // 状态栏
    // -----------------------------------------------------------------------
    m_statusLabel = new QLabel(tr("输入关键词开始搜索"), this);
    m_statusLabel->setStyleSheet("color: #666; font-size: 12px; padding: 4px 0;");
    mainLayout->addWidget(m_statusLabel);

    // -----------------------------------------------------------------------
    // 连接信号
    // -----------------------------------------------------------------------
    connect(m_searchBtn, &QPushButton::clicked, this, &SearchResultDialog::onSearch);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &SearchResultDialog::onSearch);
    connect(m_resultList, &QListWidget::itemClicked, this, &SearchResultDialog::onResultClicked);
    connect(m_resultList, &QListWidget::itemDoubleClicked, this, &SearchResultDialog::onResultDoubleClicked);
    connect(m_historyCombo, &QComboBox::textActivated, this, &SearchResultDialog::onHistorySelected);
    connect(m_clearHistoryBtn, &QPushButton::clicked, this, &SearchResultDialog::onClearHistory);
    connect(m_projectFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SearchResultDialog::onFilterChanged);

    // 加载历史
    updateHistory();
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
    const QString keyword = m_searchEdit->text().trimmed();
    if (keyword.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请输入搜索关键词"));
        return;
    }

    m_statusLabel->setText(tr("正在搜索..."));
    QApplication::setOverrideCursor(Qt::WaitCursor);

    SearchQuery query;
    query.keyword = keyword;
    query.projectId = m_projectFilter->currentData().toLongLong();
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
    m_resultList->clear();

    if (results.isEmpty()) {
        m_statusLabel->setText(tr("未找到匹配的报告"));
        m_detailBrowser->clear();
        m_detailBrowser->setHtml(
            "<div style='color: #999; text-align: center; margin-top: 50px;'>"
            "<p style='font-size: 48px;'>🔍</p>"
            "<p>未找到匹配的报告</p>"
            "<p style='font-size: 12px;'>尝试使用其他关键词</p>"
            "</div>");
        return;
    }

    m_statusLabel->setText(tr("找到 %1 个结果").arg(results.size()));

    for (const SearchResultItem& item : results) {
        QListWidgetItem* listItem = new QListWidgetItem(m_resultList);

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
    if (m_resultList->count() > 0) {
        m_resultList->setCurrentRow(0);
        onResultClicked(m_resultList->currentItem());
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

            m_detailBrowser->setHtml(html);
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
    m_searchEdit->setText(text);
    performSearch();
}

void SearchResultDialog::onClearHistory()
{
    m_searchService->clearHistory();
    updateHistory();
}

void SearchResultDialog::updateHistory()
{
    m_historyCombo->clear();
    const QStringList history = m_searchService->searchHistory();
    for (const QString& keyword : history) {
        m_historyCombo->addItem(keyword);
    }
    m_historyCombo->setCurrentIndex(-1);
}

// ===========================================================================
// 筛选变化
// ===========================================================================

void SearchResultDialog::onFilterChanged(int index)
{
    Q_UNUSED(index);
    // 如果有搜索结果，重新搜索
    if (!m_searchEdit->text().trimmed().isEmpty()) {
        performSearch();
    }
}
