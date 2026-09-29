#ifndef VERSION_COMPARE_DIALOG_H
#define VERSION_COMPARE_DIALOG_H

#include "ui/dialogs/BaseDialog.h"
#include "data/repositories/ReportRepository.h"  // VersionInfo
#include "core/utils/VersionDiffer.h"

namespace Ui { class VersionCompareDialog; }

/**
 * @brief 版本对比对话框
 *
 * 选择两个历史版本，并排显示各自提取的可见文本，
 * 逐行对比并以颜色标记差异（删除=红、新增=绿、修改=黄）。
 */
class VersionCompareDialog : public BaseDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param versions 版本列表（按时间倒序）
     * @param parent 父窗口
     */
    explicit VersionCompareDialog(const QList<VersionInfo>& versions,
                                  QWidget* parent = nullptr);
    ~VersionCompareDialog() override;

private slots:
    void on_m_compareBtn_clicked();
    void on_m_closeBtn_clicked();

private:
    void fillComboBoxes();
    void performCompare();

    QStringList m_versionNames;
    QList<VersionInfo> m_versions;
    Ui::VersionCompareDialog* ui;
};

#endif // VERSION_COMPARE_DIALOG_H
