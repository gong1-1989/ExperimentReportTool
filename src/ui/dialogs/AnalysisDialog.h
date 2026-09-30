/**
 * @file AnalysisDialog.h
 * @brief 数据分析对话框（B 域）
 *
 * 选择分析器 → 选择列/参数 → 执行（本地计算）→ 展示结论/结果表/图表。
 * 分析器来自 AnalyzerRegistry（内置或插件注册）。
 */

#ifndef ANALYSIS_DIALOG_H
#define ANALYSIS_DIALOG_H

#include "BaseDialog.h"
#include "core/models/DataTable.h"

#include <QDialog>
#include <QHash>

class QComboBox;
class QListWidget;
class QCheckBox;
class QLabel;
class QTableWidget;
class SimpleChartWidget;
class QPushButton;

namespace Ui { class AnalysisDialog; }

class AnalysisDialog : public BaseDialog
{
    Q_OBJECT

public:
    explicit AnalysisDialog(const DataTable::Ptr& table, QWidget* parent = nullptr);
    ~AnalysisDialog() override;

private slots:
    void onAnalyzerChanged();
    void onRunClicked();

private:
    Ui::AnalysisDialog* ui;
    SimpleChartWidget* m_chartWidget;  ///< 自绘图表（QPainter，避免 QtCharts 运行期崩溃）

    DataTable::Ptr m_table;
    QStringList m_columnNames;      ///< 列名（含单位）
    bool m_running = false;

    void updateColumnControls();
    void showResult(const QVariantMap& result);
};

#endif // ANALYSIS_DIALOG_H
