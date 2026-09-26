/**
 * @file VersionHistoryDialog.cpp
 * @brief 版本历史对话框实现文件
 */

#include "VersionHistoryDialog.h"
#include "ui_VersionHistoryDialog.h"  // 由 uic 工具从 .ui 文件自动生成
#include "data/repositories/ReportRepository.h"
#include "core/utils/Logger.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>
#include <QTimer>

// ===========================================================================
// 构造与析构
// ===========================================================================

VersionHistoryDialog::VersionHistoryDialog(qint64 reportId, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::VersionHistoryDialog)
    , m_reportId(reportId)
{
    ui->setupUi(this);
    loadVersions();
    setWindowTitle(tr("版本历史"));
    resize(900, 600);
}

VersionHistoryDialog::~VersionHistoryDialog()
{
    delete ui;
}

// ===========================================================================
// 加载版本列表
// ===========================================================================

void VersionHistoryDialog::loadVersions()
{
    ui->m_versionList->clear();
    m_versions.clear();

    // 从数据库获取版本列表
    const auto versions = ReportRepository::getVersions(m_reportId);

    if (versions.isEmpty()) {
        ui->m_statusLabel->setText(tr("暂无历史版本"));
        ui->m_previewBrowser->setHtml(
            "<div style='color: #999; text-align: center; margin-top: 80px;'>"
            "<p style='font-size: 48px;'>📋</p>"
            "<p>暂无历史版本</p>"
            "<p style='font-size: 12px;'>点击「保存当前版本」创建第一个版本</p>"
            "</div>");
        return;
    }

    ui->m_statusLabel->setText(tr("共 %1 个版本").arg(versions.size()));

    for (const auto& version : versions) {
        VersionInfo info;
        info.versionId = version.first;
        info.reportId = m_reportId;
        info.snapshotName = version.second;

        // 尝试从名称中解析时间（如果名称为空，用 ID 作为标识）
        // 实际创建时间需要从数据库查询，这里简化处理
        info.createdAt = QDateTime::currentDateTime();

        // 获取版本内容
        info.content = ReportRepository::getVersionContent(info.versionId);

        m_versions.append(info);

        // 添加到列表
        QListWidgetItem* item = new QListWidgetItem(ui->m_versionList);
        const QString name = info.snapshotName.isEmpty()
            ? tr("版本 #%1").arg(info.versionId)
            : info.snapshotName;

        QString displayText = QString(
            "<div style='padding: 2px 0;'>"
            "<div style='font-weight: bold; font-size: 13px; color: #1a1a1a;'>%1</div>"
            "<div style='font-size: 11px; color: #888; margin-top: 2px;'>"
            "ID: %2 | %3 字"
            "</div>"
            "</div>"
        ).arg(name.toHtmlEscaped())
         .arg(info.versionId)
         .arg(info.content.length());

        item->setText(displayText);
        item->setData(Qt::UserRole, info.versionId);
        item->setSizeHint(QSize(0, 60));
    }

    // 选中第一个
    if (ui->m_versionList->count() > 0) {
        ui->m_versionList->setCurrentRow(0);
        onVersionSelected(ui->m_versionList->currentItem());
    }
}

// ===========================================================================
// 版本选中
// ===========================================================================

void VersionHistoryDialog::onVersionSelected(QListWidgetItem* item)
{
    if (!item) return;

    const qint64 versionId = item->data(Qt::UserRole).toLongLong();

    for (const VersionInfo& version : m_versions) {
        if (version.versionId == versionId) {
            m_currentVersion = version;
            displayVersion(version);
            ui->m_restoreBtn->setEnabled(true);
            ui->m_deleteBtn->setEnabled(true);
            ui->m_compareBtn->setEnabled(true);
            break;
        }
    }
}

// ===========================================================================
// 显示版本预览
// ===========================================================================

void VersionHistoryDialog::displayVersion(const VersionInfo& version)
{
    const QString name = version.snapshotName.isEmpty()
        ? tr("版本 #%1").arg(version.versionId)
        : version.snapshotName;

    // 提取纯文本预览
    const QString plainText = extractPlainText(version.content);

    QString html = QString(
        "<div style='padding: 8px;'>"
        "<h2 style='color: #1a1a1a; border-bottom: 2px solid #4A90D9; padding-bottom: 8px;'>%1</h2>"
        "<p style='color: #666; font-size: 13px;'>"
        "<strong>版本 ID:</strong> %2<br>"
        "<strong>内容大小:</strong> %3 字节"
        "</p>"
        "<hr style='border: none; border-top: 1px solid #eee; margin: 16px 0;'>"
        "<h3 style='color: #333;'>内容预览</h3>"
        "<div style='background: #f8f9fa; padding: 16px; border-radius: 6px; "
        "font-size: 14px; line-height: 1.8; color: #333; max-height: 400px; overflow-y: auto;'>%4</div>"
        "</div>"
    ).arg(name.toHtmlEscaped())
     .arg(version.versionId)
     .arg(version.content.length())
     .arg(plainText.isEmpty() ? tr("（空内容）") : plainText.toHtmlEscaped().replace("\n", "<br>"));

    ui->m_previewBrowser->setHtml(html);
}

// ===========================================================================
// 从 JSON 内容提取纯文本
// ===========================================================================

QString VersionHistoryDialog::extractPlainText(const QString& contentJson)
{
    if (contentJson.isEmpty()) return QString();

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(contentJson.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isArray()) {
        return contentJson;  // 不是 JSON，直接返回
    }

    QString text;
    const QJsonArray blocks = doc.array();
    for (const QJsonValue& value : blocks) {
        if (!value.isObject()) continue;
        const QJsonObject block = value.toObject();
        const QString type = block.value("type").toString();
        const QJsonObject data = block.value("data").toObject();

        if (type == "heading1" || type == "heading2" || type == "heading3" || type == "paragraph" || type == "quote") {
            text += data.value("text").toString() + "\n";
        } else if (type == "bullet_list" || type == "numbered_list") {
            if (data.value("items").isArray()) {
                for (const QJsonValue& item : data.value("items").toArray()) {
                    text += "- " + item.toString() + "\n";
                }
            }
        } else if (type == "code_block") {
            text += data.value("code").toString() + "\n";
        }
        text += "\n";
    }

    return text.trimmed();
}

// ===========================================================================
// 恢复版本
// ===========================================================================

void VersionHistoryDialog::onRestore()
{
    if (m_currentVersion.versionId <= 0) return;

    const QString name = m_currentVersion.snapshotName.isEmpty()
        ? tr("版本 #%1").arg(m_currentVersion.versionId)
        : m_currentVersion.snapshotName;

    const auto result = QMessageBox::question(
        this, tr("确认恢复"),
        tr("确定要恢复到「%1」吗？\n\n"
           "当前未保存的内容将被覆盖。\n"
           "建议先保存当前版本。").arg(name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (result != QMessageBox::Yes) return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool success = ReportRepository::restoreVersion(m_reportId, m_currentVersion.versionId);
    QApplication::restoreOverrideCursor();

    if (success) {
        QMessageBox::information(this, tr("恢复成功"),
            tr("已恢复到「%1」").arg(name));
        emit versionRestored(m_reportId, m_currentVersion.versionId);
        accept();
    } else {
        QMessageBox::critical(this, tr("恢复失败"),
            tr("恢复版本时发生错误"));
    }
}

// ===========================================================================
// 删除版本
// ===========================================================================

void VersionHistoryDialog::onDelete()
{
    if (m_currentVersion.versionId <= 0) return;

    const QString name = m_currentVersion.snapshotName.isEmpty()
        ? tr("版本 #%1").arg(m_currentVersion.versionId)
        : m_currentVersion.snapshotName;

    const auto result = QMessageBox::question(
        this, tr("确认删除"),
        tr("确定要删除「%1」吗？\n此操作不可撤销。").arg(name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (result != QMessageBox::Yes) return;

    const bool success = ReportRepository::deleteVersion(m_currentVersion.versionId);
    if (success) {
        showStatusMessage(tr("版本已删除"));
        loadVersions();
    } else {
        QMessageBox::critical(this, tr("删除失败"), tr("删除版本时发生错误"));
    }
}

// ===========================================================================
// 保存新版本
// ===========================================================================

void VersionHistoryDialog::onSaveNewVersion()
{
    QString name = ui->m_versionNameEdit->text().trimmed();
    if (name.isEmpty()) {
        name = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    const qint64 versionId = ReportRepository::saveVersion(m_reportId, name);
    QApplication::restoreOverrideCursor();

    if (versionId > 0) {
        QMessageBox::information(this, tr("保存成功"),
            tr("版本「%1」已保存").arg(name));
        ui->m_versionNameEdit->clear();
        emit versionSaved(m_reportId, versionId);
        loadVersions();
    } else {
        QMessageBox::critical(this, tr("保存失败"), tr("保存版本时发生错误"));
    }
}

// ===========================================================================
// 对比（简化实现）
// ===========================================================================

void VersionHistoryDialog::onCompare()
{
    // 简化版本：显示提示，完整的 diff 对比需要额外实现
    QMessageBox::information(this, tr("版本对比"),
        tr("版本对比功能将在后续版本中实现。\n\n"
           "当前可通过预览查看历史版本内容。"));
}

// ===========================================================================
// 刷新
// ===========================================================================

void VersionHistoryDialog::onRefresh()
{
    loadVersions();
    showStatusMessage(tr("已刷新"));
}

// ===========================================================================
// 辅助方法
// ===========================================================================

void VersionHistoryDialog::showStatusMessage(const QString& message)
{
    ui->m_statusLabel->setText(message);
    QTimer::singleShot(3000, this, [this]() { ui->m_statusLabel->clear(); });
}

// 由于 showStatusMessage 使用了 QTimer，需要 include
// 这里在文件顶部已经有足够的 include，QTimer 是 Qt 核心组件
