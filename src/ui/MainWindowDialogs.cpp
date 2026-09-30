/**
 * @file MainWindowDialogs.cpp
 * @brief 主窗口对话框操作控制器实现
 *
 * 自 MainWindow 迁移（onNewProject/onNewReport/onImportData/onExportProject、
 * onEditProject/onDeleteProject/onDeleteReport、onTemplateManager/onTagManager/
 * onChangePassword/onDataBackup/onDataRestore/onSettings、onAbout/onAboutQt/
 * onCheckUpdate/onUserManager），对话框父窗口与窗口能力经构造注入。
 */

#include "MainWindowDialogs.h"
#include "service/AuditService.h"

#include "ui/dialogs/ProjectDialog.h"
#include "ui/dialogs/TemplateEditorDialog.h"
#include "ui/dialogs/TagManagerDialog.h"
#include "ui/dialogs/ChangePasswordDialog.h"
#include "ui/dialogs/SettingsDialog.h"
#include "ui/dialogs/UserManagerDialog.h"
#include "service/ReportService.h"
#include "service/ProjectService.h"
#include "service/UserService.h"
#include "service/TemplateService.h"
#include "service/DataTableService.h"
#include "utils/CsvImporter.h"
#include "core/models/DataTable.h"
#include "core/models/Report.h"
#include "core/models/Project.h"
#include "core/models/User.h"
#include "data/database/DatabaseManager.h"
#include "export/ExportManager.h"
#include "core/utils/Logger.h"
#include "core/utils/AppConstants.h"
#include "core/utils/AppTheme.h"
#include "core/utils/UserSession.h"
#include "ui/UiHelper.h"
#include "core/models/Tag.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QDate>
#include <QStringList>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QFile>
#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QLineEdit>

namespace {
inline QString trText(const char* source)
{
    return QCoreApplication::translate("MainWindowDialogs", source);
}
}

MainWindowDialogs::MainWindowDialogs(QWidget* parent, Hooks hooks)
    : m_parent(parent)
    , m_hooks(std::move(hooks))
{
}

// ===========================================================================
// 文件菜单
// ===========================================================================

void MainWindowDialogs::newProject()
{
    ProjectDialog dialog(m_parent);
    dialog.setWindowTitle(trText("新建项目"));

    if (dialog.exec() == QDialog::Accepted) {
        Project::Ptr project = dialog.projectData();
        if (ProjectService::save(project)) {
            if (m_hooks.refreshProjectTree) m_hooks.refreshProjectTree();
            if (m_hooks.selectProject) m_hooks.selectProject(project->id());
            if (m_hooks.showStatus) m_hooks.showStatus(trText("项目「%1」已创建").arg(project->name()));
        } else {
            UiHelper::error(m_parent, trText("错误"), trText("创建项目失败，请查看日志"));
        }
    }
}

void MainWindowDialogs::newReport()
{
    const qint64 projectId = m_hooks.currentProjectId ? m_hooks.currentProjectId() : -1;
    if (projectId <= 0) {
        UiHelper::info(m_parent, trText("提示"), trText("请先在左侧选择一个项目"));
        return;
    }

    // 选择模板（可见性过滤：public + 当前用户的 private）
    Template::List allTemplates = TemplateService::listAll();
    Template::List templates;
    const qint64 currentUserId = UserSession::instance().userId();
    for (const Template::Ptr& t : allTemplates) {
        if (t->isPublic()) {
            templates.append(t);
        } else if (t->createdBy() == currentUserId) {
            templates.append(t);
        }
    }
    if (templates.isEmpty()) {
        UiHelper::warning(m_parent, trText("提示"), trText("没有可用的报告模板"));
        return;
    }

    QStringList templateNames;
    for (const Template::Ptr& t : templates) {
        const QString visMark = t->isPrivate() ? trText(" [私有]") : "";
        templateNames.append(t->name() + visMark);
    }

    bool ok = false;
    const QString selected = QInputDialog::getItem(
        m_parent, trText("选择模板"), trText("请选择报告模板:"),
        templateNames, 0, false, &ok);

    if (!ok || selected.isEmpty()) return;

    // 找到选中的模板（去掉可能的 [私有] 后缀）
    Template::Ptr selectedTemplate;
    for (const Template::Ptr& t : templates) {
        QString visMark = t->isPrivate() ? trText(" [私有]") : "";
        if (t->name() + visMark == selected) {
            selectedTemplate = t;
            break;
        }
    }

    if (!selectedTemplate) return;

    // 输入报告标题
    bool titleOk = false;
    const QString title = QInputDialog::getText(
        m_parent, trText("新建报告"), trText("请输入报告标题:"),
        QLineEdit::Normal, trText("未命名实验报告"), &titleOk);

    if (!titleOk || title.trimmed().isEmpty()) return;

    // 创建报告
    Report::Ptr report = Report::create();
    report->setProjectId(projectId);
    report->setTemplateId(selectedTemplate->id());
    report->setTitle(title.trimmed());
    report->setExperimentDate(QDate::currentDate());
    // 新建报告时自动设置创建者为当前用户
    report->setCreatedBy(UserSession::instance().userId());
    report->setAuthor(UserSession::instance().displayName());

    // 从模板复制文档内容与内嵌对象（模板为连续文档 HTML + 对象锚点）
    report->setDocument(selectedTemplate->document());
    report->setObjects(selectedTemplate->objects());

    if (ReportService::save(report)) {
        if (m_hooks.refreshReportList) m_hooks.refreshReportList();
        if (m_hooks.showStatus) m_hooks.showStatus(trText("报告「%1」已创建").arg(report->title()));
    } else {
        UiHelper::error(m_parent, trText("错误"), trText("创建报告失败"));
    }
}

void MainWindowDialogs::importData()
{
    // 文件选择过滤器（内置 CSV 导入）
    QStringList filters;
    filters.append(CsvImporter::fileFilter());
    filters.append(trText("所有文件 (*.*)"));

    // 让用户选择文件
    const QString filePath = QFileDialog::getOpenFileName(m_parent,
        trText("导入数据"), QDir::homePath(), filters.join(";;"));

    if (filePath.isEmpty()) return;

    const QFileInfo fileInfo(filePath);
    const QString suffix = fileInfo.suffix().toLower();

    // 仅支持 csv / txt 格式
    if (!CsvImporter::supportedFormats().contains(suffix)) {
        UiHelper::warning(m_parent, trText("错误"),
            trText("不支持的文件格式：%1\n仅支持 %2 格式。")
                .arg(suffix.isEmpty() ? trText("未知") : suffix)
                .arg(CsvImporter::supportedFormats().join(", ")));
        return;
    }

    // 执行导入
    QString errorMessage;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    DataTable::Ptr table = CsvImporter::importFile(filePath, &errorMessage);
    QApplication::restoreOverrideCursor();

    if (!table) {
        UiHelper::warning(m_parent, trText("导入失败"),
            errorMessage.isEmpty() ? trText("数据导入失败，请检查文件格式") : errorMessage);
        return;
    }

    // 设置数据表名称（使用文件名）
    if (table->name().isEmpty()) {
        table->setName(fileInfo.baseName());
    }
    // 导入的数据表为全局数据表，不关联到特定报告
    table->setReportId(0);

    // 保存到数据库
    if (!DataTableService::save(table)) {
        UiHelper::warning(m_parent, trText("保存失败"), trText("数据表保存到数据库失败"));
        return;
    }

    UiHelper::info(m_parent, trText("导入成功"),
        trText("数据导入成功！\n\n"
               "数据表名称：%1\n"
               "行数：%2\n"
               "列数：%3\n\n"
               "可在报告编辑器的图表配置中选择此数据表作为数据源。")
            .arg(table->name())
            .arg(table->rowCount())
            .arg(table->columnCount()));

    if (m_hooks.showStatus) m_hooks.showStatus(trText("数据导入成功：%1").arg(table->name()));
    LOG_INFO(QString("数据导入成功: %1 (行数=%2, 列数=%3)")
                 .arg(table->name()).arg(table->rowCount()).arg(table->columnCount()));
}

void MainWindowDialogs::exportProject()
{
    const qint64 projectId = m_hooks.currentProjectId ? m_hooks.currentProjectId() : -1;
    if (projectId <= 0) {
        UiHelper::info(m_parent, trText("提示"), trText("请先在左侧选择要导出的项目"));
        return;
    }

    // 加载项目信息
    Project::Ptr project = ProjectService::getById(projectId);
    if (!project) {
        UiHelper::warning(m_parent, trText("错误"), trText("无法加载项目信息"));
        return;
    }

    // 查询该项目下的所有报告
    QList<Report::Ptr> reports = ReportService::listByProject(projectId);
    if (reports.isEmpty()) {
        UiHelper::info(m_parent, trText("提示"), trText("该项目下没有报告可导出"));
        return;
    }

    // 让用户选择导出格式（传 &ok 准确区分"确定"与"取消"）
    bool ok = false;
    const QString formatStr = QInputDialog::getItem(m_parent, trText("导出项目"),
        trText("选择导出格式："),
        QStringList() << trText("PDF 文档") << trText("HTML 网页") << trText("Word 文档") << trText("纯文本"),
        0, false, &ok);

    if (!ok || formatStr.isEmpty()) {
        return;  // 用户取消
    }

    // 解析选择的格式
    ExportFormat format = ExportFormat::Pdf;
    QString ext = "pdf";
    if (formatStr == trText("PDF 文档")) {
        format = ExportFormat::Pdf;
        ext = "pdf";
    } else if (formatStr == trText("HTML 网页")) {
        format = ExportFormat::Html;
        ext = "html";
    } else if (formatStr == trText("Word 文档")) {
        format = ExportFormat::Word;
        ext = "docx";
    } else if (formatStr == trText("纯文本")) {
        format = ExportFormat::Text;
        ext = "txt";
    }

    // 让用户选择保存目录
    const QString dirPath = QFileDialog::getExistingDirectory(m_parent,
        trText("选择导出目录"), QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dirPath.isEmpty()) {
        return;  // 用户取消
    }

    // 创建以项目名命名的子目录
    const QString projectDir = QDir(dirPath).filePath(project->name());
    QDir().mkpath(projectDir);

    // 循环导出每个报告
    int successCount = 0;
    int failCount = 0;
    QStringList failedReports;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    ExportManager exporter;
    for (const Report::Ptr& report : reports) {
        // 构造文件名（用报告标题，替换非法字符）
        QString fileName = report->title();
        if (fileName.isEmpty()) {
            fileName = trText("未命名报告_%1").arg(report->id());
        }
        // 替换文件名中的非法字符
        fileName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
        const QString filePath = QDir(projectDir).filePath(QString("%1.%2").arg(fileName).arg(ext));

        // 执行导出
        ExportConfig config;
        config.format = format;
        config.filePath = filePath;
        config.includeTitle = true;
        config.includeMeta = true;

        if (exporter.exportReport(report, config, m_parent)) {
            successCount++;
        } else {
            failCount++;
            failedReports << report->title();
        }
    }

    QApplication::restoreOverrideCursor();

    // 显示导出结果
    QString message = trText("项目导出完成！\n\n"
                             "成功：%1 份\n"
                             "失败：%2 份\n"
                             "导出目录：\n%3").arg(successCount).arg(failCount).arg(projectDir);

    if (!failedReports.isEmpty()) {
        message += trText("\n\n失败的报告：\n") + failedReports.join("\n");
    }

    if (m_hooks.showStatus) m_hooks.showStatus(trText("项目导出完成：成功 %1，失败 %2").arg(successCount).arg(failCount));

    if (failCount == 0) {
        UiHelper::info(m_parent, trText("导出成功"), message);
    } else {
        UiHelper::warning(m_parent, trText("导出完成（部分失败）"), message);
    }
}

// ===========================================================================
// 编辑菜单
// ===========================================================================

void MainWindowDialogs::editProject()
{
    const qint64 projectId = m_hooks.currentProjectId ? m_hooks.currentProjectId() : -1;
    if (projectId <= 0) return;

    Project::Ptr project = ProjectService::getById(projectId);
    if (!project) return;

    ProjectDialog dialog(m_parent);
    dialog.setWindowTitle(trText("编辑项目"));
    dialog.setProjectData(project);

    if (dialog.exec() == QDialog::Accepted) {
        Project::Ptr updated = dialog.projectData();
        updated->setId(projectId);
        if (ProjectService::update(updated)) {
            if (m_hooks.refreshProjectTree) m_hooks.refreshProjectTree();
            if (m_hooks.selectProject) m_hooks.selectProject(projectId);
            if (m_hooks.showStatus) m_hooks.showStatus(trText("项目已更新"));
        } else {
            UiHelper::error(m_parent, trText("错误"), trText("更新项目失败"));
        }
    }
}

void MainWindowDialogs::deleteProject(qint64 projectId)
{
    if (projectId <= 0) return;

    Project::Ptr project = ProjectService::getById(projectId);
    if (!project) return;

    if (UiHelper::confirm(m_parent,
                       trText("确认删除"),
                       trText("确定要删除项目「%1」吗？\n该项目下的所有报告将被同时删除，此操作不可恢复！").arg(project->name()))) {
        if (ProjectService::remove(projectId)) {
            if (m_hooks.refreshProjectTree) m_hooks.refreshProjectTree();
            // 清除报告列表的项目过滤，回到全部报告视图
            if (m_hooks.setReportListProjectId) m_hooks.setReportListProjectId(-1);
            if (m_hooks.showStatus) m_hooks.showStatus(trText("项目已删除"));
        } else {
            UiHelper::error(m_parent, trText("错误"), trText("删除项目失败"));
        }
    }
}

void MainWindowDialogs::deleteReport(qint64 reportId)
{
    if (reportId <= 0) return;

    Report::Ptr report = ReportService::getById(reportId);
    if (!report) return;

    if (UiHelper::confirm(m_parent,
                       trText("确认删除"),
                       trText("确定要删除报告「%1」吗？此操作不可恢复！").arg(report->title()))) {
        if (ReportService::remove(reportId)) {
            if (m_hooks.refreshReportList) m_hooks.refreshReportList();
            if (m_hooks.showStatus) m_hooks.showStatus(trText("报告已删除"));
        } else {
            UiHelper::error(m_parent, trText("错误"), trText("删除报告失败"));
        }
    }
}

// ===========================================================================
// 工具菜单
// ===========================================================================

void MainWindowDialogs::templateManager()
{
    // 获取所有模板
    const Template::List templates = TemplateService::listAll();

    // 构建模板列表供用户选择
    QStringList items;
    items.append(trText("--- 新建模板 ---"));
    for (const Template::Ptr& t : templates) {
        const QString builtinMark = t->isBuiltin() ? trText(" [内置]") : "";
        items.append(QString("%1 (%2)%3").arg(t->name(), t->category(), builtinMark));
    }

    bool ok = false;
    const QString selected = QInputDialog::getItem(
        m_parent, trText("模板管理器"), trText("选择要编辑的模板，或新建模板:"),
        items, 0, false, &ok);

    if (!ok || selected.isEmpty()) return;

    if (selected == items.first()) {
        // 新建模板
        TemplateEditorDialog dialog(m_parent);
        if (dialog.exec() == QDialog::Accepted) {
            if (m_hooks.showStatus) m_hooks.showStatus(trText("模板「%1」已创建").arg(dialog.templateData()->name()));
        }
    } else {
        // 编辑现有模板
        const int idx = items.indexOf(selected) - 1;  // 减 1 因为第一项是"新建"
        if (idx >= 0 && idx < templates.size()) {
            Template::Ptr temp = templates.at(idx);

            // 内置模板需要先复制才能编辑
            if (temp->isBuiltin()) {
                if (!UiHelper::confirm(m_parent,
                                       trText("内置模板"),
                                       trText("「%1」是内置模板，不能直接修改。\n是否创建一个副本进行编辑？")
                                           .arg(temp->name()))) return;

                // 创建副本
                Template::Ptr copy = Template::create();
                copy->setName(temp->name() + trText(" (副本)"));
                copy->setCategory(temp->category());
                copy->setDescription(temp->description());
                copy->setDocument(temp->document());
                TemplateService::save(copy);
                temp = copy;
            }

            TemplateEditorDialog dialog(m_parent, temp);
            if (dialog.exec() == QDialog::Accepted) {
                if (m_hooks.showStatus) m_hooks.showStatus(trText("模板「%1」已更新").arg(dialog.templateData()->name()));
            }
        }
    }
}

void MainWindowDialogs::tagManager()
{
    TagManagerDialog dialog(m_parent);
    dialog.exec();
    // 标签可能被修改，刷新报告列表和属性面板
    if (m_hooks.refreshReportList) m_hooks.refreshReportList();
    if (m_hooks.updatePropertyPanel) m_hooks.updatePropertyPanel();
}

void MainWindowDialogs::changePassword()
{
    ChangePasswordDialog dialog(m_parent);
    if (dialog.exec() != QDialog::Accepted) return;

    // 验证原密码
    const QString username = UserSession::instance().username();
    User::Ptr user = UserService::authenticate(username, dialog.oldPassword());
    if (!user) {
        UiHelper::warning(m_parent, trText("修改失败"), trText("原密码不正确"));
        return;
    }

    // 修改密码
    if (UserService::changePassword(user->id(), dialog.newPassword())) {
        UiHelper::info(m_parent, trText("修改成功"), trText("密码已修改成功，下次登录请使用新密码"));
    } else {
        UiHelper::error(m_parent, trText("修改失败"), trText("修改密码时发生错误"));
    }
}

void MainWindowDialogs::dataBackup()
{
    // 默认备份目录：应用 data/backup/，文件名带时间戳
    const QString backupDir = QCoreApplication::applicationDirPath()
        + "/data/backup";
    QDir().mkpath(backupDir);
    const QString defaultName = "experiment_report_backup_"
        + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".db";

    const QString filePath = QFileDialog::getSaveFileName(
        m_parent, trText("备份数据"),
        backupDir + "/" + defaultName,
        trText("数据库文件 (*.db);;所有文件 (*)"));

    if (filePath.isEmpty()) return;

    const QString dbPath = QCoreApplication::applicationDirPath()
        + "/data/experiment_reports.db";

    if (QFile::copy(dbPath, filePath)) {
        // 自动清理：备份目录下只保留最近 10 份
        QDir dir(backupDir, "*.db", QDir::Time, QDir::Files);
        const QStringList entries = dir.entryList();
        for (int i = 10; i < entries.size(); ++i) {
            QFile::remove(backupDir + "/" + entries.at(i));
        }
        UiHelper::info(m_parent, trText("备份成功"),
            trText("数据已备份到:\n%1\n\n（备份目录自动保留最近 10 份）").arg(filePath));
        if (m_hooks.showStatus) m_hooks.showStatus(trText("数据备份完成"));
        AuditService::log(trText("数据备份"), filePath);
    } else {
        UiHelper::error(m_parent, trText("备份失败"),
            trText("无法复制数据库文件。\n请确保目标路径可写。"));
    }
}

void MainWindowDialogs::dataRestore()
{
    // 选择备份文件
    const QString filePath = QFileDialog::getOpenFileName(
        m_parent, trText("选择备份文件"),
        QDir::homePath(),
        trText("数据库文件 (*.db);;所有文件 (*)"));
    if (filePath.isEmpty()) return;

    // 确认覆盖（恢复会替换当前数据库）
    if (!UiHelper::confirm(m_parent, trText("数据恢复"),
            trText("恢复数据将覆盖当前所有数据！\n\n"
                   "确定要从备份文件恢复吗？\n"
                   "建议先执行\"数据备份\"再继续。\n\n"
                   "所选备份: %1").arg(filePath))) {
        return;
    }

    const QString dbPath = QCoreApplication::applicationDirPath()
        + "/data/experiment_reports.db";

    // 关闭数据库连接（释放文件锁）
    DatabaseManager::instance().close();

    // 安全替换：旧库先重命名为 .bak，失败可回滚
    const QString bakPath = dbPath + ".pre_restore.bak";
    QFile::remove(bakPath);  // 清理旧的临时备份
    if (!QFile::rename(dbPath, bakPath)) {
        DatabaseManager::instance().initialize(dbPath);
        UiHelper::error(m_parent, trText("恢复失败"),
            trText("无法锁定数据库文件。\n请关闭其他占用该文件的程序后重试。"));
        return;
    }

    if (!QFile::copy(filePath, dbPath)) {
        // 回滚
        QFile::rename(bakPath, dbPath);
        DatabaseManager::instance().initialize(dbPath);
        UiHelper::error(m_parent, trText("恢复失败"),
            trText("无法复制备份文件，已回滚到原数据库。"));
        return;
    }
    QFile::remove(bakPath);

    AuditService::log(trText("数据恢复"), filePath);

    // 重新初始化数据库并刷新界面
    DatabaseManager::instance().initialize(dbPath);

    // 关闭所有已打开的报告编辑器窗口（数据已变，旧窗口内容失效）
    if (m_hooks.closeAllEditorWindows && !m_hooks.closeAllEditorWindows()) {
        return;
    }

    if (m_hooks.refreshProjectTree) m_hooks.refreshProjectTree();
    if (m_hooks.refreshReportList) m_hooks.refreshReportList();
    if (m_hooks.updateStatusBar) m_hooks.updateStatusBar();
    if (m_hooks.updatePropertyPanel) m_hooks.updatePropertyPanel();
    if (m_hooks.updateActionsState) m_hooks.updateActionsState();

    UiHelper::info(m_parent, trText("恢复成功"),
        trText("数据已从备份恢复，界面已刷新。"));
}

void MainWindowDialogs::settings()
{
    SettingsDialog dialog(m_parent);
    dialog.exec();
}

// ===========================================================================
// 帮助菜单
// ===========================================================================

void MainWindowDialogs::about()
{
    QMessageBox::about(m_parent, trText("关于 %1").arg(AppConstants::APP_DISPLAY_NAME),
        QString(
            "<p style='font-size:18pt;font-weight:bold;margin:0 0 8px 0;'>%1</p>"
            "<p>版本: %2</p>"
            "<p>基于 Qt %3 + C++ 开发的实验报告记录工具</p>"
            "<p>功能特性：</p>"
            "<ul>"
            "<li>项目树状管理</li>"
            "<li>模板化报告创建</li>"
            "<li>结构化富文本编辑</li>"
            "<li>实验数据表格与图表</li>"
            "<li>多格式导出（PDF/Word/HTML）</li>"
            "<li>全文检索</li>"
            "</ul>"
            "<p style='color: %4; font-size: %5px;'>%6</p>"
        ).arg(AppConstants::APP_DISPLAY_NAME)
         .arg(AppConstants::APP_VERSION)
         .arg(qVersion())
         .arg(AppTheme::Color::TextSecondary)
         .arg(AppTheme::FontSize::ExtraSmall)
         .arg(trText("© 2024 实验报告记录工具开发组")));
}

void MainWindowDialogs::aboutQt()
{
    QMessageBox::aboutQt(m_parent, trText("关于 Qt"));
}

void MainWindowDialogs::checkUpdate()
{
    UiHelper::info(m_parent, trText("检查更新"),
        trText("当前版本: %1\n\n"
               "本程序为离线单机版，无在线更新服务。\n"
               "如需升级，请获取最新版本安装包后覆盖安装。")
            .arg(AppConstants::APP_VERSION));
}

void MainWindowDialogs::userManager()
{
    // 超级管理员/总管/组长 可访问（对话框内部按权限过滤可见用户与可执行操作）
    const User::Ptr current = UserSession::instance().currentUser();
    if (!current || (!current->isSuperAdmin() && !current->isManager() && !current->isLeader())) {
        UiHelper::warning(m_parent, trText("权限不足"), trText("只有管理员和组长可以管理用户"));
        return;
    }

    UserManagerDialog dialog(m_parent);
    dialog.exec();
}
