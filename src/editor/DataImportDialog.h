/**
 * @file DataImportDialog.h
 * @brief 数据导入对话框头文件
 *
 * 支持从 CSV 文件导入数据到数据表。
 * 功能包括：文件选择、预览、分隔符设置、表头设置、列映射、导入模式选择。
 */

#ifndef DATA_IMPORT_DIALOG_H
#define DATA_IMPORT_DIALOG_H

#include <QDialog>
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
class DataImportDialog : public QDialog
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
    void onBrowseFile();
    void onPreview();
    void onDelimiterChanged(int index);
    void onHasHeaderChanged(int state);
    void onImportModeChanged(int id);
    void onImport();

private:
    void setupUi();
    bool loadAndPreview();
    void updatePreviewTable();
    void updateColumnMapping();
    void applyImport();

    // -----------------------------------------------------------------------
    // UI 控件
    // -----------------------------------------------------------------------

    QLineEdit* m_filePathEdit;         ///< 文件路径
    QPushButton* m_browseBtn;           ///< 浏览按钮
    QPushButton* m_previewBtn;          ///< 预览按钮

    QComboBox* m_delimiterCombo;        ///< 分隔符选择
    QCheckBox* m_hasHeaderCheck;        ///< 第一行是表头
    QComboBox* m_encodingCombo;         ///< 编码选择

    QTableWidget* m_previewTable;       ///< 预览表格
    QLabel* m_infoLabel;                 ///< 信息标签

    // 导入模式
    QRadioButton* m_appendRadio;         ///< 追加模式
    QRadioButton* m_replaceRadio;        ///< 替换模式
    QRadioButton* m_newTableRadio;       ///< 新表模式
    QButtonGroup* m_modeGroup;           ///< 模式按钮组

    QPushButton* m_importBtn;            ///< 导入按钮
    QPushButton* m_cancelBtn;            ///< 取消按钮

    // -----------------------------------------------------------------------
    // 数据
    // -----------------------------------------------------------------------

    DataTable::Ptr m_targetTable;        ///< 目标数据表
    DataTable::Ptr m_importedTable;      ///< 导入后的数据表
    CsvParseResult m_parseResult;        ///< 解析结果
    ImportMode m_importMode;              ///< 导入模式
    QString m_currentFilePath;            ///< 当前文件路径
};

#endif // DATA_IMPORT_DIALOG_H
