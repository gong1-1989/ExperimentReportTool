/**
 * @file SettingsDialog.cpp
 * @brief 设置对话框实现文件
 */

#include "SettingsDialog.h"
#include "core/utils/AppConstants.h"

#include <QTabWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QFontComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>

// ============================================================================
// 构造与析构
// ============================================================================

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
    , m_tabWidget(nullptr)
    , m_autoSaveCheck(nullptr)
    , m_autoSaveIntervalSpin(nullptr)
    , m_exportFormatCombo(nullptr)
    , m_fontCombo(nullptr)
    , m_fontSizeSpin(nullptr)
    , m_dbPathEdit(nullptr)
    , m_okButton(nullptr)
    , m_applyButton(nullptr)
    , m_cancelButton(nullptr)
    , m_resetButton(nullptr)
{
    setWindowTitle(tr("设置"));
    setMinimumSize(500, 450);

    setupUi();
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
}

// ============================================================================
// UI 初始化
// ============================================================================

void SettingsDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    // 选项卡控件
    m_tabWidget = new QTabWidget(this);

    // ------------------------------------------------------------------------
    // 常规设置选项卡
    // ------------------------------------------------------------------------
    QWidget* generalTab = new QWidget(this);
    QVBoxLayout* generalLayout = new QVBoxLayout(generalTab);
    generalLayout->setContentsMargins(16, 16, 16, 16);
    generalLayout->setSpacing(12);

    // 自动保存组
    QGroupBox* autoSaveGroup = new QGroupBox(tr("自动保存"), generalTab);
    QVBoxLayout* autoSaveLayout = new QVBoxLayout(autoSaveGroup);

    m_autoSaveCheck = new QCheckBox(tr("启用自动保存"), autoSaveGroup);
    m_autoSaveCheck->setToolTip(tr("编辑报告时自动保存，防止数据丢失"));
    autoSaveLayout->addWidget(m_autoSaveCheck);

    QHBoxLayout* intervalLayout = new QHBoxLayout();
    intervalLayout->addWidget(new QLabel(tr("保存间隔（分钟）："), autoSaveGroup));
    m_autoSaveIntervalSpin = new QSpinBox(autoSaveGroup);
    m_autoSaveIntervalSpin->setRange(1, 60);
    m_autoSaveIntervalSpin->setValue(5);
    m_autoSaveIntervalSpin->setSuffix(tr(" 分钟"));
    intervalLayout->addWidget(m_autoSaveIntervalSpin);
    intervalLayout->addStretch();
    autoSaveLayout->addLayout(intervalLayout);

    generalLayout->addWidget(autoSaveGroup);

    // 导出设置组
    QGroupBox* exportGroup = new QGroupBox(tr("导出设置"), generalTab);
    QFormLayout* exportLayout = new QFormLayout(exportGroup);

    m_exportFormatCombo = new QComboBox(exportGroup);
    m_exportFormatCombo->addItem(tr("PDF 文档"), "pdf");
    m_exportFormatCombo->addItem(tr("HTML 网页"), "html");
    m_exportFormatCombo->addItem(tr("Word 文档"), "word");
    m_exportFormatCombo->addItem(tr("纯文本"), "text");
    exportLayout->addRow(tr("默认导出格式："), m_exportFormatCombo);

    generalLayout->addWidget(exportGroup);
    generalLayout->addStretch();

    m_tabWidget->addTab(generalTab, tr("常规"));

    // ------------------------------------------------------------------------
    // 编辑器设置选项卡
    // ------------------------------------------------------------------------
    QWidget* editorTab = new QWidget(this);
    QVBoxLayout* editorLayout = new QVBoxLayout(editorTab);
    editorLayout->setContentsMargins(16, 16, 16, 16);
    editorLayout->setSpacing(12);

    QGroupBox* fontGroup = new QGroupBox(tr("默认字体"), editorTab);
    QFormLayout* fontLayout = new QFormLayout(fontGroup);

    m_fontCombo = new QFontComboBox(fontGroup);
    m_fontCombo->setToolTip(tr("新建报告时的默认字体"));
    fontLayout->addRow(tr("字体族："), m_fontCombo);

    m_fontSizeSpin = new QSpinBox(fontGroup);
    m_fontSizeSpin->setRange(8, 72);
    m_fontSizeSpin->setValue(12);
    m_fontSizeSpin->setSuffix(tr(" pt"));
    fontLayout->addRow(tr("字号："), m_fontSizeSpin);

    editorLayout->addWidget(fontGroup);

    // 提示标签
    QLabel* hintLabel = new QLabel(tr(
        "<div style='color: #999; font-size: 12px;'>"
        "注意：字体设置仅对新建的报告生效，已有报告保持原有字体设置。"
        "</div>"), editorTab);
    hintLabel->setWordWrap(true);
    editorLayout->addWidget(hintLabel);

    editorLayout->addStretch();

    m_tabWidget->addTab(editorTab, tr("编辑器"));

    // ------------------------------------------------------------------------
    // 数据设置选项卡
    // ------------------------------------------------------------------------
    QWidget* dataTab = new QWidget(this);
    QVBoxLayout* dataLayout = new QVBoxLayout(dataTab);
    dataLayout->setContentsMargins(16, 16, 16, 16);
    dataLayout->setSpacing(12);

    QGroupBox* dbGroup = new QGroupBox(tr("数据库"), dataTab);
    QVBoxLayout* dbLayout = new QVBoxLayout(dbGroup);

    dbLayout->addWidget(new QLabel(tr("数据库文件路径："), dbGroup));
    m_dbPathEdit = new QLineEdit(dbGroup);
    m_dbPathEdit->setReadOnly(true);
    m_dbPathEdit->setStyleSheet("background-color: #f5f5f5; color: #666;");
    dbLayout->addWidget(m_dbPathEdit);

    // 数据库大小提示
    QLabel* dbHintLabel = new QLabel(tr(
        "<div style='color: #999; font-size: 12px; margin-top: 8px;'>"
        "数据库文件包含所有项目、报告、标签和附件信息。<br>"
        "建议定期备份数据库文件以防止数据丢失。"
        "</div>"), dbGroup);
    dbHintLabel->setWordWrap(true);
    dbLayout->addWidget(dbHintLabel);

    dataLayout->addWidget(dbGroup);
    dataLayout->addStretch();

    m_tabWidget->addTab(dataTab, tr("数据"));

    mainLayout->addWidget(m_tabWidget);

    // ------------------------------------------------------------------------
    // 底部按钮
    // ------------------------------------------------------------------------
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_resetButton = new QPushButton(tr("恢复默认"), this);
    connect(m_resetButton, &QPushButton::clicked, this, &SettingsDialog::onResetDefaults);
    buttonLayout->addWidget(m_resetButton);

    buttonLayout->addStretch();

    m_applyButton = new QPushButton(tr("应用"), this);
    connect(m_applyButton, &QPushButton::clicked, this, &SettingsDialog::onApply);
    buttonLayout->addWidget(m_applyButton);

    m_okButton = new QPushButton(tr("确定"), this);
    m_okButton->setDefault(true);
    connect(m_okButton, &QPushButton::clicked, this, &SettingsDialog::onAccept);
    buttonLayout->addWidget(m_okButton);

    m_cancelButton = new QPushButton(tr("取消"), this);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);
}

// ============================================================================
// 设置加载与保存
// ============================================================================

void SettingsDialog::loadSettings()
{
    QSettings settings;

    // 常规设置
    m_autoSaveCheck->setChecked(settings.value("autoSave/enabled", true).toBool());
    m_autoSaveIntervalSpin->setValue(settings.value("autoSave/interval", 5).toInt());

    const QString exportFormat = settings.value("export/defaultFormat", "pdf").toString();
    const int idx = m_exportFormatCombo->findData(exportFormat);
    m_exportFormatCombo->setCurrentIndex(idx >= 0 ? idx : 0);

    // 编辑器设置
    const QString fontFamily = settings.value("editor/defaultFontFamily", "Microsoft YaHei").toString();
    m_fontCombo->setCurrentFont(QFont(fontFamily));
    m_fontSizeSpin->setValue(settings.value("editor/defaultFontSize", 12).toInt());

    // 数据设置
    m_dbPathEdit->setText(databasePath());
}

void SettingsDialog::saveSettings()
{
    QSettings settings;

    // 常规设置
    settings.setValue("autoSave/enabled", m_autoSaveCheck->isChecked());
    settings.setValue("autoSave/interval", m_autoSaveIntervalSpin->value());
    settings.setValue("export/defaultFormat", m_exportFormatCombo->currentData().toString());

    // 编辑器设置
    settings.setValue("editor/defaultFontFamily", m_fontCombo->currentFont().family());
    settings.setValue("editor/defaultFontSize", m_fontSizeSpin->value());

    settings.sync();
}

void SettingsDialog::applySettings()
{
    saveSettings();
    // 这里可以发出信号通知主窗口应用新设置
    // 目前设置在下次新建报告或导出时生效
}

// ============================================================================
// 槽函数
// ============================================================================

void SettingsDialog::onAccept()
{
    saveSettings();
    accept();
}

void SettingsDialog::onApply()
{
    applySettings();
    QMessageBox::information(this, tr("设置"), tr("设置已应用并保存。"));
}

void SettingsDialog::onResetDefaults()
{
    const QMessageBox::StandardButton ret = QMessageBox::question(
        this, tr("恢复默认"),
        tr("确定要将所有设置恢复为默认值吗？"),
        QMessageBox::Yes | QMessageBox::No);

    if (ret != QMessageBox::Yes) return;

    // 恢复默认值
    m_autoSaveCheck->setChecked(true);
    m_autoSaveIntervalSpin->setValue(5);
    m_exportFormatCombo->setCurrentIndex(0);
    m_fontCombo->setCurrentFont(QFont("Microsoft YaHei"));
    m_fontSizeSpin->setValue(12);
}

// ============================================================================
// 静态便捷方法
// ============================================================================

bool SettingsDialog::autoSaveEnabled()
{
    QSettings settings;
    return settings.value("autoSave/enabled", true).toBool();
}

int SettingsDialog::autoSaveInterval()
{
    QSettings settings;
    return settings.value("autoSave/interval", 5).toInt();
}

QString SettingsDialog::defaultExportFormat()
{
    QSettings settings;
    return settings.value("export/defaultFormat", "pdf").toString();
}

QString SettingsDialog::defaultFontFamily()
{
    QSettings settings;
    return settings.value("editor/defaultFontFamily", "Microsoft YaHei").toString();
}

int SettingsDialog::defaultFontSize()
{
    QSettings settings;
    return settings.value("editor/defaultFontSize", 12).toInt();
}

QString SettingsDialog::databasePath()
{
    // 使用应用数据目录下的数据库文件
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(dataDir).filePath("experiment_report.db");
}
