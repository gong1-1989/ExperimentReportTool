/**
 * @file DataTableEditorDialog.h
 * @brief 数据表编辑器对话框头文件
 *
 * 用于详细编辑实验数据表，支持：
 * - 动态增删行列
 * - 列类型/单位/校验规则设置
 * - 单元格批量编辑
 * - 数据校验
 * - 从 CSV 导入/导出
 */

#ifndef DATA_TABLE_EDITOR_DIALOG_H
#define DATA_TABLE_EDITOR_DIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSplitter>

#include "core/models/DataTable.h"

/**
 * @brief 列属性编辑面板
 *
 * 右侧面板，用于编辑选中列的属性（名称、类型、单位、校验规则等）。
 */
class ColumnPropertyPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ColumnPropertyPanel(QWidget* parent = nullptr);

    /// 设置当前编辑的列定义
    void setColumn(const ColumnDefinition& col, int columnIndex);

    /// 获取编辑后的列定义
    ColumnDefinition column() const;

signals:
    /// 列属性变化
    void columnChanged(int columnIndex, const ColumnDefinition& col);

private slots:
    void onNameChanged(const QString& name);
    void onTypeChanged(int index);
    void onUnitChanged(const QString& unit);
    void onRequiredChanged(int state);
    void onMinChanged(double value);
    void onMaxChanged(double value);

private:
    void setupUi();
    void updateVisibility();

    int m_columnIndex;
    ColumnDefinition m_column;

    QLineEdit* m_nameEdit;
    QComboBox* m_typeCombo;
    QLineEdit* m_unitEdit;
    QCheckBox* m_requiredCheck;
    QDoubleSpinBox* m_minSpin;
    QDoubleSpinBox* m_maxSpin;
    QLabel* m_rangeLabel;
};

/**
 * @brief 数据表编辑器对话框
 */
class DataTableEditorDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param table 要编辑的数据表
     * @param parent 父窗口
     */
    explicit DataTableEditorDialog(const DataTable::Ptr& table, QWidget* parent = nullptr);

    /// 获取编辑后的数据表
    DataTable::Ptr tableData() const { return m_table; }

private slots:
    // 表格操作
    void onAddRow();
    void onAddColumn();
    void onInsertRow();
    void onInsertColumn();
    void onRemoveRow();
    void onRemoveColumn();
    void onCellChanged(int row, int col);
    void onCurrentCellChanged(int row, int col, int prevRow, int prevCol);
    void onHeaderDoubleClicked(int logicalIndex);

    // 列属性
    void onColumnChanged(int columnIndex, const ColumnDefinition& col);

    // 导入导出
    void onImportCsv();
    void onExportCsv();

    // 校验
    void onValidate();

    // 保存
    void onAccept();

    void updateStatus();

    void refreshTable();

private:
    void setupUi();
    void loadTable();
    void updateHeaders();
    void updateColumnProperties();
    bool validateInput();

    // -----------------------------------------------------------------------
    // 成员变量
    // -----------------------------------------------------------------------

    DataTable::Ptr m_table;

    // 左侧：表格编辑区
    QTableWidget* m_tableWidget;
    QPushButton* m_addRowBtn;
    QPushButton* m_addColBtn;
    QPushButton* m_insertRowBtn;
    QPushButton* m_insertColBtn;
    QPushButton* m_removeRowBtn;
    QPushButton* m_removeColBtn;
    QPushButton* m_importBtn;
    QPushButton* m_exportBtn;
    QPushButton* m_validateBtn;

    // 右侧：列属性面板
    ColumnPropertyPanel* m_columnPanel;
    QLabel* m_columnInfoLabel;

    // 底部
    QDialogButtonBox* m_buttonBox;
    QLabel* m_statusLabel;

    int m_currentColumn;
    bool m_loading;
};

#endif // DATA_TABLE_EDITOR_DIALOG_H
