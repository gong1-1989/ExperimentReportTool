/**
 * @file ChartBlockEditor.h
 * @brief 图表块编辑器头文件
 *
 * 用于在报告中插入图表，支持基于数据表的图表生成。
 */

#ifndef CHART_BLOCK_EDITOR_H
#define CHART_BLOCK_EDITOR_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

#include "editor/BlockEditor.h"
#include "chart/ChartConfigDialog.h"  // ChartConfig 结构体定义

// 前向声明
class ChartRenderer;
class DataTable;

/**
 * @brief 图表块编辑器
 */
class ChartBlockEditor : public BlockEditor
{
    Q_OBJECT

public:
    explicit ChartBlockEditor(const ContentBlock& block, QWidget* parent = nullptr);
    ~ChartBlockEditor() override;

    QJsonObject blockData() const override;
    void setBlockData(const QJsonObject& data) override;
    BlockType blockType() const override { return BlockType::Chart; }
    bool isEmpty() const override { return m_config.dataTableId == 0; }

    /// 设置报告 ID（用于查找该报告下的数据表）
    Q_INVOKABLE void setReportId(qint64 reportId);

private slots:
    void onConfigureChart();
    void onEditData();

private:
    void setupChartArea();
    void renderChart();

    /// 将报告中的表格块转换为临时 DataTable
    DataTable::Ptr tableBlockToDataTable(const ContentBlock& block, int index) const;

    /// 根据 ID 获取数据表（正 ID 从数据库，负 ID 从报告表格块）
    DataTable::Ptr getDataTableById(qint64 id) const;

    QWidget* m_chartContainer;
    QLabel* m_placeholderLabel;
    QPushButton* m_configBtn;
    QPushButton* m_editDataBtn;
    ChartRenderer* m_renderer;
    ChartConfig m_config;
    qint64 m_reportId;
};

#endif // CHART_BLOCK_EDITOR_H
