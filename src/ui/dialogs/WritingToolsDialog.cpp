#include "WritingToolsDialog.h"
#include "ui_WritingToolsDialog.h"

#include "extension/ToolRegistry.h"

#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>

namespace {

/// QVariantMap 按键名排序展示
QMap<QString, QVariant> orderedStats(const QVariantMap& stats)
{
    return QMap<QString, QVariant>(stats);
}

}  // namespace

WritingToolsDialog::WritingToolsDialog(const DocumentContext& ctx, QWidget* parent)
    : BaseDialog(parent)
    , ui(new Ui::WritingToolsDialog)
    , m_ctx(ctx)
{
    ui->setupUi(this);
    setWindowTitle(tr("写作工具"));

    ToolRegistry& reg = ToolRegistry::instance();
    reg.ensureBuiltinTools();
    for (const DocumentToolPtr& tool : reg.tools()) {
        ui->toolCombo->addItem(tool->displayName(), tool->toolId());
    }

    // 切换工具自动执行（不再需要每次手动点击）
    connect(ui->toolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { onRun(); });
    connect(ui->runBtn, &QPushButton::clicked, this, &WritingToolsDialog::onRun);
    connect(ui->insertBtn, &QPushButton::clicked, this, &WritingToolsDialog::onInsert);
    connect(ui->closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    ui->insertBtn->setEnabled(false);
    onRun();  // 打开时默认执行第一个工具
}

WritingToolsDialog::~WritingToolsDialog()
{
    delete ui;
}

void WritingToolsDialog::onRun()
{
    const QString id = ui->toolCombo->currentData().toString();
    m_activeToolId = id;
    DocumentToolPtr tool = ToolRegistry::instance().toolById(id);
    if (!tool) return;

    QString err;
    const DocumentToolResult r = tool->execute(m_ctx, &err);
    if (!err.isEmpty()) {
        QMessageBox::warning(this, tr("写作工具"), err);
        return;
    }

    ui->resultTable->clearContents();
    ui->resultTable->setRowCount(0);
    m_snippetNames.clear();
    ui->insertBtn->setEnabled(false);

    if (id == QLatin1String("word_count")) {
        ui->resultTable->setColumnCount(2);
        ui->resultTable->setHorizontalHeaderLabels({tr("指标"), tr("数值")});
        const QMap<QString, QVariant> stats = orderedStats(r.stats);
        ui->resultTable->setRowCount(stats.size());
        int row = 0;
        for (auto it = stats.constBegin(); it != stats.constEnd(); ++it, ++row) {
            ui->resultTable->setItem(row, 0, new QTableWidgetItem(it.key()));
            ui->resultTable->setItem(row, 1, new QTableWidgetItem(it.value().toString()));
        }
        ui->hintLabel->setText(r.messages.isEmpty() ? QString()
                                                    : r.messages.first());
    } else if (id == QLatin1String("snippet")) {
        ui->resultTable->setColumnCount(1);
        ui->resultTable->setHorizontalHeaderLabels({tr("常用片段")});
        m_snippetNames = r.messages;
        ui->resultTable->setRowCount(m_snippetNames.size());
        for (int i = 0; i < m_snippetNames.size(); ++i) {
            ui->resultTable->setItem(i, 0, new QTableWidgetItem(m_snippetNames.at(i)));
        }
        ui->hintLabel->setText(tr("选中片段后点击「插入到光标处」"));
        ui->insertBtn->setEnabled(true);
    } else {
        ui->resultTable->setColumnCount(2);
        ui->resultTable->setHorizontalHeaderLabels({tr("序号"), tr("提示")});
        ui->resultTable->setRowCount(r.messages.size());
        for (int i = 0; i < r.messages.size(); ++i) {
            ui->resultTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
            ui->resultTable->setItem(i, 1, new QTableWidgetItem(r.messages.at(i)));
        }
        ui->hintLabel->setText(r.stats.value(QStringLiteral("summary")).toString());
    }
    ui->resultTable->resizeColumnsToContents();
}

void WritingToolsDialog::onInsert()
{
    if (m_activeToolId != QLatin1String("snippet")) return;
    const int row = ui->resultTable->currentRow();
    if (row < 0 || row >= m_snippetNames.size()) return;

    DocumentToolPtr tool = ToolRegistry::instance().toolById(m_activeToolId);
    if (!tool) return;
    const QString text = tool->snippetText(m_snippetNames.at(row));
    if (text.isEmpty()) return;

    m_insertRequested = true;
    m_insertText = text;
    m_insertPos = (m_ctx.cursorPos >= 0) ? m_ctx.cursorPos : m_ctx.text.length();
    accept();
}
