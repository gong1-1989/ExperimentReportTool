/**
 * @file VersionHistoryDialog.cpp
 * @brief 版本历史对话框实现文件
 */

#include "VersionHistoryDialog.h"
#include "ui_VersionHistoryDialog.h"  // 由 uic 工具从 .ui 文件自动生成
#include "ui/dialogs/VersionCompareDialog.h"
#include "service/ReportService.h"
#include "data/repositories/ReportRepository.h"
#include "core/utils/Logger.h"
#include "core/utils/AppTheme.h"
#include "core/utils/AppDimensions.h"
#include "core/utils/AppConfig.h"

#include <QMessageBox>
#include "ui/UiHelper.h"
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>
#include <QTimer>
#include <QLabel>
#include <QTextDocument>

// ===========================================================================
// 构造与析构
// ===========================================================================

VersionHistoryDialog::VersionHistoryDialog(qint64 reportId, QWidget* parent)
    : BaseDialog(parent)
    , ui(new Ui::VersionHistoryDialog)
    , m_reportId(reportId)
{
    ui->setupUi(this);
    // 槽函数通过 uic 自动连接（on_m_closeBtn_clicked 等）
    loadVersions();
    setWindowTitle(tr("版本历史"));
    resize(AppDimensions::Window::DialogLargeWidth, AppDimensions::Window::DialogLargeHeight);
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
    const auto versions = ReportService::getVersions(m_reportId);

    if (versions.isEmpty()) {
        ui->m_statusLabel->setText(tr("暂无历史版本"));
        ui->m_previewBrowser->setHtml(
            QString("<div style='color: %1; text-align: center; margin-top: 80px;'>"
                    "<p style='font-size: %2px;'>📋</p>"
                    "<p>暂无历史版本</p>"
                    "<p style='font-size: %3px;'>点击「保存当前版本」创建第一个版本</p>"
                    "</div>")
                .arg(AppTheme::Color::TextSecondary)
                .arg(AppTheme::FontSize::Massive)
                .arg(AppTheme::FontSize::Small));
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
        info.content = ReportService::getVersionContent(info.versionId);

        m_versions.append(info);

        // 添加到列表
        QListWidgetItem* item = new QListWidgetItem(ui->m_versionList);
        const QString name = info.snapshotName.isEmpty()
            ? tr("版本 #%1").arg(info.versionId)
            : info.snapshotName;

        QString displayText = QString(
            "<div style='padding: %1px 0;'>"
            "<div style='font-weight: bold; font-size: %2px; color: %3;'>%4</div>"
            "<div style='font-size: %5px; color: %6; margin-top: %7px;'>"
            "ID: %8 | %9 字"
            "</div>"
            "</div>"
        ).arg(AppTheme::Spacing::Tiny)
         .arg(AppTheme::FontSize::Medium)
         .arg(AppTheme::Color::TextPrimary)
         .arg(name.toHtmlEscaped())
         .arg(AppTheme::FontSize::ExtraSmall)
         .arg(AppTheme::Color::TextSecondary)
         .arg(AppTheme::Spacing::Tiny)
         .arg(info.versionId)
         .arg(info.content.length());

        // 使用 QLabel 作为 item widget 以支持 HTML 富文本渲染
        QLabel* label = new QLabel(displayText);
        label->setTextFormat(Qt::RichText);
        label->setStyleSheet(
            QString("padding: %1px %2px; background: transparent;")
                .arg(AppTheme::Spacing::Medium).arg(AppTheme::Spacing::Large));
        ui->m_versionList->setItemWidget(item, label);

        item->setData(Qt::UserRole, info.versionId);
        item->setSizeHint(QSize(0, 60));
    }

    // 选中第一个
    if (ui->m_versionList->count() > 0) {
        ui->m_versionList->setCurrentRow(0);
        on_m_versionList_itemClicked(ui->m_versionList->currentItem());
    }
}

// ===========================================================================
// 版本选中
// ===========================================================================

void VersionHistoryDialog::on_m_versionList_itemClicked(QListWidgetItem* item)
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

    // 直接复用 PrintManager 的渲染逻辑（和 PDF 导出、打印预览完全一致）
    const QString contentHtml = renderVersionContent(version.content);

    // 构建完整的预览页面：头部信息 + 内容预览
    const QString html = QString(
        "<html><head><meta charset='utf-8'><style>"
        "body { font-family: 'Microsoft YaHei', sans-serif; font-size: 11pt; line-height: 1.8; color: %1; margin: 0; padding: %2px; }"
        ".header { border-bottom: 2px solid %3; padding-bottom: 10px; margin-bottom: %2px; }"
        ".header h2 { margin: 0 0 8px 0; color: %4; font-size: %5px; }"
        ".header .meta { color: %6; font-size: %7px; }"
        ".header .meta span { margin-right: 16px; }"
        ".content-title { color: %1; font-size: %8px; font-weight: bold; margin: %2px 0 8px 0; }"
        ".content { background: %9; border: 1px solid %10; border-radius: %11px; padding: 20px; min-height: 200px; }"
        ".empty { color: %6; text-align: center; padding: 40px 0; }"
        "</style></head><body>"
        "<div class='header'>"
        "<div style='font-size:20px;font-weight:bold;'>%12</div>"
        "<div class='meta'>"
        "<span><strong>版本 ID:</strong> %13</span>"
        "<span><strong>内容大小:</strong> %14 字节</span>"
        "</div>"
        "</div>"
        "<div class='content-title'>内容预览</div>"
        "<div class='content'>%15</div>"
        "</body></html>"
    ).arg(AppTheme::Color::Gray333)
     .arg(AppTheme::Spacing::Large)
     .arg(AppTheme::Color::Primary)
     .arg(AppTheme::Color::TextPrimary)
     .arg(AppTheme::FontSize::Large)
     .arg(AppTheme::Color::Gray666)
     .arg(AppTheme::FontSize::Small)
     .arg(AppTheme::FontSize::Normal)
     .arg(AppTheme::Color::BgGray)
     .arg(AppTheme::Color::BorderExtraLight)
     .arg(AppTheme::Radius::Medium)
     .arg(name.toHtmlEscaped())
     .arg(version.versionId)
     .arg(version.content.length())
     .arg(contentHtml.isEmpty() ? "<div class='empty'>（空内容）</div>" : contentHtml);

    ui->m_previewBrowser->setHtml(html);
}

// ===========================================================================
// 从 JSON 内容提取纯文本
// ===========================================================================

// ===========================================================================
// 版本内容渲染（直接复用 PrintManager，和 PDF 导出、打印预览完全一致）
// ===========================================================================

QString VersionHistoryDialog::renderVersionContent(const QString& contentJson)
{
    if (contentJson.isEmpty()) return QString();

    // 构造临时 Report 对象，把版本内容设置进去
    Report::Ptr report = Report::create();
    report->contentFromJson(contentJson);

    // 使用 PrintManager 渲染（和打印预览、PDF 导出完全相同的逻辑）
    PrintConfig config;
    config.includeTitle = false;   // 版本预览不需要报告标题
    config.includeMeta = false;    // 版本预览不需要元信息
    config.includeTableOfContents = false;

    PrintManager printManager;
    QTextDocument* doc = printManager.renderDocument(report, config);
    if (!doc) return QString();

    // 提取渲染后的 HTML（QTextDocument 会自动处理图片、表格、图表等）
    const QString html = doc->toHtml();
    delete doc;

    // 从完整 HTML 文档中提取 body 内容，嵌入到版本预览页面中
    if (!html.contains("<body", Qt::CaseInsensitive)) {
        return html;
    }
    const int bodyStart = html.indexOf("<body", 0, Qt::CaseInsensitive);
    const int bodyTagEnd = html.indexOf('>', bodyStart);
    const int bodyEnd = html.indexOf("</body>", bodyTagEnd, Qt::CaseInsensitive);
    if (bodyStart < 0 || bodyTagEnd < 0 || bodyEnd < 0) {
        return html;
    }
    return html.mid(bodyTagEnd + 1, bodyEnd - bodyTagEnd - 1);
}

// ===========================================================================
// 恢复版本
// ===========================================================================

void VersionHistoryDialog::on_m_restoreBtn_clicked()
{
    if (m_currentVersion.versionId <= 0) return;

    const QString name = m_currentVersion.snapshotName.isEmpty()
        ? tr("版本 #%1").arg(m_currentVersion.versionId)
        : m_currentVersion.snapshotName;

    if (!UiHelper::confirm(this,
                           tr("确认恢复"),
                           tr("确定要恢复到「%1」吗？\n\n"
                              "当前未保存的内容将被覆盖。\n"
                              "建议先保存当前版本。").arg(name))) return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool success = ReportService::restoreVersion(m_reportId, m_currentVersion.versionId);
    QApplication::restoreOverrideCursor();

    if (success) {
        UiHelper::info(this, tr("恢复成功"),
            tr("已恢复到「%1」").arg(name));
        accept();
    } else {
        UiHelper::error(this, tr("恢复失败"),
            tr("恢复版本时发生错误"));
    }
}

// ===========================================================================
// 删除版本
// ===========================================================================

void VersionHistoryDialog::on_m_deleteBtn_clicked()
{
    if (m_currentVersion.versionId <= 0) return;

    const QString name = m_currentVersion.snapshotName.isEmpty()
        ? tr("版本 #%1").arg(m_currentVersion.versionId)
        : m_currentVersion.snapshotName;

    if (!UiHelper::confirm(this,
                           tr("确认删除"),
                           tr("确定要删除「%1」吗？\n此操作不可撤销。").arg(name))) return;

    const bool success = ReportService::deleteVersion(m_currentVersion.versionId);
    if (success) {
        showStatusMessage(tr("版本已删除"));
        loadVersions();
    } else {
        UiHelper::error(this, tr("删除失败"), tr("删除版本时发生错误"));
    }
}

// ===========================================================================
// 保存新版本
// ===========================================================================

void VersionHistoryDialog::on_m_saveBtn_clicked()
{
    QString name = ui->m_versionNameEdit->text().trimmed();
    if (name.isEmpty()) {
        name = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    const qint64 versionId = ReportService::saveVersion(m_reportId, name);
    QApplication::restoreOverrideCursor();

    if (versionId > 0) {
        UiHelper::info(this, tr("保存成功"),
            tr("版本「%1」已保存").arg(name));
        ui->m_versionNameEdit->clear();
        emit versionSaved(m_reportId, versionId);
        loadVersions();
    } else {
        UiHelper::error(this, tr("保存失败"), tr("保存版本时发生错误"));
    }
}

// ===========================================================================
// 对比（简化实现）
// ===========================================================================

void VersionHistoryDialog::on_m_compareBtn_clicked()
{
    if (m_versions.size() < 2) {
        UiHelper::info(this, tr("版本对比"), tr("至少需要保存两个版本才能对比"));
        return;
    }

    VersionCompareDialog dialog(m_versions, this);
    dialog.exec();
}

// ===========================================================================
// 刷新
// ===========================================================================

void VersionHistoryDialog::on_m_refreshBtn_clicked()
{
    loadVersions();
    showStatusMessage(tr("已刷新"));
}

void VersionHistoryDialog::on_m_closeBtn_clicked()
{
    reject();  // 关闭对话框
}

// ===========================================================================
// 辅助方法
// ===========================================================================

void VersionHistoryDialog::showStatusMessage(const QString& message)
{
    ui->m_statusLabel->setText(message);
    QTimer::singleShot(AppDimensions::Delay::StatusMessage, this, [this]() { ui->m_statusLabel->clear(); });
}

// 由于 showStatusMessage 使用了 QTimer，需要 include
// 这里在文件顶部已经有足够的 include，QTimer 是 Qt 核心组件
