/**
 * @file SearchResultDialog.h
 * @brief 搜索结果对话框头文件
 *
 * 展示全文搜索结果，支持点击打开报告、结果高亮、搜索历史。
 */

#ifndef SEARCH_RESULT_DIALOG_H
#define SEARCH_RESULT_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBrowser>
#include <QSplitter>

#include "search/SearchService.h"

/**
 * @brief 搜索结果对话框
 */
class SearchResultDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     * @param initialKeyword 初始搜索关键词
     */
    explicit SearchResultDialog(QWidget* parent = nullptr,
                                 const QString& initialKeyword = QString());

    ~SearchResultDialog() override;

signals:
    /**
     * @brief 请求打开报告
     * @param reportId 报告 ID
     */
    void reportOpenRequested(qint64 reportId);

private slots:
    void onSearch();
    void onResultClicked(QListWidgetItem* item);
    void onResultDoubleClicked(QListWidgetItem* item);
    void onHistorySelected(const QString& text);
    void onClearHistory();
    void onFilterChanged(int index);

private:
    void setupUi();
    void performSearch();
    void displayResults(const QList<SearchResultItem>& results);
    void updateHistory();

    // -----------------------------------------------------------------------
    // UI 控件
    // -----------------------------------------------------------------------

    QLineEdit* m_searchEdit;           ///< 搜索框
    QPushButton* m_searchBtn;          ///< 搜索按钮
    QComboBox* m_projectFilter;        ///< 项目筛选
    QComboBox* m_historyCombo;         ///< 搜索历史
    QPushButton* m_clearHistoryBtn;    ///< 清除历史按钮

    QSplitter* m_splitter;             ///< 分割器
    QListWidget* m_resultList;         ///< 结果列表
    QTextBrowser* m_detailBrowser;     ///< 详情预览

    QLabel* m_statusLabel;             ///< 状态标签

    // -----------------------------------------------------------------------
    // 数据
    // -----------------------------------------------------------------------

    SearchService* m_searchService;    ///< 搜索服务
    QList<SearchResultItem> m_results; ///< 当前搜索结果
};

#endif // SEARCH_RESULT_DIALOG_H
