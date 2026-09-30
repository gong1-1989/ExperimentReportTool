#include "AnalysisDialog.h"
#include "ui_AnalysisDialog.h"

#include "core/models/DataTable.h"
#include "extension/AnalyzerRegistry.h"
#include "extension/DataAnalyzer.h"
#include "ui/UiHelper.h"

#include <QComboBox>
#include <QListWidget>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QFont>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "ui/widgets/SimpleChartWidget.h"
#include <QVBoxLayout>

AnalysisDialog::AnalysisDialog(const DataTable::Ptr& table, QWidget* parent)
    : BaseDialog(parent)
    , ui(new Ui::AnalysisDialog)
    , m_table(table)
{
    ui->setupUi(this);

    // 自绘图表挂载到容器（布局填充）
    m_chartWidget = new SimpleChartWidget(ui->chartContainer);
    {
        QVBoxLayout* lay = new QVBoxLayout(ui->chartContainer);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->addWidget(m_chartWidget);
    }

    setWindowTitle(tr("数据分析"));
    resize(780, 680);

    // 列名（含单位）
    if (m_table) {
        for (const ColumnDefinition& col : m_table->columns()) {
            QString name = col.name;
            if (!col.unit.trimmed().isEmpty())
                name += QStringLiteral(" (%1)").arg(col.unit.trimmed());
            m_columnNames.append(name);
        }
    }
    ui->col1Combo->addItems(m_columnNames);
    ui->col2Combo->addItems(m_columnNames);
    ui->multiList->addItems(m_columnNames);
    ui->multiList->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // 方法下拉（异常值检测用）
    ui->methodCombo->addItem(tr("3σ 法"), QStringLiteral("3sigma"));
    ui->methodCombo->addItem(tr("箱线图 (IQR) 法"), QStringLiteral("iqr"));

    // 分析器下拉（AnalyzerRegistry 自动聚合）
    AnalyzerRegistry::instance().ensureBuiltinAnalyzers();
    for (DataAnalyzer* a : AnalyzerRegistry::instance().allAnalyzers())
        ui->analyzerCombo->addItem(a->displayName(), a->analyzerId());
    if (ui->analyzerCombo->count() > 0) onAnalyzerChanged();

    connect(ui->analyzerCombo, &QComboBox::currentIndexChanged,
            this, &AnalysisDialog::onAnalyzerChanged);
    connect(ui->runBtn, &QPushButton::clicked, this, &AnalysisDialog::onRunClicked);

    updateColumnControls();
}

AnalysisDialog::~AnalysisDialog()
{
    delete ui;
}

void AnalysisDialog::onAnalyzerChanged()
{
    DataAnalyzer* a = AnalyzerRegistry::instance().analyzerById(
        ui->analyzerCombo->currentData().toString());
    ui->descLabel->setText(a ? a->description() : QString());
    ui->col1Label->setVisible(true);
    ui->col1Combo->setVisible(true);
    updateColumnControls();
}

void AnalysisDialog::updateColumnControls()
{
    DataAnalyzer* a = AnalyzerRegistry::instance().analyzerById(
        ui->analyzerCombo->currentData().toString());
    const int need = a ? a->requiredColumns() : 1;

    const bool multi = (need < 0);
    ui->col1Label->setVisible(!multi);
    ui->col1Combo->setVisible(!multi);
    ui->col2Label->setVisible(!multi);
    ui->col2Combo->setVisible(!multi);
    ui->multiLabel->setVisible(multi);
    ui->multiList->setVisible(multi);

    // 配对勾选：仅 t 检验
    ui->pairedCheck->setVisible(a && a->analyzerId() == QStringLiteral("t-test"));
    // 方法下拉：仅异常值检测
    ui->methodCombo->setVisible(a && a->analyzerId() == QStringLiteral("outlier-detection"));

    if (!multi) {
        // 双列时列2 标签/下拉可用；单列时隐藏
        const bool two = (need == 2);
        ui->col2Label->setVisible(two && !multi);
        ui->col2Combo->setVisible(two && !multi);
        ui->col1Label->setText(need == 2 ? tr("X/组1 列：") : tr("列："));
    }
    if (a && a->analyzerId() == QStringLiteral("linear-regression"))
        ui->col1Label->setText(tr("X 列："));
}

void AnalysisDialog::onRunClicked()
{
    if (!m_table || m_running) return;
    if (m_table->rows().isEmpty()) {
        UiHelper::warning(this, tr("提示"), tr("数据表为空，无法分析"));
        return;
    }

    DataAnalyzer* a = AnalyzerRegistry::instance().analyzerById(
        ui->analyzerCombo->currentData().toString());
    if (!a) return;
    m_running = true;
    ui->runBtn->setEnabled(false);
    ui->conclusionLabel->setText(tr("正在计算..."));
    ui->warningLabel->clear();

    // 组装参数
    QVariantMap params;
    const int need = a->requiredColumns();
    if (need < 0) {
        QVariantList idxs;
        const QList<QListWidgetItem*> items = ui->multiList->selectedItems();
        for (QListWidgetItem* it : items) idxs.append(ui->multiList->row(it));
        if (idxs.size() < 2) {
            UiHelper::warning(this, tr("提示"), tr("多列分析至少选择 2 列"));
            m_running = false;
            ui->runBtn->setEnabled(true);
            return;
        }
        params.insert(QStringLiteral("columnIndexes"), idxs);
    } else {
        const int c1 = ui->col1Combo->currentIndex();
        params.insert(QStringLiteral("columnIndex"), c1);
        if (need == 2) {
            const int c2 = ui->col2Combo->currentIndex();
            if (c1 == c2) {
                UiHelper::warning(this, tr("提示"), tr("两列不能相同"));
                m_running = false;
                ui->runBtn->setEnabled(true);
                return;
            }
            if (a->analyzerId() == QStringLiteral("linear-regression")) {
                params.insert(QStringLiteral("xColumnIndex"), c1);
                params.insert(QStringLiteral("yColumnIndex"), c2);
            } else {
                params.insert(QStringLiteral("secondColumnIndex"), c2);
                if (a->analyzerId() == QStringLiteral("t-test"))
                    params.insert(QStringLiteral("paired"), ui->pairedCheck->isChecked());
            }
        }
    }
    if (a->analyzerId() == QStringLiteral("outlier-detection"))
        params.insert(QStringLiteral("method"), ui->methodCombo->currentData().toString());

    QString error;
    const AnalysisResult r = a->analyze(m_table->rows(), m_columnNames, params, &error);
    if (!error.isEmpty()) {
        UiHelper::warning(this, tr("无法分析"), error);
        ui->conclusionLabel->setText(error);
        m_running = false;
        ui->runBtn->setEnabled(true);
        return;
    }

    // 展示
    ui->conclusionLabel->setText(QStringLiteral("<b>%1</b><br>%2").arg(r.title.toHtmlEscaped(), r.textConclusion.toHtmlEscaped()));
    QStringList warns = r.warnings;
    ui->warningLabel->setText(warns.isEmpty() ? QString()
        : tr("提示：") + warns.join(QStringLiteral("；")).toHtmlEscaped());

    // 结果表
    ui->resultTable->clear();
    ui->resultTable->setRowCount(0);
    ui->resultTable->setColumnCount(0);
    if (!r.summaryTable.isEmpty()) {
        const int rows = r.summaryTable.size();
        const int cols = r.summaryTable.first().size();
        ui->resultTable->setRowCount(rows);
        ui->resultTable->setColumnCount(cols);
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < r.summaryTable.at(i).size(); ++j) {
                QTableWidgetItem* item = new QTableWidgetItem(r.summaryTable.at(i).at(j));
                if (i == 0) {
                    QFont f = item->font();
                    f.setBold(true);
                    item->setFont(f);
                }
                ui->resultTable->setItem(i, j, item);
            }
        }
        ui->resultTable->resizeColumnsToContents();
    }

    // 图表（chartData 非空时，自绘 scatter）
    QVector<SimpleChartWidget::Series> chartSeries;
    const QJsonObject chartObj = QJsonObject::fromVariantMap(r.chartData);
    const QJsonArray series = chartObj.value(QStringLiteral("series")).toArray();
    for (const QJsonValue& sv : series) {
        const QJsonObject so = sv.toObject();
        SimpleChartWidget::Series sc;
        sc.name = so.value(QStringLiteral("name")).toString();
        const QJsonArray pts = so.value(QStringLiteral("points")).toArray();
        for (const QJsonValue& pv : pts) {
            const QJsonArray p = pv.toArray();
            if (p.size() >= 2) sc.points.append(QPointF(p.at(0).toDouble(), p.at(1).toDouble()));
        }
        chartSeries.append(sc);
    }
    if (!chartSeries.isEmpty()) {
        m_chartWidget->setChart(QStringLiteral("scatter"), chartSeries);
        m_chartWidget->setVisible(true);
    } else {
        m_chartWidget->setChart(QString(), chartSeries);
        m_chartWidget->setVisible(false);
    }

    m_running = false;
    ui->runBtn->setEnabled(true);
}
