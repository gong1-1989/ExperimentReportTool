#include "StatsDialog.h"
#include "ui_StatsDialog.h"

#include "ui/UiHelper.h"
#include "core/utils/Logger.h"
#include "extension/StatsRegistry.h"
#include "service/ProjectService.h"

#include <QDateEdit>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QTextStream>

#include "ui/widgets/SimpleChartWidget.h"

StatsDialog::StatsDialog(const StatsScope& scope, QWidget* parent)
    : BaseDialog(parent)
    , ui(new Ui::StatsDialog)
    , m_chartWidget(nullptr)
    , m_scope(scope)
    , m_groupLocked(scope.groupId > 0)
{
    ui->setupUi(this);

    // 自绘图表挂载到容器（布局填充）
    m_chartWidget = new SimpleChartWidget(ui->chartContainer);
    {
        QVBoxLayout* lay = new QVBoxLayout(ui->chartContainer);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->addWidget(m_chartWidget);
    }

    // 报表类型：惰性注册内置提供者后填充
    StatsRegistry::instance().ensureBuiltinProviders();
    const QList<StatsProviderPtr> providers = StatsRegistry::instance().providers();
    for (const StatsProviderPtr& p : providers) {
        ui->typeCombo->addItem(p->displayName(), p->providerId());
        ui->typeCombo->setItemData(ui->typeCombo->count() - 1, p->description(), Qt::ToolTipRole);
    }

    // 项目筛选（组长视图锁定本组，隐藏项目/时间筛选以外的范围控件保留项目隐藏）
    updateProjectCombo();
    if (m_groupLocked) {
        ui->projectLabel->setVisible(false);
        ui->projectCombo->setVisible(false);
        ui->scopeBox->setTitle(tr("统计范围（本组）"));
    }

    // 时间范围：日期等于最小值时显示"不限"
    ui->fromEdit->setMinimumDate(QDate(2000, 1, 1));
    ui->fromEdit->setDate(QDate(2000, 1, 1));
    ui->toEdit->setMinimumDate(QDate(2000, 1, 1));
    ui->toEdit->setDate(QDate(2000, 1, 1));

    // 首屏自动统计一次
    connect(ui->runBtn, &QPushButton::clicked, this, &StatsDialog::onRun);
    connect(ui->exportBtn, &QPushButton::clicked, this, &StatsDialog::onExportCsv);
    connect(ui->closeBtn, &QPushButton::clicked, this, &StatsDialog::accept);
    connect(ui->typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { onRun(); });

    onRun();
}

StatsDialog::~StatsDialog()
{
    delete ui;
    LOG_DEBUG(QStringLiteral("报表统计: StatsDialog 析构"));
}

void StatsDialog::updateProjectCombo()
{
    ui->projectCombo->clear();
    ui->projectCombo->addItem(tr("全部项目"), -1);
    const Project::List projects = ProjectService::listAll();
    for (const Project::Ptr& p : projects) {
        ui->projectCombo->addItem(p->name(), p->id());
    }
}

void StatsDialog::onRun()
{
    LOG_DEBUG(QStringLiteral("报表统计: onRun 被触发"));
    // 组装统计范围
    StatsScope scope = m_scope;
    if (!m_groupLocked) {
        scope.projectId = ui->projectCombo->currentData().toLongLong();
    }
    if (ui->fromEdit->date() != ui->fromEdit->minimumDate()) scope.from = ui->fromEdit->date();
    else scope.from = QDate();
    if (ui->toEdit->date() != ui->toEdit->minimumDate()) scope.to = ui->toEdit->date();
    else scope.to = QDate();

    const QString providerId = ui->typeCombo->currentData().toString();
    const StatsProviderPtr provider = StatsRegistry::instance().providerById(providerId);
    if (!provider) return;

    LOG_DEBUG(QStringLiteral("报表统计: provider=%1, project=%2, from=%3, to=%4")
                 .arg(providerId).arg(scope.projectId)
                 .arg(scope.from.toString(QStringLiteral("yyyy-MM-dd")))
                 .arg(scope.to.toString(QStringLiteral("yyyy-MM-dd"))));
    QString error;
    const StatsResult result = provider->compute(scope, &error);
    LOG_DEBUG(QStringLiteral("报表统计: compute 完成 rows=%1, chartEmpty=%2")
                 .arg(result.rows.size()).arg(result.chartData.isEmpty()));

    // 结果表
    ui->resultTable->clear();
    ui->resultTable->setColumnCount(1 + result.metricHeaders.size());
    QStringList headers;
    headers << tr("维度") << result.metricHeaders;
    ui->resultTable->setHorizontalHeaderLabels(headers);
    ui->resultTable->setRowCount(result.rows.size());
    for (int r = 0; r < result.rows.size(); ++r) {
        ui->resultTable->setItem(r, 0, new QTableWidgetItem(result.rows.at(r).name));
        for (int c = 0; c < result.rows.at(r).metrics.size(); ++c) {
            QTableWidgetItem* item =
                new QTableWidgetItem(result.rows.at(r).metrics.at(c).toString());
            if (c > 0) item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            ui->resultTable->setItem(r, c + 1, item);
        }
    }
    ui->resultTable->horizontalHeader()->setStretchLastSection(true);

    // 结论与提醒
    ui->conclusionLabel->setText(result.textConclusion);
    ui->warningLabel->setText(result.warnings.isEmpty() ? QString()
                                                        : result.warnings.join(QStringLiteral("；")));

    renderChart(result);
}

void StatsDialog::renderChart(const StatsResult& result)
{
    const QJsonObject chartObj = QJsonObject::fromVariantMap(result.chartData);
    const QString type = chartObj.value(QStringLiteral("type")).toString();
    const QJsonArray series = chartObj.value(QStringLiteral("series")).toArray();
    LOG_DEBUG(QStringLiteral("报表统计: renderChart type=%1, series=%2").arg(type).arg(series.size()));

    QVector<SimpleChartWidget::Series> out;
    out.reserve(series.size());
    for (const QJsonValue& sv : series) {
        const QJsonObject so = sv.toObject();
        SimpleChartWidget::Series s;
        s.name = so.value(QStringLiteral("name")).toString();
        const QJsonArray pts = so.value(QStringLiteral("points")).toArray();
        for (const QJsonValue& pv : pts) {
            const QJsonArray p = pv.toArray();
            if (p.size() >= 2) s.points.append(QPointF(p.at(0).toDouble(), p.at(1).toDouble()));
        }
        out.append(s);
    }

    if (type == QStringLiteral("pie") || type == QStringLiteral("bar")
        || type == QStringLiteral("line") || type == QStringLiteral("scatter")) {
        // bar/line 的 x 类别用维度行名（趋势：时间段名）
        QStringList categories;
        if (type == QStringLiteral("bar") || type == QStringLiteral("line")) {
            for (const StatsRow& row : result.rows) categories << row.name;
        }
        m_chartWidget->setChart(type, out, categories);
        m_chartWidget->setVisible(true);
    } else {
        m_chartWidget->setChart(QString(), out);
        m_chartWidget->setVisible(false);
    }
}

void StatsDialog::onExportCsv()
{
    LOG_DEBUG(QStringLiteral("报表统计: 导出 CSV 被触发"));
    if (ui->resultTable->rowCount() == 0) {
        UiHelper::info(this, tr("导出报表"), tr("当前没有可导出的数据。"));
        return;
    }
    const QString defaultName = QStringLiteral("报表_%1.csv").arg(
        QDate::currentDate().toString(QStringLiteral("yyyyMMdd")));
    const QString filePath = QFileDialog::getSaveFileName(this, tr("导出报表 CSV"), defaultName,
                                                          tr("CSV 文件 (*.csv)"));
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        UiHelper::error(this, tr("导出失败"), tr("无法写入文件：%1").arg(filePath));
        return;
    }

    QTextStream out(&file);  // Qt6 QTextStream 默认 UTF-8（setCodec 已移除）
    out << QChar(0xFEFF);  // UTF-8 BOM，Excel 打开中文不乱码

    // 表头：维度 + 指标列
    QStringList header;
    header << tr("维度");
    for (int c = 1; c < ui->resultTable->columnCount(); ++c)
        header << ui->resultTable->horizontalHeaderItem(c)->text();
    out << header.join(QStringLiteral(",")) << QStringLiteral("\n");

    // 数据行（含逗号/引号转义）
    for (int r = 0; r < ui->resultTable->rowCount(); ++r) {
        QStringList cells;
        for (int c = 0; c < ui->resultTable->columnCount(); ++c) {
            const QTableWidgetItem* it = ui->resultTable->item(r, c);
            QString cell = it ? it->text() : QString();
            if (cell.contains(QStringLiteral(",")) || cell.contains(QStringLiteral("\""))
                || cell.contains(QStringLiteral("\n"))) {
                cell = QStringLiteral("\"") + cell.replace(QStringLiteral("\""),
                                                           QStringLiteral("\"\"")) + QStringLiteral("\"");
            }
            cells << cell;
        }
        out << cells.join(QStringLiteral(",")) << QStringLiteral("\n");
    }
    file.close();

    UiHelper::info(this, tr("导出报表"), tr("已导出到：\n%1").arg(filePath));
}
