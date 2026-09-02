/**
 * @file PrintManager.cpp
 * @brief 打印管理器实现文件
 */

#include "PrintManager.h"
#include "core/utils/Logger.h"

#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPageSetupDialog>
#include <QTextDocument>
#include <QTextCursor>
#include <QPainter>
#include <QMessageBox>
#include <QApplication>
#include <QSettings>

// ===========================================================================
// 构造与析构
// ===========================================================================

PrintManager::PrintManager(QObject* parent)
    : QObject(parent)
{
    // 从设置中加载打印配置
    QSettings settings;
    m_config.pageSize = static_cast<QPageSize::PageSizeId>(
        settings.value("print/pageSize", QPageSize::A4).toInt());
    m_config.orientation = static_cast<QPageLayout::Orientation>(
        settings.value("print/orientation", QPageLayout::Portrait).toInt());
    m_config.printHeader = settings.value("print/printHeader", true).toBool();
    m_config.printFooter = settings.value("print/printFooter", true).toBool();
    m_config.printPageNumbers = settings.value("print/printPageNumbers", true).toBool();
}

PrintManager::~PrintManager()
{
    // 保存打印配置
    QSettings settings;
    settings.setValue("print/pageSize", static_cast<int>(m_config.pageSize));
    settings.setValue("print/orientation", static_cast<int>(m_config.orientation));
    settings.setValue("print/printHeader", m_config.printHeader);
    settings.setValue("print/printFooter", m_config.printFooter);
    settings.setValue("print/printPageNumbers", m_config.printPageNumbers);
}

// ===========================================================================
// 打印预览
// ===========================================================================

bool PrintManager::printPreview(const Report::Ptr& report, QWidget* parent)
{
    if (!report) {
        QMessageBox::critical(parent, tr("打印失败"), tr("报告为空"));
        return false;
    }

    QPrinter printer(QPrinter::HighResolution);
    setupPrinter(printer, m_config);

    QPrintPreviewDialog preview(&printer, parent);
    preview.setWindowTitle(tr("打印预览 - %1").arg(report->title()));
    preview.resize(1000, 700);

    // 连接 paintRequested 信号
    connect(&preview, &QPrintPreviewDialog::paintRequested,
            this, [this, report](QPrinter* printer) {
                QTextDocument* doc = renderDocument(report, m_config);
                if (doc) {
                    doc->print(printer);
                    delete doc;
                }
            });

    return preview.exec() == QDialog::Accepted;
}

// ===========================================================================
// 打印
// ===========================================================================

bool PrintManager::print(const Report::Ptr& report, QWidget* parent)
{
    if (!report) {
        QMessageBox::critical(parent, tr("打印失败"), tr("报告为空"));
        return false;
    }

    QPrinter printer(QPrinter::HighResolution);
    setupPrinter(printer, m_config);

    QPrintDialog dialog(&printer, parent);
    dialog.setWindowTitle(tr("打印 - %1").arg(report->title()));

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    // 从对话框更新配置
    m_config.copies = printer.copyCount();
    m_config.collate = printer.collateCopies();

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QTextDocument* doc = renderDocument(report, m_config);
    if (doc) {
        doc->print(&printer);
        delete doc;
    }
    QApplication::restoreOverrideCursor();

    LOG_INFO(QString("报告已打印: %1").arg(report->title()));
    return true;
}

// ===========================================================================
// 使用指定配置打印
// ===========================================================================

bool PrintManager::printWithConfig(const Report::Ptr& report,
                                     const PrintConfig& config,
                                     QWidget* parent)
{
    if (!report) return false;

    QPrinter printer(QPrinter::HighResolution);
    setupPrinter(printer, config);

    QTextDocument* doc = renderDocument(report, config);
    if (doc) {
        doc->print(&printer);
        delete doc;
        return true;
    }
    return false;
}

// ===========================================================================
// 页面设置
// ===========================================================================

bool PrintManager::pageSetup(PrintConfig& config, QWidget* parent)
{
    QPrinter printer(QPrinter::HighResolution);
    setupPrinter(printer, config);

    QPageSetupDialog dialog(&printer, parent);
    dialog.setWindowTitle(tr("页面设置"));

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    // 从打印机更新配置
    config.pageSize = printer.pageLayout().pageSize().id();
    config.orientation = printer.pageLayout().orientation();
    config.margins = printer.pageLayout().margins(QPageLayout::Millimeter);

    m_config = config;
    return true;
}

// ===========================================================================
// 配置 QPrinter
// ===========================================================================

void PrintManager::setupPrinter(QPrinter& printer, const PrintConfig& config)
{
    printer.setPageSize(QPageSize(config.pageSize));
    printer.setPageOrientation(config.orientation);
    printer.setPageMargins(config.margins, QPageLayout::Millimeter);
    printer.setCopyCount(config.copies);
    printer.setCollateCopies(config.collate);

    if (config.printRange == QPrinter::PageRange) {
        printer.setPrintRange(QPrinter::PageRange);
        printer.setFromTo(config.fromPage, config.toPage);
    } else {
        printer.setPrintRange(QPrinter::AllPages);
    }
}

// ===========================================================================
// 渲染文档
// ===========================================================================

QTextDocument* PrintManager::renderDocument(const Report::Ptr& report,
                                              const PrintConfig& config)
{
    if (!report) return nullptr;

    // 使用 ExportManager 生成 HTML
    ExportConfig exportConfig;
    exportConfig.includeTitle = config.includeTitle;
    exportConfig.includeMeta = config.includeMeta;
    exportConfig.includeTableOfContents = config.includeTableOfContents;
    exportConfig.fontFamily = "Microsoft YaHei";
    exportConfig.fontSize = 12;

    ExportManager exporter;
    // 调用 reportToHtml（需要通过一个公开方法或直接使用）
    // 由于 reportToHtml 是私有方法，我们这里直接生成 HTML
    // 实际上可以让 ExportManager 暴露一个 toHtml 方法
    // 简化处理：直接构造 QTextDocument

    QTextDocument* doc = new QTextDocument();
    doc->setDefaultFont(QFont("Microsoft YaHei", 11));

    // 构建 HTML 内容
    QString html;
    html += "<html><head><meta charset='utf-8'><style>";
    html += "body { font-family: 'Microsoft YaHei', sans-serif; font-size: 11pt; line-height: 1.8; color: #333; }";
    html += "h1 { font-size: 22pt; text-align: center; color: #1a1a1a; margin-bottom: 20px; }";
    html += "h2 { font-size: 16pt; color: #2a2a2a; margin-top: 24px; border-bottom: 1px solid #ddd; padding-bottom: 4px; }";
    html += "h3 { font-size: 13pt; color: #333; margin-top: 18px; }";
    html += "p { margin: 10px 0; text-align: justify; }";
    html += "ul, ol { margin: 10px 0; padding-left: 28px; }";
    html += "li { margin: 6px 0; }";
    html += "blockquote { border-left: 3px solid #4A90D9; background: #f0f7ff; margin: 14px 0; padding: 10px 16px; color: #555; font-style: italic; }";
    html += "pre { background: #f5f5f5; border: 1px solid #ddd; padding: 12px; border-radius: 4px; font-family: Consolas, monospace; font-size: 9pt; overflow-x: auto; }";
    html += "code { background: #f0f0f0; padding: 1px 4px; border-radius: 2px; font-family: Consolas, monospace; font-size: 0.9em; }";
    html += "hr { border: none; border-top: 1px solid #ddd; margin: 24px 0; }";
    html += ".meta { background: #f8f9fa; padding: 10px 14px; border-radius: 4px; margin-bottom: 20px; font-size: 10pt; color: #666; }";
    html += ".meta span { margin-right: 16px; }";
    html += "img { max-width: 100%; }";
    html += "</style></head><body>";

    // 标题
    if (config.includeTitle) {
        html += QString("<h1>%1</h1>").arg(report->title().toHtmlEscaped());
    }

    // 元信息
    if (config.includeMeta) {
        html += "<div class='meta'>";
        html += QString("<span><strong>作者:</strong> %1</span>").arg(report->author().toHtmlEscaped());
        html += QString("<span><strong>实验日期:</strong> %1</span>").arg(report->experimentDate().toString("yyyy-MM-dd"));
        html += QString("<span><strong>创建时间:</strong> %1</span>").arg(report->createdAt().toString("yyyy-MM-dd hh:mm"));
        html += "</div>";
    }

    // 内容块
    for (int i = 0; i < report->blockCount(); ++i) {
        const ContentBlock& block = report->blockAt(i);
        switch (block.type) {
        case BlockType::Heading1:
            html += QString("<h2>%1</h2>").arg(block.data.value("text").toString().toHtmlEscaped());
            break;
        case BlockType::Heading2:
            html += QString("<h3>%1</h3>").arg(block.data.value("text").toString().toHtmlEscaped());
            break;
        case BlockType::Heading3:
            html += QString("<h3 style='font-size:12pt;'>%1</h3>").arg(block.data.value("text").toString().toHtmlEscaped());
            break;
        case BlockType::Paragraph:
            html += QString("<p>%1</p>").arg(block.data.value("text").toString().toHtmlEscaped());
            break;
        case BlockType::BulletList:
            html += "<ul>";
            if (block.data.value("items").isArray()) {
                for (const QJsonValue& item : block.data.value("items").toArray()) {
                    html += QString("<li>%1</li>").arg(item.toString().toHtmlEscaped());
                }
            }
            html += "</ul>";
            break;
        case BlockType::NumberedList:
            html += "<ol>";
            if (block.data.value("items").isArray()) {
                for (const QJsonValue& item : block.data.value("items").toArray()) {
                    html += QString("<li>%1</li>").arg(item.toString().toHtmlEscaped());
                }
            }
            html += "</ol>";
            break;
        case BlockType::Quote:
            html += QString("<blockquote>%1</blockquote>").arg(block.data.value("text").toString().toHtmlEscaped());
            break;
        case BlockType::CodeBlock:
            html += QString("<pre><code>%1</code></pre>").arg(block.data.value("code").toString().toHtmlEscaped());
            break;
        case BlockType::Divider:
            html += "<hr>";
            break;
        case BlockType::Image: {
            const QString caption = block.data.value("caption").toString();
            html += QString("<p style='text-align:center; color:#888; font-size:10pt;'>[图片: %1]</p>").arg(caption.toHtmlEscaped());
            break;
        }
        case BlockType::Table:
            html += "<p style='text-align:center; color:#888;'>[表格数据]</p>";
            break;
        case BlockType::Chart:
            html += "<p style='text-align:center; color:#888;'>[图表]</p>";
            break;
        default:
            break;
        }
    }

    html += "</body></html>";
    doc->setHtml(html);

    return doc;
}

// ===========================================================================
// 页眉页脚（预留，当前通过 QTextDocument 实现）
// ===========================================================================

void PrintManager::printHeaderFooter(QPrinter* printer, QPainter* painter,
                                       int page, int totalPages)
{
    Q_UNUSED(printer);
    Q_UNUSED(painter);
    Q_UNUSED(page);
    Q_UNUSED(totalPages);
    // 页眉页脚功能可以通过 QTextDocument 的页眉页脚框架实现
    // 当前版本简化处理
}
