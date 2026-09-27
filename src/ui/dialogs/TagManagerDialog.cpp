/**
 * @file TagManagerDialog.cpp
 * @brief 标签管理对话框实现文件
 */

#include "TagManagerDialog.h"
#include "ui_TagManagerDialog.h"  // 由 uic 工具从 .ui 文件自动生成
#include "data/repositories/TagRepository.h"
#include "core/utils/Logger.h"
#include "core/utils/AppDimensions.h"
#include "core/utils/AppTheme.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QColor>
#include <QBrush>
#include <QLabel>

// ===========================================================================
// 构造与析构
// ===========================================================================

TagManagerDialog::TagManagerDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::TagManagerDialog)
    , m_editing(false)
{
    ui->setupUi(this);
    // 槽函数命名符合 on_<objectName>_<signalName> 约定，uic 自动连接，无需手动 connect
    loadTags();
    setWindowTitle(tr("标签管理"));
    resize(AppDimensions::Window::DialogSmallWidth, AppDimensions::Window::DialogSmallHeight);
}

TagManagerDialog::~TagManagerDialog()
{
    delete ui;
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
    ui->m_tagList->clear();

    for (const Tag::Ptr& tag : m_tags) {
        QListWidgetItem* item = new QListWidgetItem(ui->m_tagList);

        const QColor color = tag->effectiveColor();
        const QString displayText = QString(
            "<div style='display: flex; align-items: center;'>"
            "<span style='display: inline-block; width: 14px; height: 14px; "
            "border-radius: 3px; background: %1; margin-right: 8px;'></span>"
            "<span style='font-weight: bold;'>%2</span>"
            "</div>"
        ).arg(color.name()).arg(tag->name().toHtmlEscaped());

        // 使用 QLabel 作为 item widget 以支持 HTML 富文本渲染
        QLabel* label = new QLabel(displayText);
        label->setTextFormat(Qt::RichText);
        label->setStyleSheet("padding: 6px 10px; background: transparent;");
        ui->m_tagList->setItemWidget(item, label);

        item->setData(Qt::UserRole, tag->id());
        item->setSizeHint(QSize(0, 40));
    }

    if (m_tags.isEmpty()) {
        ui->m_tagList->addItem(tr("暂无标签，点击「新建」创建"));
        ui->m_tagList->item(0)->setFlags(Qt::NoItemFlags);
    }
}

// ===========================================================================
// 标签选择
// ===========================================================================

void TagManagerDialog::on_m_tagList_itemClicked(QListWidgetItem* item)
{
    if (!item || !item->data(Qt::UserRole).isValid()) {
        m_currentTag.reset();
        ui->m_editBtn->setEnabled(false);
        ui->m_deleteBtn->setEnabled(false);
        clearEditForm();
        return;
    }

    const qint64 tagId = item->data(Qt::UserRole).toLongLong();
    for (const Tag::Ptr& tag : m_tags) {
        if (tag->id() == tagId) {
            m_currentTag = tag;
            ui->m_editBtn->setEnabled(true);
            ui->m_deleteBtn->setEnabled(true);

            // 显示详情（只读）
            ui->m_nameEdit->setText(tag->name());
            ui->m_descEdit->setPlainText(tag->description());

            // 设置颜色
            const QString color = tag->color().isEmpty()
                ? tag->effectiveColor().name() : tag->color();
            const int idx = ui->m_colorCombo->findData(color);
            ui->m_colorCombo->setCurrentIndex(idx >= 0 ? idx : 0);

            setEditMode(false);
            break;
        }
    }
}

// ===========================================================================
// 新建/编辑/删除
// ===========================================================================

void TagManagerDialog::on_m_newBtn_clicked()
{
    m_currentTag = Tag::create();
    m_currentTag->setColor(Tag::presetColors().first());
    clearEditForm();
    ui->m_nameEdit->setFocus();
    setEditMode(true);
}

void TagManagerDialog::on_m_editBtn_clicked()
{
    if (!m_currentTag) return;
    setEditMode(true);
    ui->m_nameEdit->setFocus();
    ui->m_nameEdit->selectAll();
}

void TagManagerDialog::on_m_deleteBtn_clicked()
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
        ui->m_editBtn->setEnabled(false);
        ui->m_deleteBtn->setEnabled(false);
        loadTags(ui->m_searchEdit->text());
    } else {
        QMessageBox::critical(this, tr("删除失败"), tr("删除标签时发生错误"));
    }
}

// ===========================================================================
// 保存/取消
// ===========================================================================

void TagManagerDialog::on_m_saveBtn_clicked()
{
    const QString name = ui->m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("标签名称不能为空"));
        ui->m_nameEdit->setFocus();
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
    m_currentTag->setColor(ui->m_colorCombo->currentData().toString());
    m_currentTag->setDescription(ui->m_descEdit->toPlainText().trimmed());

    if (TagRepository::save(m_currentTag)) {
        QMessageBox::information(this, tr("保存成功"), tr("标签已保存"));
        setEditMode(false);
        loadTags(ui->m_searchEdit->text());

        // 选中刚保存的标签
        for (int i = 0; i < ui->m_tagList->count(); ++i) {
            QListWidgetItem* item = ui->m_tagList->item(i);
            if (item->data(Qt::UserRole).toLongLong() == m_currentTag->id()) {
                ui->m_tagList->setCurrentRow(i);
                on_m_tagList_itemClicked(item);
                break;
            }
        }
    } else {
        QMessageBox::critical(this, tr("保存失败"), tr("保存标签时发生错误"));
    }
}

void TagManagerDialog::on_m_cancelBtn_clicked()
{
    if (m_currentTag && !m_currentTag->isNew()) {
        // 恢复原始数据
        on_m_tagList_itemClicked(ui->m_tagList->currentItem());
    } else {
        clearEditForm();
        m_currentTag.reset();
    }
    setEditMode(false);
}

// ===========================================================================
// 搜索
// ===========================================================================

void TagManagerDialog::on_m_searchEdit_textChanged(const QString& text)
{
    loadTags(text);
}

// ===========================================================================
// 辅助方法
// ===========================================================================

void TagManagerDialog::clearEditForm()
{
    ui->m_nameEdit->clear();
    ui->m_descEdit->clear();
    ui->m_colorCombo->setCurrentIndex(0);
}

void TagManagerDialog::on_m_colorCombo_currentIndexChanged(int index)
{
    // 颜色下拉框选择变化时的处理
    // 如果正在编辑标签，可以在此更新颜色预览
    Q_UNUSED(index);
    // 注意：实际颜色保存是在 onSaveTag() 中通过 currentData() 获取的
    // 此槽函数可用于实时预览颜色效果
}

void TagManagerDialog::setEditMode(bool editing)
{
    m_editing = editing;
    ui->m_nameEdit->setEnabled(editing);
    ui->m_colorCombo->setEnabled(editing);
    ui->m_descEdit->setEnabled(editing);
    ui->m_saveBtn->setEnabled(editing);
    ui->m_cancelBtn->setEnabled(editing);
    ui->m_newBtn->setEnabled(!editing);
    ui->m_editBtn->setEnabled(!editing && m_currentTag);
    ui->m_deleteBtn->setEnabled(!editing && m_currentTag);
}

QString TagManagerDialog::colorSwatchHtml(const QString& color, int size)
{
    return QString(
        "<span style='display: inline-block; width: %1px; height: %1px; "
        "border-radius: 3px; background: %2; vertical-align: middle;'></span>"
    ).arg(size).arg(color);
}
