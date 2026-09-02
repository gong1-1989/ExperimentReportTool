/**
 * @file ReportTagDialog.cpp
 * @brief 报告标签选择对话框实现文件
 */

#include "ReportTagDialog.h"
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
    , m_searchEdit(nullptr)
    , m_tagList(nullptr)
    , m_newTagBtn(nullptr)
    , m_selectAllBtn(nullptr)
    , m_deselectAllBtn(nullptr)
    , m_okBtn(nullptr)
    , m_cancelBtn(nullptr)
    , m_selectedLabel(nullptr)
    , m_reportId(reportId)
{
    setupUi();
    loadTags();
    loadSelectedTags();
    setWindowTitle(tr("选择标签"));
    resize(500, 500);
}

ReportTagDialog::~ReportTagDialog()
{
}

// ===========================================================================
// UI 初始化
// ===========================================================================

void ReportTagDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // 搜索
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("搜索标签..."));
    m_searchEdit->setClearButtonEnabled(true);
    mainLayout->addWidget(m_searchEdit);

    // 标签列表
    m_tagList = new QListWidget(this);
    m_tagList->setSelectionMode(QAbstractItemView::NoSelection);
    m_tagList->setStyleSheet(
        "QListWidget { border: 1px solid #ddd; border-radius: 4px; }"
        "QListWidget::item { padding: 8px; border-bottom: 1px solid #eee; }"
        "QListWidget::item:hover { background: #f5f5f5; }");
    mainLayout->addWidget(m_tagList, 1);

    // 已选标签
    m_selectedLabel = new QLabel(tr("已选: 0 个标签"), this);
    m_selectedLabel->setStyleSheet("color: #666; font-size: 12px; padding: 4px 0;");
    m_selectedLabel->setWordWrap(true);
    mainLayout->addWidget(m_selectedLabel);

    // 按钮行
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_newTagBtn = new QPushButton(tr("+ 新建标签"), this);
    m_selectAllBtn = new QPushButton(tr("全选"), this);
    m_deselectAllBtn = new QPushButton(tr("全不选"), this);
    btnLayout->addWidget(m_newTagBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_selectAllBtn);
    btnLayout->addWidget(m_deselectAllBtn);
    mainLayout->addLayout(btnLayout);

    // 确定/取消
    QHBoxLayout* okLayout = new QHBoxLayout();
    okLayout->addStretch();
    m_cancelBtn = new QPushButton(tr("取消"), this);
    m_okBtn = new QPushButton(tr("确定"), this);
    m_okBtn->setStyleSheet(
        "QPushButton { background: #4A90D9; color: white; padding: 6px 24px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #357ABD; }");
    m_okBtn->setDefault(true);
    okLayout->addWidget(m_cancelBtn);
    okLayout->addWidget(m_okBtn);
    mainLayout->addLayout(okLayout);

    // 连接信号
    connect(m_newTagBtn, &QPushButton::clicked, this, &ReportTagDialog::onNewTag);
    connect(m_selectAllBtn, &QPushButton::clicked, this, &ReportTagDialog::onSelectAll);
    connect(m_deselectAllBtn, &QPushButton::clicked, this, &ReportTagDialog::onDeselectAll);
    connect(m_okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &ReportTagDialog::onSearchTextChanged);
    connect(m_tagList, &QListWidget::itemChanged,
            this, &ReportTagDialog::onItemChanged);
}

// ===========================================================================
// 加载标签
// ===========================================================================

void ReportTagDialog::loadTags()
{
    m_tagList->clear();
    m_allTags = TagRepository::findAll();

    for (const Tag::Ptr& tag : m_allTags) {
        QListWidgetItem* item = new QListWidgetItem(m_tagList);

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
        QListWidgetItem* item = new QListWidgetItem(tr("暂无标签，点击「新建标签」创建"), m_tagList);
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
        for (int i = 0; i < m_tagList->count(); ++i) {
            QListWidgetItem* item = m_tagList->item(i);
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
        m_selectedLabel->setText(tr("已选: 0 个标签"));
    } else {
        m_selectedLabel->setText(tr("已选 %1 个: %2")
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
    m_tagList->clear();
    m_allTags = text.isEmpty()
        ? TagRepository::findAll()
        : TagRepository::search(text);

    for (const Tag::Ptr& tag : m_allTags) {
        QListWidgetItem* item = new QListWidgetItem(m_tagList);
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
    for (int i = 0; i < m_tagList->count(); ++i) {
        QListWidgetItem* item = m_tagList->item(i);
        if (item->flags() & Qt::ItemIsUserCheckable) {
            item->setCheckState(Qt::Checked);
        }
    }
}

void ReportTagDialog::onDeselectAll()
{
    for (int i = 0; i < m_tagList->count(); ++i) {
        QListWidgetItem* item = m_tagList->item(i);
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
