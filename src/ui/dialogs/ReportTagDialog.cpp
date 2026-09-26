/**
 * @file ReportTagDialog.cpp
 * @brief 报告标签选择对话框实现文件
 */

#include "ReportTagDialog.h"
#include "ui_ReportTagDialog.h"  // 由 uic 工具从 .ui 文件自动生成
#include "data/repositories/TagRepository.h"
#include "core/utils/Logger.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QCheckBox>

// ===========================================================================
// 构造与析构
// ===========================================================================

ReportTagDialog::ReportTagDialog(qint64 reportId, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ReportTagDialog)
    , m_reportId(reportId)
{
    ui->setupUi(this);
    loadTags();
    loadSelectedTags();
    setWindowTitle(tr("选择标签"));
    resize(500, 500);
}

ReportTagDialog::~ReportTagDialog()
{
    delete ui;
}

// ===========================================================================
// 加载标签
// ===========================================================================

void ReportTagDialog::loadTags()
{
    ui->m_tagList->clear();
    m_allTags = TagRepository::findAll();

    for (const Tag::Ptr& tag : m_allTags) {
        QListWidgetItem* item = new QListWidgetItem(ui->m_tagList);

        const QColor color = tag->effectiveColor();
        const QString displayText = QString(
            "<span style='display: inline-block; width: 12px; height: 12px; "
            "border-radius: 3px; background: %1; margin-right: 8px; vertical-align: middle;'></span>"
            "<span>%2</span>"
            "<span style='color: #999; font-size: 11px; float: right;'>%3 篇</span>"
        ).arg(color.name()).arg(tag->name().toHtmlEscaped()).arg(tag->usageCount());

        item->setText(displayText);
        item->setData(Qt::UserRole, tag->id());
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        item->setSizeHint(QSize(0, 36));
    }

    if (m_allTags.isEmpty()) {
        QListWidgetItem* item = new QListWidgetItem(tr("暂无标签，点击「新建标签」创建"), ui->m_tagList);
        item->setFlags(Qt::NoItemFlags);
    }
}

void ReportTagDialog::loadSelectedTags()
{
    if (m_reportId <= 0) return;

    const Tag::List selected = TagRepository::findByReport(m_reportId);
    m_selectedIds.clear();

    for (const Tag::Ptr& tag : selected) {
        m_selectedIds.append(tag->id());
        // 在列表中勾选
        for (int i = 0; i < ui->m_tagList->count(); ++i) {
            QListWidgetItem* item = ui->m_tagList->item(i);
            if (item->data(Qt::UserRole).toLongLong() == tag->id()) {
                item->setCheckState(Qt::Checked);
                break;
            }
        }
    }

    updateSelectedLabel();
}

// ===========================================================================
// 新建标签
// ===========================================================================

void ReportTagDialog::onNewTag()
{
    bool ok;
    const QString name = QInputDialog::getText(
        this, tr("新建标签"), tr("标签名称:"), QLineEdit::Normal, QString(), &ok);

    if (!ok || name.trimmed().isEmpty()) return;

    const QString trimmed = name.trimmed();

    // 检查重名
    if (TagRepository::exists(trimmed)) {
        QMessageBox::information(this, tr("提示"), tr("标签已存在"));
        return;
    }

    Tag::Ptr tag = Tag::create(trimmed);
    if (TagRepository::save(tag)) {
        QMessageBox::information(this, tr("成功"), tr("标签已创建"));
        loadTags();
        loadSelectedTags();
    } else {
        QMessageBox::critical(this, tr("失败"), tr("创建标签失败"));
    }
}

// ===========================================================================
// 勾选变化
// ===========================================================================

void ReportTagDialog::onItemChanged(QListWidgetItem* item)
{
    if (!item || !item->data(Qt::UserRole).isValid()) return;

    const qint64 tagId = item->data(Qt::UserRole).toLongLong();
    if (item->checkState() == Qt::Checked) {
        if (!m_selectedIds.contains(tagId)) {
            m_selectedIds.append(tagId);
        }
    } else {
        m_selectedIds.removeAll(tagId);
    }

    updateSelectedLabel();
}

void ReportTagDialog::updateSelectedLabel()
{
    QStringList names;
    for (const Tag::Ptr& tag : m_allTags) {
        if (m_selectedIds.contains(tag->id())) {
            names.append(tag->name());
        }
    }

    if (names.isEmpty()) {
        ui->m_selectedLabel->setText(tr("已选: 0 个标签"));
    } else {
        ui->m_selectedLabel->setText(tr("已选 %1 个: %2")
            .arg(names.size()).arg(names.join(", ")));
    }
}

// ===========================================================================
// 搜索
// ===========================================================================

void ReportTagDialog::onSearchTextChanged(const QString& text)
{
    // 保存当前勾选状态
    QList<qint64> checked = m_selectedIds;

    // 重新加载（过滤）
    ui->m_tagList->clear();
    m_allTags = text.isEmpty()
        ? TagRepository::findAll()
        : TagRepository::search(text);

    for (const Tag::Ptr& tag : m_allTags) {
        QListWidgetItem* item = new QListWidgetItem(ui->m_tagList);
        const QColor color = tag->effectiveColor();
        const QString displayText = QString(
            "<span style='display: inline-block; width: 12px; height: 12px; "
            "border-radius: 3px; background: %1; margin-right: 8px; vertical-align: middle;'></span>"
            "<span>%2</span>"
            "<span style='color: #999; font-size: 11px; float: right;'>%3 篇</span>"
        ).arg(color.name()).arg(tag->name().toHtmlEscaped()).arg(tag->usageCount());

        item->setText(displayText);
        item->setData(Qt::UserRole, tag->id());
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(checked.contains(tag->id()) ? Qt::Checked : Qt::Unchecked);
        item->setSizeHint(QSize(0, 36));
    }
}

// ===========================================================================
// 全选/全不选
// ===========================================================================

void ReportTagDialog::onSelectAll()
{
    for (int i = 0; i < ui->m_tagList->count(); ++i) {
        QListWidgetItem* item = ui->m_tagList->item(i);
        if (item->flags() & Qt::ItemIsUserCheckable) {
            item->setCheckState(Qt::Checked);
        }
    }
}

void ReportTagDialog::onDeselectAll()
{
    for (int i = 0; i < ui->m_tagList->count(); ++i) {
        QListWidgetItem* item = ui->m_tagList->item(i);
        if (item->flags() & Qt::ItemIsUserCheckable) {
            item->setCheckState(Qt::Unchecked);
        }
    }
}

// ===========================================================================
// 获取结果
// ===========================================================================

QList<qint64> ReportTagDialog::selectedTagIds() const
{
    return m_selectedIds;
}

QStringList ReportTagDialog::selectedTagNames() const
{
    QStringList names;
    for (const Tag::Ptr& tag : m_allTags) {
        if (m_selectedIds.contains(tag->id())) {
            names.append(tag->name());
        }
    }
    return names;
}
