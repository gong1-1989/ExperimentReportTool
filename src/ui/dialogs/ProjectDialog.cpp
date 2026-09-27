/**
 * @file ProjectDialog.cpp
 * @brief 项目编辑对话框实现文件
 *
 * 本文件实现 ProjectDialog 类的所有功能。
 * UI 界面通过 Qt Designer 设计的 ProjectDialog.ui 文件加载，
 * 由 CMake 的 AUTOUIC 功能自动生成 ui_ProjectDialog.h 头文件。
 */

#include "ProjectDialog.h"
#include "ui_ProjectDialog.h"  // 由 uic 工具从 .ui 文件自动生成
#include "data/repositories/ProjectRepository.h"
#include "core/utils/UserSession.h"

#include <QMessageBox>
#include <QPushButton>

// ===========================================================================
// 构造函数
// ===========================================================================

/**
 * @brief 构造函数
 *
 * 初始化 UI 界面（从 .ui 文件加载），设置默认值，连接信号槽。
 *
 * @param parent 父窗口指针，用于窗口父子关系管理
 */
ProjectDialog::ProjectDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ProjectDialog)  // 创建 UI 界面对象
    , m_parentProjectId(-1)       // 默认根项目（无父项目）
    , m_editingProjectId(-1)      // 默认新建模式（-1 表示不是编辑已有项目）
{
    // 从 .ui 文件加载界面布局和控件
    ui->setupUi(this);

    // 设置对话框最小尺寸，确保所有控件都能正常显示
    setMinimumSize(450, 380);

    // 设置状态下拉框的 itemData（用于存储 ProjectStatus 枚举值）
    // 注意：.ui 文件中只设置了显示文本，这里需要设置对应的数据值
    ui->m_statusCombo->setItemData(0, static_cast<int>(ProjectStatus::Active));
    ui->m_statusCombo->setItemData(1, static_cast<int>(ProjectStatus::Completed));
    ui->m_statusCombo->setItemData(2, static_cast<int>(ProjectStatus::Archived));

    // 设置按钮文本为中文
    ui->m_buttonBox->button(QDialogButtonBox::Ok)->setText(tr("确定"));
    ui->m_buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("取消"));

    // 负责人自动为当前用户显示名称，不允许修改
    const QString currentUserName = UserSession::instance().displayName();
    if (!currentUserName.isEmpty()) {
        ui->m_ownerEdit->setText(currentUserName);
    }
    ui->m_ownerEdit->setReadOnly(true);

    // 槽函数通过 uic 自动连接（on_m_buttonBox_accepted）

    // 设置初始焦点在名称输入框，方便用户直接输入
    ui->m_nameEdit->setFocus();
}

// ===========================================================================
// 析构函数
// ===========================================================================

/**
 * @brief 析构函数
 *
 * 释放 UI 界面对象资源。
 */
ProjectDialog::~ProjectDialog()
{
    delete ui;
}

// ===========================================================================
// 数据获取与设置
// ===========================================================================

/**
 * @brief 获取对话框中的项目数据
 *
 * 从所有输入控件中读取用户输入，构造一个新的 Project 对象。
 * 如果是编辑模式，会设置项目 ID；如果指定了父项目，会设置父项目 ID。
 *
 * @return Project::Ptr 项目对象智能指针，包含所有用户输入的字段
 */
Project::Ptr ProjectDialog::projectData() const
{
    // 创建新的项目对象
    Project::Ptr project = Project::create();

    // 从输入控件读取数据并设置到项目对象
    project->setName(ui->m_nameEdit->text().trimmed());           // 项目名称（去除首尾空格）
    project->setType(ui->m_typeEdit->text().trimmed());           // 项目类型
    project->setOwner(UserSession::instance().displayName());      // 负责人固定为当前用户
    project->setDescription(ui->m_descriptionEdit->toPlainText().trimmed());  // 项目描述
    // 创建者为当前登录用户
    project->setCreatedBy(UserSession::instance().userId());

    // 从下拉框读取状态（itemData 存储了 ProjectStatus 枚举的整数值）
    project->setStatus(static_cast<ProjectStatus>(ui->m_statusCombo->currentData().toInt()));

    // 如果指定了父项目 ID，设置父项目（用于新建子项目）
    if (m_parentProjectId > 0) {
        project->setParentId(m_parentProjectId);
    }

    // 如果是编辑模式，设置项目 ID（用于更新已有项目而不是新建）
    if (m_editingProjectId > 0) {
        project->setId(m_editingProjectId);
    }

    return project;
}

/**
 * @brief 设置对话框中的项目数据（用于编辑模式，回填已有数据）
 *
 * 将已有项目的各个字段回填到对应的输入控件中，
 * 并设置编辑模式标记，保存项目 ID 用于后续更新。
 *
 * @param project 要编辑的项目对象，如果为空则不做任何操作
 */
void ProjectDialog::setProjectData(const Project::Ptr& project)
{
    // 空指针检查，避免访问空对象
    if (!project) return;

    // 保存正在编辑的项目 ID（用于后续更新操作）
    m_editingProjectId = project->id();

    // 将项目数据回填到各个输入控件
    ui->m_nameEdit->setText(project->name());                    // 项目名称
    ui->m_typeEdit->setText(project->type());                    // 项目类型
    // 负责人固定为当前用户，不允许修改
    ui->m_ownerEdit->setText(UserSession::instance().displayName());
    ui->m_ownerEdit->setReadOnly(true);
    ui->m_descriptionEdit->setPlainText(project->description()); // 项目描述

    // 设置状态下拉框的当前选中项
    // 通过 itemData 查找对应的索引（itemData 存储了 ProjectStatus 枚举值）
    const int statusIndex = ui->m_statusCombo->findData(static_cast<int>(project->status()));
    if (statusIndex >= 0) {
        ui->m_statusCombo->setCurrentIndex(statusIndex);
    }
}

// ===========================================================================
// 槽函数
// ===========================================================================

/**
 * @brief 确定按钮点击槽函数
 *
 * 调用 validateInput() 验证用户输入，
 * 如果验证通过则调用 accept() 关闭对话框并返回 Accepted 结果码，
 * 如果验证失败则保持对话框打开，让用户修改输入。
 */
void ProjectDialog::on_m_buttonBox_accepted()
{
    if (validateInput()) {
        accept();  // 验证通过，关闭对话框
    }
    // 验证失败时不关闭对话框，用户可以修改后重新点击确定
}

// ===========================================================================
// 输入验证
// ===========================================================================

/**
 * @brief 验证用户输入是否合法
 *
 * 依次检查以下条件：
 * 1. 项目名称不能为空
 * 2. 项目名称长度不能超过 100 个字符
 * 3. 项目名称不能与已有项目重复（编辑时排除自身）
 *
 * 任何一项验证失败都会显示对应的错误提示对话框，
 * 并将焦点设置到名称输入框，方便用户修改。
 *
 * @return bool 所有验证通过返回 true，否则返回 false
 */
bool ProjectDialog::validateInput()
{
    // 获取项目名称并去除首尾空格
    const QString name = ui->m_nameEdit->text().trimmed();

    // 验证1：名称不能为空
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("输入错误"), tr("项目名称不能为空"));
        ui->m_nameEdit->setFocus();  // 将焦点设置到名称输入框
        return false;
    }

    // 验证2：名称长度限制（最多100个字符）
    if (name.length() > 100) {
        QMessageBox::warning(this, tr("输入错误"), tr("项目名称不能超过 100 个字符"));
        ui->m_nameEdit->setFocus();
        return false;
    }

    // 验证3：名称重复检查
    // 编辑模式下排除正在编辑的项目自身，避免误报
    if (ProjectRepository::existsByName(name, m_editingProjectId)) {
        QMessageBox::warning(this, tr("输入错误"), tr("已存在同名项目，请使用其他名称"));
        ui->m_nameEdit->setFocus();
        return false;
    }

    // 所有验证通过
    return true;
}
