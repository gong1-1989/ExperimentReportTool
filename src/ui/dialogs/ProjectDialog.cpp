/**
 * @file ProjectDialog.cpp
 * @brief 项目编辑对话框实现文件
 */

#include "ProjectDialog.h"
#include "data/repositories/ProjectRepository.h"

#include <QMessageBox>
#include <QPushButton>

// ===========================================================================
// 构造函数
// ===========================================================================

ProjectDialog::ProjectDialog(QWidget* parent)
    : QDialog(parent)
    , m_parentProjectId(-1)
    , m_editingProjectId(-1)
{
    setupUi();
    setMinimumSize(450, 380);
}

// ===========================================================================
// UI 初始化
// ===========================================================================

void ProjectDialog::setupUi()
{
    setWindowTitle(tr("项目信息"));

    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 表单布局
    QFormLayout* formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setSpacing(12);
    formLayout->setContentsMargins(10, 10, 10, 10);

    // 项目名称
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("请输入项目名称"));
    m_nameEdit->setClearButtonEnabled(true);
    formLayout->addRow(tr("项目名称:"), m_nameEdit);

    // 项目类型
    m_typeEdit = new QLineEdit(this);
    m_typeEdit->setPlaceholderText(tr("如：物理、化学、生物、通用"));
    m_typeEdit->setClearButtonEnabled(true);
    formLayout->addRow(tr("项目类型:"), m_typeEdit);

    // 负责人
    m_ownerEdit = new QLineEdit(this);
    m_ownerEdit->setPlaceholderText(tr("请输入负责人姓名"));
    m_ownerEdit->setClearButtonEnabled(true);
    formLayout->addRow(tr("负责人:"), m_ownerEdit);

    // 项目状态
    m_statusCombo = new QComboBox(this);
    m_statusCombo->addItem(tr("进行中"), static_cast<int>(ProjectStatus::Active));
    m_statusCombo->addItem(tr("已完成"), static_cast<int>(ProjectStatus::Completed));
    m_statusCombo->addItem(tr("已归档"), static_cast<int>(ProjectStatus::Archived));
    formLayout->addRow(tr("项目状态:"), m_statusCombo);

    // 项目描述
    m_descriptionEdit = new QTextEdit(this);
    m_descriptionEdit->setPlaceholderText(tr("请输入项目描述（可选）"));
    m_descriptionEdit->setMaximumHeight(120);
    formLayout->addRow(tr("项目描述:"), m_descriptionEdit);

    mainLayout->addLayout(formLayout);

    // 按钮组
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setText(tr("确定"));
    m_buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("取消"));
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &ProjectDialog::onAccept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(m_buttonBox);

    // 默认焦点在名称输入框
    m_nameEdit->setFocus();
}

// ===========================================================================
// 数据获取与设置
// ===========================================================================

Project::Ptr ProjectDialog::projectData() const
{
    Project::Ptr project = Project::create();

    project->setName(m_nameEdit->text().trimmed());
    project->setType(m_typeEdit->text().trimmed());
    project->setOwner(m_ownerEdit->text().trimmed());
    project->setDescription(m_descriptionEdit->toPlainText().trimmed());
    project->setStatus(static_cast<ProjectStatus>(m_statusCombo->currentData().toInt()));

    if (m_parentProjectId > 0) {
        project->setParentId(m_parentProjectId);
    }

    if (m_editingProjectId > 0) {
        project->setId(m_editingProjectId);
    }

    return project;
}

void ProjectDialog::setProjectData(const Project::Ptr& project)
{
    if (!project) return;

    m_editingProjectId = project->id();
    m_nameEdit->setText(project->name());
    m_typeEdit->setText(project->type());
    m_ownerEdit->setText(project->owner());
    m_descriptionEdit->setPlainText(project->description());

    // 设置状态下拉框
    const int statusIndex = m_statusCombo->findData(static_cast<int>(project->status()));
    if (statusIndex >= 0) {
        m_statusCombo->setCurrentIndex(statusIndex);
    }
}

// ===========================================================================
// 槽函数
// ===========================================================================

void ProjectDialog::onAccept()
{
    if (validateInput()) {
        accept();
    }
}

// ===========================================================================
// 输入验证
// ===========================================================================

bool ProjectDialog::validateInput()
{
    const QString name = m_nameEdit->text().trimmed();

    // 名称不能为空
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("输入错误"), tr("项目名称不能为空"));
        m_nameEdit->setFocus();
        return false;
    }

    // 名称长度限制
    if (name.length() > 100) {
        QMessageBox::warning(this, tr("输入错误"), tr("项目名称不能超过 100 个字符"));
        m_nameEdit->setFocus();
        return false;
    }

    // 名称重复检查（编辑时排除自身）
    if (ProjectRepository::existsByName(name, m_editingProjectId)) {
        QMessageBox::warning(this, tr("输入错误"), tr("已存在同名项目，请使用其他名称"));
        m_nameEdit->setFocus();
        return false;
    }

    return true;
}
