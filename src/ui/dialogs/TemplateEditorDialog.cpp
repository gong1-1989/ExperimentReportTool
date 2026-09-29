/**
 * @file TemplateEditorDialog.cpp
 * @brief 模板编辑器对话框实现文件
 *
 * 本文件实现 TemplateEditorDialog 类的所有功能。
 * UI 界面通过 Qt Designer 设计的 TemplateEditorDialog.ui 文件加载，
 * 由 CMake 的 AUTOUIC 功能自动生成 ui_TemplateEditorDialog.h 头文件。
 */

#include "TemplateEditorDialog.h"
#include "ui_TemplateEditorDialog.h"  // 由 uic 工具从 .ui 文件自动生成
#include "editor/ReportEditor.h"
#include "service/TemplateService.h"
#include "data/repositories/TemplateRepository.h"
#include "core/utils/Logger.h"
#include "core/utils/AppDimensions.h"

#include <QMessageBox>
#include "ui/UiHelper.h"
#include <QInputDialog>
#include <QVBoxLayout>

// ===========================================================================
// 构造函数
// ===========================================================================

/**
 * @brief 构造函数
 *
 * 初始化 UI 界面（从 .ui 文件加载），加载模板数据，设置窗口标题。
 *
 * @param parent 父窗口指针
 * @param existingTemplate 已有的模板对象（为空则新建模板）
 */
TemplateEditorDialog::TemplateEditorDialog(QWidget* parent,
                                             const Template::Ptr& existingTemplate)
    : BaseDialog(parent)
    , ui(new Ui::TemplateEditorDialog)  // 创建 UI 界面对象
    , m_editor(nullptr)                  // 报告编辑器组件（动态创建）
    , m_template(existingTemplate)
    , m_isNewTemplate(existingTemplate.isNull() || !existingTemplate->isPersisted())
{
    // 从 .ui 文件加载界面布局和控件
    ui->setupUi(this);

    // 动态创建 ReportEditor 组件（因为 .ui 文件中无法直接嵌入自定义组件）
    m_editor = new ReportEditor(this);
    // 将 ReportEditor 添加到 .ui 文件中的布局（需要在 .ui 文件中预留一个容器）
    // 这里我们直接替换 central widget 的布局
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (mainLayout) {
        // 在按钮组之前插入编辑器
        int buttonIndex = mainLayout->indexOf(ui->m_buttonBox);
        if (buttonIndex >= 0) {
            mainLayout->insertWidget(buttonIndex - 1, m_editor, 1);
        } else {
            mainLayout->addWidget(m_editor, 1);
        }
    }

    // 设置按钮文本
    ui->m_buttonBox->button(QDialogButtonBox::Save)->setText(tr("保存模板"));
    ui->m_buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("取消"));

    // 槽函数通过 uic 自动连接（on_m_buttonBox_accepted / on_m_buttonBox_rejected）

    // 槽函数通过 uic 自动连接（on_m_buttonBox_accepted）

    // 加载模板数据
    loadTemplate();

    // 设置窗口标题和大小
    setWindowTitle(m_isNewTemplate ? tr("新建模板") : tr("编辑模板"));
    resize(AppDimensions::Window::DialogLargeWidth, AppDimensions::Window::DialogLargeHeight);
}

// ===========================================================================
// 析构函数
// ===========================================================================

/**
 * @brief 析构函数
 *
 * 释放 UI 界面对象资源。
 */
TemplateEditorDialog::~TemplateEditorDialog()
{
    delete ui;
}

// ===========================================================================
// 加载模板
// ===========================================================================

/**
 * @brief 加载模板数据到界面控件
 *
 * 如果是新建模板，创建一个包含默认块结构的模板；
 * 如果是编辑现有模板，将模板数据回填到各个输入控件。
 */
void TemplateEditorDialog::loadTemplate()
{
    if (!m_template) {
        // 新建模板：创建一个默认结构
        m_template = Template::create();
        m_template->setName(tr("新模板"));
        m_template->setCategory(tr("通用"));

        // 默认模板结构（连续文档 HTML）
        Report::Ptr dummyReport = Report::create();
        dummyReport->setDocument(
            QString("<p style=\"font-size:20pt;\">实验名称</p>\n"
                    "<p></p>\n"
                    "<p style=\"font-size:20pt;\">实验目的</p>"));

        m_editor->loadReport(dummyReport);
    } else {
        // 编辑现有模板：回填数据到输入控件
        ui->m_nameEdit->setText(m_template->name());
        ui->m_descriptionEdit->setPlainText(m_template->description());

        // 设置分类下拉框
        const int idx = ui->m_categoryCombo->findText(m_template->category());
        if (idx >= 0) {
            ui->m_categoryCombo->setCurrentIndex(idx);
        } else {
            ui->m_categoryCombo->setEditText(m_template->category());
        }

        // 加载模板文档到编辑器（含内嵌对象锚点）
        Report::Ptr dummyReport = Report::create();
        dummyReport->setDocument(m_template->document());
        dummyReport->setObjects(m_template->objects());
        m_editor->loadReport(dummyReport);
    }
}

// ===========================================================================
// 保存
// ===========================================================================

/**
 * @brief 保存模板按钮点击槽函数
 *
 * 验证用户输入，收集模板块，更新模板数据，保存到数据库。
 * 保存成功后关闭对话框，失败则显示错误提示。
 */
void TemplateEditorDialog::on_m_buttonBox_accepted()
{
    // 验证输入
    if (!validateInput()) return;

    // 收集模板文档（从编辑器获取）
    const QString doc = collectDocument();

    // 从输入控件读取数据，更新模板对象
    m_template->setName(ui->m_nameEdit->text().trimmed());
    m_template->setCategory(ui->m_categoryCombo->currentText().trimmed());
    m_template->setDescription(ui->m_descriptionEdit->toPlainText().trimmed());
    m_template->setDocument(doc);

    // 保存到数据库
    bool success = false;
    if (m_isNewTemplate) {
        success = TemplateService::save(m_template);
    } else {
        success = TemplateService::save(m_template);
    }

    if (success) {
        LOG_INFO(QString("模板已保存: %1").arg(m_template->name()));
        accept();  // 保存成功，关闭对话框
    } else {
        UiHelper::error(this, tr("保存失败"), tr("保存模板时发生错误。"));
    }
}

void TemplateEditorDialog::on_m_buttonBox_rejected()
{
    reject();  // 取消，关闭对话框
}

/**
 * @brief 验证用户输入
 *
 * 检查模板名称是否为空。
 *
 * @return bool 验证通过返回 true，否则显示错误提示并返回 false
 */
bool TemplateEditorDialog::validateInput()
{
    if (ui->m_nameEdit->text().trimmed().isEmpty()) {
        UiHelper::warning(this, tr("输入错误"), tr("模板名称不能为空"));
        ui->m_nameEdit->setFocus();
        return false;
    }
    return true;
}

// ===========================================================================
// 数据收集
// ===========================================================================

/**
 * @brief 从编辑器收集模板块
 *
 * 从 ReportEditor 获取报告内容，然后提取所有内容块。
 *
 * @return QList<ContentBlock> 内容块列表
 */
QString TemplateEditorDialog::collectDocument()
{
    Report::Ptr report = m_editor->saveToReport();
    // 同步文档内嵌对象到模板（与 document 中的对象锚点对应）
    m_template->setObjects(report->objects());
    return report->document();
}

/**
 * @brief 获取模板数据
 *
 * @return Template::Ptr 模板对象智能指针
 */
Template::Ptr TemplateEditorDialog::templateData() const
{
    return m_template;
}
