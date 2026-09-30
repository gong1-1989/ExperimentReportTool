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
#include <QtConcurrent>
#include <QFutureWatcher>
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
#include "import/ImportData.h"
#include "import/ImportValidator.h"

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
 * @brief 后台解析结果（数据 + 错误信息）
 */
struct ParseOutcome {
    ImportData data;        ///< 统一解析结果
    QString errorMessage;   ///< 解析错误信息（data 为空时有效）
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
    /// 后台解析完成（回主线程更新预览）
    void onParseFinished();
    /// 导出标准模板（按当前数据表列定义）
    void on_m_exportTemplateBtn_clicked();
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
    void runValidation();
    void applyImport();

    // -----------------------------------------------------------------------
    // 成员变量
    // -----------------------------------------------------------------------

    Ui::DataImportDialog* ui;     ///< UI 界面对象（从 .ui 文件自动生成）
    DataTable::Ptr m_targetTable; ///< 目标数据表
    DataTable::Ptr m_importedTable; ///< 导入后的数据表
    ImportData m_importData;      ///< 统一解析结果
    ImportMode m_importMode;       ///< 导入模式
    QString m_currentFilePath;     ///< 当前文件路径

    /// 校验结果（预览时生成；导入前复查）
    QHash<int, int> m_columnMapping;          ///< 文件列 → 目标列 映射
    QList<CellIssue> m_issues;                ///< 校验问题列表
    QStringList m_missingRequired;            ///< 缺失的必填列（Error）
    QStringList m_unknownColumns;             ///< 未知列（Warning，忽略）

    QFutureWatcher<ParseOutcome>* m_watcher;    ///< 后台解析监视器（setFuture 替换即只响应最新任务）
};

#endif // DATA_IMPORT_DIALOG_H
