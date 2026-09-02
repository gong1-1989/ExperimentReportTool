/**
 * @file TagManagerDialog.cpp
 * @brief 标签管理对话框实现文件
 */

#include "TagManagerDialog.h"
#include "data/repositories/TagRepository.h"
#include "core/utils/Logger.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QColor>
#include <QBrush>

// ===========================================================================
// 构造与析构
// ===========================================================================

TagManagerDialog::TagManagerDialog(QWidget* parent)
    : QDialog(parent)
    , m_searchEdit(nullptr)
    , m_tagList(nullptr)
    , m_newBtn(nullptr)
    , m_editBtn(nullptr)
    , m_deleteBtn(nullptr)
    , m_editGroup(nullptr)
    , m_nameEdit(nullptr)
    , m_colorCombo(nullptr)
    , m_descEdit(nullptr)
    , m_saveBtn(nullptr)
    , m_cancelBtn(nullptr)
    , m_usageLabel(nullptr)
    , m_editing(false)
{
    setupUi();
    loadTags();
    setWindowTitle(tr("标签管理"));
    resize(700, 500);
}

TagManagerDialog::~TagManagerDialog()
{
}

// ===========================================================================
// UI 初始化
// ===========================================================================

void TagManagerDialog::setupUi()
{
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    // -----------------------------------------------------------------------
    // 左侧：标签列表
    // -----------------------------------------------------------------------
    QWidget* leftPanel = new QWidget(this);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(6);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("搜索标签..."));
    m_searchEdit->setClearButtonEnabled(true);
    leftLayout->addWidget(m_searchEdit);

    m_tagList = new QListWidget(this);
    m_tagList->setStyleSheet(
        "QListWidget { border: 1px solid #ddd; border-radius: 4px; }"
        "QListWidget::item { padding: 8px; border-bottom: 1px solid #eee; }"
        "QListWidget::item:selected { background: #e8f0fe; }"
        "QListWidget::item:hover { background: #f5f5f5; }");
    leftLayout->addWidget(m_tagList, 1);

    // 按钮
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_newBtn = new QPushButton(tr("新建"), this);
    m_editBtn = new QPushButton(tr("编辑"), this);
    m_deleteBtn = new QPushButton(tr("删除"), this);
    m_editBtn->setEnabled(false);
    m_deleteBtn->setEnabled(false);
    btnLayout->addWidget(m_newBtn);
    btnLayout->addWidget(m_editBtn);
    btnLayout->addWidget(m_deleteBtn);
    leftLayout->addLayout(btnLayout);

    mainLayout->addWidget(leftPanel, 1);

    // -----------------------------------------------------------------------
    // 右侧：编辑表单
    // -----------------------------------------------------------------------
    m_editGroup = new QGroupBox(tr("标签详情"), this);
    QVBoxLayout* editLayout = new QVBoxLayout(m_editGroup);
    editLayout->setSpacing(8);

    editLayout->addWidget(new QLabel(tr("名称:"), this));
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("输入标签名称"));
    editLayout->addWidget(m_nameEdit);

    editLayout->addWidget(new QLabel(tr("颜色:"), this));
    m_colorCombo = new QComboBox(this);
    // 添加预设颜色
    for (const QString& color : Tag::presetColors()) {
        m_colorCombo->addItem(colorSwatchHtml(color) + "  " + color, color);
    }
    m_colorCombo->setEditable(false);
    editLayout->addWidget(m_colorCombo);

    editLayout->addWidget(new QLabel(tr("描述:"), this));
    m_descEdit = new QTextEdit(this);
    m_descEdit->setPlaceholderText(tr("标签描述（可选）"));
    m_descEdit->setMaximumHeight(100);
    editLayout->addWidget(m_descEdit);

    m_usageLabel = new QLabel(tr("使用次数: -"), this);
    m_usageLabel->setStyleSheet("color: #666; font-size: 12px;");
    editLayout->addWidget(m_usageLabel);

    editLayout->addStretch();

    // 保存/取消按钮
    QHBoxLayout* saveLayout = new QHBoxLayout();
    saveLayout->addStretch();
    m_cancelBtn = new QPushButton(tr("取消"), this);
    m_saveBtn = new QPushButton(tr("保存"), this);
    m_saveBtn->setStyleSheet(
        "QPushButton { background: #4A90D9; color: white; padding: 6px 20px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #357ABD; }");
    saveLayout->addWidget(m_cancelBtn);
    saveLayout->addWidget(m_saveBtn);
    editLayout->addLayout(saveLayout);

    mainLayout->addWidget(m_editGroup, 1);

    // 初始状态：禁用编辑表单
    setEditMode(false);

    // -----------------------------------------------------------------------
    // 连接信号
    // -----------------------------------------------------------------------
    connect(m_tagList, &QListWidget::itemClicked,
            this, &TagManagerDialog::onTagSelected);
    connect(m_newBtn, &QPushButton::clicked, this, &TagManagerDialog::onNewTag);
    connect(m_editBtn, &QPushButton::clicked, this, &TagManagerDialog::onEditTag);
    connect(m_deleteBtn, &QPushButton::clicked, this, &TagManagerDialog::onDeleteTag);
    connect(m_saveBtn, &QPushButton::clicked, this, &TagManagerDialog::onSaveTag);
    connect(m_cancelBtn, &QPushButton::clicked, this, &TagManagerDialog::onCancelEdit);
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &TagManagerDialog::onSearchTextChanged);
}

// ===========================================================================
// 加载标签
// ===========================================================================

void TagManagerDialog::loadTags(const QString& filter)
{
    if (filter.isEmpty()) {
        m_tags = TagRepository::findAll();
    } else {
        m_tags = TagRepository::search(filter);
    }
    updateTagList();
}

void TagManagerDialog::updateTagList()
{
    m_tagList->clear();

    for (const Tag::Ptr& tag : m_tags) {
        QListWidgetItem* item = new QListWidgetItem(m_tagList);

        const QColor color = tag->effectiveColor();
        const QString displayText = QString(
            "<div style='display: flex; align-items: center;'>"
            "<span style='display: inline-block; width: 14px; height: 14px; "
            "border-radius: 3px; background: %1; margin-right: 8px;'></span>"
            "<span style='font-weight: bold;'>%2</span>"
            "<span style='color: #999; font-size: 11px; margin-left: auto;'>%3 篇</span>"
            "</div>"
        ).arg(color.name()).arg(tag->name().toHtmlEscaped()).arg(tag->usageCount());

        item->setText(displayText);
        item->setData(Qt::UserRole, tag->id());
        item->setSizeHint(QSize(0, 40));
    }

    if (m_tags.isEmpty()) {
        m_tagList->addItem(tr("暂无标签，点击「新建」创建"));
        m_tagList->item(0)->setFlags(Qt::NoItemFlags);
    }
}

// ===========================================================================
// 标签选择
// ===========================================================================

void TagManagerDialog::onTagSelected(QListWidgetItem* item)
{
    if (!item || !item->data(Qt::UserRole).isValid()) {
        m_currentTag.reset();
        m_editBtn->setEnabled(false);
        m_deleteBtn->setEnabled(false);
        clearEditForm();
        return;
    }

    const qint64 tagId = item->data(Qt::UserRole).toLongLong();
    for (const Tag::Ptr& tag : m_tags) {
        if (tag->id() == tagId) {
            m_currentTag = tag;
            m_editBtn->setEnabled(true);
            m_deleteBtn->setEnabled(true);

            // 显示详情（只读）
            m_nameEdit->setText(tag->name());
            m_descEdit->setPlainText(tag->description());
            m_usageLabel->setText(tr("使用次数: %1").arg(tag->usageCount()));

            // 设置颜色
            const QString color = tag->color().isEmpty()
                ? tag->effectiveColor().name() : tag->color();
            const int idx = m_colorCombo->findData(color);
            m_colorCombo->setCurrentIndex(idx >= 0 ? idx : 0);

            setEditMode(false);
            break;
        }
    }
}

// ===========================================================================
// 新建/编辑/删除
// ===========================================================================

void TagManagerDialog::onNewTag()
{
    m_currentTag = Tag::create();
    m_currentTag->setColor(Tag::presetColors().first());
    clearEditForm();
    m_nameEdit->setFocus();
    setEditMode(true);
}

void TagManagerDialog::onEditTag()
{
    if (!m_currentTag) return;
    setEditMode(true);
    m_nameEdit->setFocus();
    m_nameEdit->selectAll();
}

void TagManagerDialog::onDeleteTag()
{
    if (!m_currentTag) return;

    const auto result = QMessageBox::question(
        this, tr("确认删除"),
        tr("确定要删除标签「%1」吗？\n\n"
           "该标签将从所有关联的报告中移除。\n此操作不可撤销。")
            .arg(m_currentTag->name()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (result != QMessageBox::Yes) return;

    if (TagRepository::remove(m_currentTag->id())) {
        QMessageBox::information(this, tr("删除成功"), tr("标签已删除"));
        m_currentTag.reset();
        clearEditForm();
        setEditMode(false);
        m_editBtn->setEnabled(false);
        m_deleteBtn->setEnabled(false);
        loadTags(m_searchEdit->text());
    } else {
        QMessageBox::critical(this, tr("删除失败"), tr("删除标签时发生错误"));
    }
}

// ===========================================================================
// 保存/取消
// ===========================================================================

void TagManagerDialog::onSaveTag()
{
    const QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("标签名称不能为空"));
        m_nameEdit->setFocus();
        return;
    }

    // 检查重名
    if (TagRepository::exists(name, m_currentTag ? m_currentTag->id() : -1)) {
        QMessageBox::warning(this, tr("提示"), tr("标签名称已存在"));
        return;
    }

    if (!m_currentTag) {
        m_currentTag = Tag::create();
    }

    m_currentTag->setName(name);
    m_currentTag->setColor(m_colorCombo->currentData().toString());
    m_currentTag->setDescription(m_descEdit->toPlainText().trimmed());

    if (TagRepository::save(m_currentTag)) {
        QMessageBox::information(this, tr("保存成功"), tr("标签已保存"));
        setEditMode(false);
        loadTags(m_searchEdit->text());

        // 选中刚保存的标签
        for (int i = 0; i < m_tagList->count(); ++i) {
            QListWidgetItem* item = m_tagList->item(i);
            if (item->data(Qt::UserRole).toLongLong() == m_currentTag->id()) {
                m_tagList->setCurrentRow(i);
                onTagSelected(item);
                break;
            }
        }
    } else {
        QMessageBox::critical(this, tr("保存失败"), tr("保存标签时发生错误"));
    }
}

void TagManagerDialog::onCancelEdit()
{
    if (m_currentTag && !m_currentTag->isNew()) {
        // 恢复原始数据
        onTagSelected(m_tagList->currentItem());
    } else {
        clearEditForm();
        m_currentTag.reset();
    }
    setEditMode(false);
}

// ===========================================================================
// 搜索
// ===========================================================================

void TagManagerDialog::onSearchTextChanged(const QString& text)
{
    loadTags(text);
}

// ===========================================================================
// 辅助方法
// ===========================================================================

void TagManagerDialog::clearEditForm()
{
    m_nameEdit->clear();
    m_descEdit->clear();
    m_colorCombo->setCurrentIndex(0);
    m_usageLabel->setText(tr("使用次数: -"));
}

void TagManagerDialog::setEditMode(bool editing)
{
    m_editing = editing;
    m_nameEdit->setEnabled(editing);
    m_colorCombo->setEnabled(editing);
    m_descEdit->setEnabled(editing);
    m_saveBtn->setEnabled(editing);
    m_cancelBtn->setEnabled(editing);
    m_newBtn->setEnabled(!editing);
    m_editBtn->setEnabled(!editing && m_currentTag);
    m_deleteBtn->setEnabled(!editing && m_currentTag);
}

QString TagManagerDialog::colorSwatchHtml(const QString& color, int size)
{
    return QString(
        "<span style='display: inline-block; width: %1px; height: %1px; "
        "border-radius: 3px; background: %2; vertical-align: middle;'></span>"
    ).arg(size).arg(color);
}
