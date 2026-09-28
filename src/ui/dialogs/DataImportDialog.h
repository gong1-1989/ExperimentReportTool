/**
 * @file DataImportDialog.h
 * @brief 数据导入对话框头文件
 *
 * 支持从 CSV 文件导入数据到数据表。
 * 功能包括：文件选择、预览、分隔符设置、表头设置、列映射、导入模式选择。
 */

#ifndef DATA_IMPORT_DIALOG_H
#define DATA_IMPORT_DIALOG_H

#include "BaseDialog.h"
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QRadioButton>
#include <QButtonGroup>

#include "core/models/DataTable.h"
#include "utils/CsvParser.h"

// 前向声明 UI 类（由 uic 工具从 .ui 文件自动生成）
namespace Ui {
class DataImportDialog;
}

/**
 * @brief 导入模式
 */
enum class ImportMode {
    Append,     ///< 追加到现有数据
    Replace,    ///< 替换现有数据
    NewTable    ///< 创建新数据表
};

/**
 * @brief 数据导入对话框
 */
class DataImportDialog : public BaseDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param table 目标数据表（用于列映射）
     * @param parent 父窗口
     */
    explicit DataImportDialog(const DataTable::Ptr& table, QWidget* parent = nullptr);
    ~DataImportDialog() override;

    /**
     * @brief 获取导入后的数据表
     */
    DataTable::Ptr importedTable() const { return m_importedTable; }

    /**
     * @brief 获取导入模式
     */
    ImportMode importMode() const { return m_importMode; }

private slots:
    void on_m_browseBtn_clicked();
    void on_m_previewBtn_clicked();
    void on_m_delimiterCombo_currentIndexChanged(int index);
    void on_m_hasHeaderCheck_stateChanged(int state);
    void on_m_newTableRadio_toggled(bool checked);
    void on_m_appendRadio_toggled(bool checked);
    void on_m_replaceRadio_toggled(bool checked);
    void on_m_importBtn_clicked();
    void on_m_cancelBtn_clicked();

private:
    bool loadAndPreview();
    void updatePreviewTable();
    void updateColumnMapping();
    void applyImport();

    // -----------------------------------------------------------------------
    // 成员变量
    // -----------------------------------------------------------------------

    Ui::DataImportDialog* ui;     ///< UI 界面对象（从 .ui 文件自动生成）
    DataTable::Ptr m_targetTable; ///< 目标数据表
    DataTable::Ptr m_importedTable; ///< 导入后的数据表
    CsvParseResult m_parseResult; ///< 解析结果
    ImportMode m_importMode;       ///< 导入模式
    QString m_currentFilePath;     ///< 当前文件路径
};

#endif // DATA_IMPORT_DIALOG_H
