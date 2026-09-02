/**
 * @file TemplateEditorDialog.cpp
 * @brief 模板编辑器对话框实现文件
 */

#include "TemplateEditorDialog.h"
#include "editor/ReportEditor.h"
#include "data/repositories/TemplateRepository.h"
#include "core/utils/Logger.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QGroupBox>
#include <QFormLayout>

// ===========================================================================
// 构造函数
// ===========================================================================

TemplateEditorDialog::TemplateEditorDialog(QWidget* parent,
                                             const Template::Ptr& existingTemplate)
    : QDialog(parent)
    , m_nameEdit(nullptr)
    , m_categoryCombo(nullptr)
    , m_descriptionEdit(nullptr)
    , m_editor(nullptr)
    , m_buttonBox(nullptr)
    , m_template(existingTemplate)
    , m_isNewTemplate(existingTemplate.isNull() || !existingTemplate->isPersisted())
{
    setupUi();
    loadTemplate();
    setWindowTitle(m_isNewTemplate ? tr("新建模板") : tr("编辑模板"));
    resize(1000, 700);
}

// ===========================================================================
// UI 初始化
// ===========================================================================

void TemplateEditorDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // -----------------------------------------------------------------------
    // 模板元信息
    // -----------------------------------------------------------------------
    QGroupBox* metaGroup = new QGroupBox(tr("模板信息"), this);
    QFormLayout* metaLayout = new QFormLayout(metaGroup);

    m_nameEdit = new QLineEdit(metaGroup);
    m_nameEdit->setPlaceholderText(tr("请输入模板名称"));
    metaLayout->addRow(tr("模板名称:"), m_nameEdit);

    m_categoryCombo = new QComboBox(metaGroup);
    m_categoryCombo->setEditable(true);
    m_categoryCombo->addItems({tr("通用"), tr("物理"), tr("化学"), tr("生物"),
                                 tr("计算机"), tr("工程"), tr("其他")});
    metaLayout->addRow(tr("模板分类:"), m_categoryCombo);

    m_descriptionEdit = new QTextEdit(metaGroup);
    m_descriptionEdit->setPlaceholderText(tr("请输入模板描述（可选）"));
    m_descriptionEdit->setMaximumHeight(60);
    metaLayout->addRow(tr("模板描述:"), m_descriptionEdit);

    mainLayout->addWidget(metaGroup);

    // -----------------------------------------------------------------------
    // 模板块编辑区
    // -----------------------------------------------------------------------
    QLabel* hintLabel = new QLabel(tr("编辑模板结构（块内容为占位示例，创建报告时可修改）:"), this);
    hintLabel->setStyleSheet("color: #666; font-size: 12px;");
    mainLayout->addWidget(hintLabel);

    m_editor = new ReportEditor(this);
    mainLayout->addWidget(m_editor, 1);

    // -----------------------------------------------------------------------
    // 按钮
    // -----------------------------------------------------------------------
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Save)->setText(tr("保存模板"));
    m_buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("取消"));
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &TemplateEditorDialog::onSave);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(m_buttonBox);
}

// ===========================================================================
// 加载模板
// ===========================================================================

void TemplateEditorDialog::loadTemplate()
{
    if (!m_template) {
        // 新建模板：创建一个默认结构
        m_template = Template::create();
        m_template->setName(tr("新模板"));
        m_template->setCategory(tr("通用"));

        // 添加默认块
        Report::Ptr dummyReport = Report::create();
        ContentBlock h1(BlockType::Heading1);
        h1.data["text"] = "实验名称";
        dummyReport->appendBlock(h1);

        ContentBlock p(BlockType::Paragraph);
        p.data["text"] = "";
        dummyReport->appendBlock(p);

        ContentBlock h2(BlockType::Heading1);
        h2.data["text"] = "实验目的";
        dummyReport->appendBlock(h2);

        m_editor->loadReport(dummyReport);
    } else {
        // 编辑现有模板
        m_nameEdit->setText(m_template->name());
        m_descriptionEdit->setPlainText(m_template->description());

        // 设置分类
        const int idx = m_categoryCombo->findText(m_template->category());
        if (idx >= 0) m_categoryCombo->setCurrentIndex(idx);
        else m_categoryCombo->setEditText(m_template->category());

        // 加载模板块到编辑器
        Report::Ptr dummyReport = Report::create();
        for (const ContentBlock& block : m_template->blocks()) {
            dummyReport->appendBlock(block);
        }
        m_editor->loadReport(dummyReport);
    }
}

// ===========================================================================
// 保存
// ===========================================================================

void TemplateEditorDialog::onSave()
{
    if (!validateInput()) return;

    // 收集模板块
    const QList<ContentBlock> blocks = collectBlocks();

    // 更新模板数据
    m_template->setName(m_nameEdit->text().trimmed());
    m_template->setCategory(m_categoryCombo->currentText().trimmed());
    m_template->setDescription(m_descriptionEdit->toPlainText().trimmed());
    m_template->setBlocks(blocks);

    // 保存到数据库
    bool success = false;
    if (m_isNewTemplate) {
        success = TemplateRepository::insert(m_template);
    } else {
        success = TemplateRepository::update(m_template);
    }

    if (success) {
        LOG_INFO(QString("模板已保存: %1").arg(m_template->name()));
        accept();
    } else {
        QMessageBox::critical(this, tr("保存失败"), tr("保存模板时发生错误。"));
    }
}

bool TemplateEditorDialog::validateInput()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("输入错误"), tr("模板名称不能为空"));
        m_nameEdit->setFocus();
        return false;
    }
    return true;
}

// ===========================================================================
// 数据收集
// ===========================================================================

QList<ContentBlock> TemplateEditorDialog::collectBlocks() const
{
    // 从编辑器获取报告，然后提取块
    Report::Ptr report = m_editor->saveToReport();
    return report->blocks();
}

Template::Ptr TemplateEditorDialog::templateData() const
{
    return m_template;
}
