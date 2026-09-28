/**
 * @file SearchResultDialog.h
 * @brief 搜索结果对话框头文件
 *
 * 展示全文搜索结果，支持点击打开报告、结果高亮、搜索历史。
 */

#ifndef SEARCH_RESULT_DIALOG_H
#define SEARCH_RESULT_DIALOG_H

#include "BaseDialog.h"
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBrowser>
#include <QList>
#include <QSplitter>

#include "search/SearchService.h"


// 前向声明 UI 类（由 uic 工具从 .ui 文件自动生成）
namespace Ui {
class SearchResultDialog;
}
/**
 * @brief 搜索结果对话框
 */
class SearchResultDialog : public BaseDialog
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
    void on_m_searchBtn_clicked();
    void on_m_clearHistoryBtn_clicked();
    void on_m_openBtn_clicked();
    void on_m_closeBtn_clicked();
    void on_m_resultList_itemClicked(QListWidgetItem* item);
    void on_m_resultList_itemDoubleClicked(QListWidgetItem* item);
    void on_m_historyCombo_currentIndexChanged(const QString& text);
    void on_m_projectFilter_currentIndexChanged(int index);

private:

    Ui::SearchResultDialog* ui;  ///< UI 界面对象（从 .ui 文件自动生成）
    void performSearch();
    void displayResults(const QList<SearchResultItem>& results);
    void updateHistory();

    // -----------------------------------------------------------------------
    // UI 控件
    // -----------------------------------------------------------------------




    // -----------------------------------------------------------------------
    // 数据
    // -----------------------------------------------------------------------

    SearchService* m_searchService;    ///< 搜索服务
    QList<SearchResultItem> m_results; ///< 当前搜索结果
};

#endif // SEARCH_RESULT_DIALOG_H
