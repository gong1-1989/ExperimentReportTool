/**
 * @file VersionCompareDialog.cpp
 * @brief 版本对比对话框实现
 */

#include "VersionCompareDialog.h"
#include "ui_VersionCompareDialog.h"

#include "core/models/Report.h"  // ContentBlock 定义（与 Report 同文件）

#include "ui/UiHelper.h"

#include <QComboBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QTextBrowser>
#include <QTextDocument>
#include <QSet>
#include <QVector>
#include <QPair>

#include <QtCore/Qt>
#include <QtMath>

// ===========================================================================
// 构造与析构
// ===========================================================================

VersionCompareDialog::VersionCompareDialog(const QList<VersionInfo>& versions,
                                           QWidget* parent)
    : BaseDialog(parent)
    , m_versions(versions)
    , ui(new Ui::VersionCompareDialog)
{
    ui->setupUi(this);

    fillComboBoxes();
}

VersionCompareDialog::~VersionCompareDialog()
{
    delete ui;
}

// ===========================================================================
// 版本选择
// ===========================================================================

void VersionCompareDialog::fillComboBoxes()
{
    // 版本列表按时间倒序（最新的在前）
    // 默认：A = 最新版本，B = 倒数第二个版本（若存在）
    for (const VersionInfo& version : m_versions) {
        const QString name = version.snapshotName.isEmpty()
            ? tr("版本 #%1").arg(version.versionId)
            : version.snapshotName;
        m_versionNames.append(name);
        ui->m_verACombo->addItem(name);
        ui->m_verBCombo->addItem(name);
    }

    if (m_versions.size() >= 2) {
        ui->m_verBCombo->setCurrentIndex(1);
    }

    if (m_versions.size() < 2) {
        ui->m_compareBtn->setEnabled(false);
        ui->m_statLabel->setText(tr("至少需要两个版本才能对比"));
    }
}

// ===========================================================================
// 对比逻辑
// ===========================================================================

void VersionCompareDialog::on_m_compareBtn_clicked()
{
    performCompare();
}

void VersionCompareDialog::on_m_closeBtn_clicked()
{
    reject();
}

void VersionCompareDialog::performCompare()
{
    if (m_versions.size() < 2) return;

    const int idxA = ui->m_verACombo->currentIndex();
    const int idxB = ui->m_verBCombo->currentIndex();
    if (idxA < 0 || idxB < 0) return;
    if (idxA == idxB) {
        UiHelper::warning(this, tr("提示"), tr("请选择两个不同的版本进行对比"));
        return;
    }

    const QStringList blocksA = VersionDiffer::extractBlocks(m_versions.at(idxA).content);
    const QStringList blocksB = VersionDiffer::extractBlocks(m_versions.at(idxB).content);

    const VersionDiffer::DiffResult result = VersionDiffer::buildBlockDiff(blocksA, blocksB);

    ui->m_textA->setHtml(result.leftHtml);
    ui->m_textB->setHtml(result.rightHtml);

    ui->m_statLabel->setText(
        tr("版本 A: %1 块 | 版本 B: %2 块 | 相同: %3 | 差异: %4")
            .arg(blocksA.size()).arg(blocksB.size())
            .arg(result.sameCount).arg(result.diffCount));
}
