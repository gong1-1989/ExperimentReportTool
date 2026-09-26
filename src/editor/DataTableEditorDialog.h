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


// 前向声明 UI 类（由 uic 工具从 .ui 文件自动生成）
namespace Ui {
class DataTableEditorDialog;
}
/**
 * @brief 列属性编辑面板
 *
 * 右侧面板，用于编辑选中列的属性（名称、类型、单位、校验规则等）。
 * 动态创建控件，不使用 .ui 文件。
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
    void onRequiredChanged(Qt::CheckState state);  ///< Qt6 使用 checkStateChanged 信号
    void onMinChanged(double value);
    void onMaxChanged(double value);

private:
    /// 动态创建 UI 控件
    void setupUi();

    /// 根据列类型更新控件可见性
    void updateVisibility();

    // 控件成员变量（动态创建）
    QLineEdit* m_nameEdit;        ///< 列名称编辑框
    QComboBox* m_typeCombo;       ///< 列类型下拉框
    QLineEdit* m_unitEdit;        ///< 单位编辑框
    QCheckBox* m_requiredCheck;   ///< 必填复选框
    QDoubleSpinBox* m_minSpin;    ///< 最小值
    QDoubleSpinBox* m_maxSpin;    ///< 最大值
    QLabel* m_rangeLabel;         ///< 数值范围标签

    int m_columnIndex;            ///< 当前编辑的列索引
    ColumnDefinition m_column;    ///< 当前列定义
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
    /// 析构函数
    ~DataTableEditorDialog() override;

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

private:
    void loadTable();
    void updateHeaders();
    void updateColumnProperties();
    void updateStatus();       ///< 更新状态栏显示
    bool validateInput();

    // -----------------------------------------------------------------------
    // 成员变量
    // -----------------------------------------------------------------------

    Ui::DataTableEditorDialog* ui;  ///< UI 界面对象（从 .ui 文件自动生成）
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
