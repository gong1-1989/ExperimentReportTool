/**
 * @file ChartConfigDialog.h
 * @brief 图表配置对话框头文件
 *
 * 用于配置图表的数据源、类型、样式等参数。
 */

#ifndef CHART_CONFIG_DIALOG_H
#define CHART_CONFIG_DIALOG_H

#include "ui/dialogs/BaseDialog.h"
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <QTabWidget>
#include <QList>

#include "core/models/DataTable.h"

// 前向声明 UI 类（由 uic 工具从 .ui 文件自动生成）
namespace Ui {
class ChartConfigDialog;
}

/**
 * @brief 图表类型枚举
 */
enum class ChartType {
    Line,       ///< 折线图
    Bar,        ///< 柱状图
    Pie,        ///< 饼图
    Scatter,    ///< 散点图
    Area        ///< 面积图
};

/**
 * @brief 图表配置结构体
 */
struct ChartConfig {
    ChartType type;              ///< 图表类型
    QString title;               ///< 图表标题
    QString xAxisTitle;          ///< X轴标题
    QString yAxisTitle;          ///< Y轴标题
    qint64 dataTableId;         ///< 关联的数据表 ID
    int xAxisColumn;             ///< X轴数据列索引
    QList<int> yAxisColumns;     ///< Y轴数据列索引（可多选）
    bool showLegend;             ///< 显示图例
    bool showGrid;               ///< 显示网格
    bool showDataPoints;         ///< 显示数据点
    QString theme;               ///< 主题（light/dark）
    int width;                   ///< 图表宽度
    int height;                  ///< 图表高度

    ChartConfig()
        : type(ChartType::Line)
        , dataTableId(-1)
        , xAxisColumn(0)
        , showLegend(true)
        , showGrid(true)
        , showDataPoints(true)
        , theme("light")
        , width(600)
        , height(400)
    {}

    /// 序列化为 JSON
    QJsonObject toJson() const;
    /// 从 JSON 反序列化
    static ChartConfig fromJson(const QJsonObject& json);
    /// 图表类型转字符串
    static QString chartTypeToString(ChartType type);
    /// 从字符串解析图表类型
    static ChartType chartTypeFromString(const QString& str);
};

/**
 * @brief 图表配置对话框
 */
class ChartConfigDialog : public BaseDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param tables 可用的数据表列表
     * @param config 初始配置
     * @param parent 父窗口
     */
    explicit ChartConfigDialog(const DataTable::List& tables,
                                const ChartConfig& config = ChartConfig(),
                                QWidget* parent = nullptr);

    /// 析构函数
    ~ChartConfigDialog() override;

    /// 获取配置
    ChartConfig config() const { return m_config; }

private slots:
    void onTableChanged(int index);
    void onAccept();
    void onPreview();

private:
    void loadConfig();
    void updateColumnLists();
    bool validateConfig();

    // -----------------------------------------------------------------------
    // 成员变量
    // -----------------------------------------------------------------------

    Ui::ChartConfigDialog* ui;  ///< UI 界面对象（从 .ui 文件自动生成）

    /// 收集当前界面配置到 m_config
    void collectConfig();

    /// 图表渲染器（用于预览，需保持存活以维护 QChartView）
    class ChartRenderer* m_previewRenderer = nullptr;
    DataTable::List m_tables;   ///< 可用的数据表列表
    ChartConfig m_config;       ///< 图表配置
};

#endif // CHART_CONFIG_DIALOG_H
