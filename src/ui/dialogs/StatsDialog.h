#pragma once
// ===========================================================================
// 报表中心对话框（G 域）
// ===========================================================================

#include "BaseDialog.h"
#include "extension/StatsProvider.h"

class QComboBox;
class SimpleChartWidget;

namespace Ui { class StatsDialog; }

class StatsDialog : public BaseDialog {
    Q_OBJECT
public:
    /// @param scope 基础统计范围（组长传入 groupId 锁定本组；超管/总管传全范围）
    explicit StatsDialog(const StatsScope& scope, QWidget* parent = nullptr);
    ~StatsDialog() override;

private slots:
    void onRun();
    void onExportCsv();

private:
    void updateProjectCombo();
    void renderChart(const StatsResult& result);

    Ui::StatsDialog* ui;
    SimpleChartWidget* m_chartWidget;  ///< 自绘图表（QPainter，避免 QtCharts 运行期崩溃）
    StatsScope m_scope;    ///< 基础范围
    bool m_groupLocked;    ///< 组长视图：范围锁定本组
};
