/**
 * @file ExportManager.cpp
 * @brief 导出管理器实现文件
 */

#include "ExportManager.h"
#include "HtmlGenerator.h"
#include "core/utils/Logger.h"
#include "extension/ObjectRegistry.h"
#include "core/utils/AppConfig.h"
#include "print/PrintManager.h"

#include <QTextDocument>
#include <QTextCursor>
#include <QPrinter>
#include <QPrintDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include "ui/UiHelper.h"
#include "chart/ChartRenderer.h"
#include "chart/ChartConfigDialog.h"
#include "data/repositories/DataTableRepository.h"
#include "core/models/DataTable.h"
#include <QFileDialog>
#include <QDateTime>
#include <QTextList>
#include <QTextTable>
#include <QBuffer>
#include <QImage>
#include <QPixmap>
#include <QXmlStreamWriter>
#include <QMimeDatabase>
#include <QMimeType>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>

// ===========================================================================
// 构造与析构
// ===========================================================================

ExportManager::ExportManager()
    : m_htmlGenerator(new HtmlGenerator())
{
}

ExportManager::~ExportManager()
{
    delete m_htmlGenerator;
}

// ===========================================================================
// 导出入口
// ===========================================================================

bool ExportManager::exportReport(const Report::Ptr& report,
                                  const ExportConfig& config,
                                  QWidget* parent)
{
    if (!report) {
        UiHelper::error(parent, QObject::tr("导出失败"), QObject::tr("报告为空"));
        return false;
    }

    if (config.filePath.isEmpty()) {
        UiHelper::error(parent, QObject::tr("导出失败"), QObject::tr("输出路径为空"));
        return false;
    }

    LOG_DEBUG(QString("开始导出: 格式=%1 路径=%2")
        .arg(static_cast<int>(config.format))
        .arg(config.filePath));

    // 确保输出目录存在
    QDir().mkpath(QFileInfo(config.filePath).absolutePath());

    bool success = false;
    switch (config.format) {
    case ExportFormat::Pdf:
        success = exportToPdf(report, config, parent);
        break;
    case ExportFormat::Html:
        success = exportToHtml(report, config, parent);
        break;
    case ExportFormat::Word:
        success = exportToWord(report, config, parent);
        break;
    case ExportFormat::Text:
        success = exportToText(report, config, parent);
        break;
    }

    if (success) {
        LOG_INFO(QString("报告已导出: %1").arg(config.filePath));
    } else {
        LOG_ERROR(QString("报告导出失败: %1").arg(config.filePath));
    }

    return success;
}

bool ExportManager::exportReport(const Report::Ptr& report,
                                  ExportFormat format,
                                  const QString& filePath,
                                  QWidget* parent)
{
    ExportConfig config;
    config.format = format;
    config.filePath = filePath;
    return exportReport(report, config, parent);
}

// ===========================================================================
// PDF 导出
// ===========================================================================

bool ExportManager::exportToPdf(const Report::Ptr& report,
                                  const ExportConfig& config,
                                  QWidget* parent)
{
    // 直接复用打印预览的渲染逻辑，确保 PDF 与打印预览显示一致
    PrintManager printManager;
    PrintConfig printConfig = printManager.currentConfig();
    printConfig.includeTitle = config.includeTitle;
    printConfig.includeMeta = config.includeMeta;
    printConfig.includeTableOfContents = config.includeTableOfContents;
    printManager.setConfig(printConfig);

    return printManager.exportToPdf(report, config.filePath, parent);
}

// ===========================================================================
// HTML 导出
// ===========================================================================

bool ExportManager::exportToHtml(const Report::Ptr& report,
                                   const ExportConfig& config,
                                   QWidget* parent)
{
    Q_UNUSED(parent);

    const QString html = m_htmlGenerator->generate(report, config);

    QFile file(config.filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    // Qt6 中 QTextStream 默认使用 UTF-8 编码，无需调用 setCodec
    stream << html;
    file.close();

    return true;
}

// ===========================================================================
// Word 导出（基于 HTML 的 .docx 简化实现）
// ===========================================================================


/**
 * Word 格式说明：当前实现生成 HTML 内容并以 .doc 扩展名保存。
 * Microsoft Word / WPS 均可正常打开 HTML 格式的 .doc 文件。
 * 图片以 base64 data URI 内嵌（Word 2016+ / WPS 支持显示），
 * 不生成额外的 images/ 文件夹。
 * 如需真正的 .docx（OOXML）格式，后续可引入 QTextDocument::exportToOdf
 * 或第三方库（如 libdocx）。
 */
bool ExportManager::exportToWord(const Report::Ptr& report,
                                   const ExportConfig& config,
                                   QWidget* parent)
{
    Q_UNUSED(parent);

    // 简化实现：生成 Word 可以打开的 HTML 文件，扩展名必须用 .doc
    // （Word 对 .docx 按 OOXML 解析，纯 HTML 内容会报"文件已损坏"）
    // 完整的 .docx 需要 OOXML 格式，后续可以用 libdocx 或 pandoc
    QString outPath = config.filePath;
    if (outPath.endsWith(QStringLiteral(".docx"), Qt::CaseInsensitive)) {
        outPath = outPath.left(outPath.size() - 5) + QStringLiteral(".doc");
    }
    QString html = m_htmlGenerator->generate(report, config);

    // 图片以 base64 data URI 直接内嵌 HTML（Word 2016+/WPS 支持 data URI 图片显示）
    // 不生成 images/ 文件夹，避免导出时产生额外文件
    // 包装为 Word 兼容的 HTML（添加 MSO 命名空间）
    const QString wordHtml = QString(
        "<html xmlns:o=\"urn:schemas-microsoft-com:office:office\" "
        "xmlns:w=\"urn:schemas-microsoft-com:office:word\" "
        "xmlns=\"http://www.w3.org/TR/REC-html40\">"
        "<head><meta charset=\"utf-8\">"
        "<!--[if gte mso 9]><xml><w:WordDocument>"
        "<w:View>Print</w:View><w:Zoom>100</w:Zoom>"
        "</w:WordDocument></xml><![endif]-->"
        "</head><body>%1</body></html>"
    ).arg(html.section("<body>", 1).section("</body>", 0, 0));

    // Word 的 HTML 解析不支持 <video>/<audio> 标签（打开会报错），
    // 删除播放器标签块，保留标题行（音视频引用仍以文字形式呈现）
    {
        static const QRegularExpression mediaTagRe(
            QStringLiteral("<(video|audio)[^>]*>.*?</\\1>"),
            QRegularExpression::DotMatchesEverythingOption
                | QRegularExpression::CaseInsensitiveOption);
        QString w = wordHtml;
        w.replace(mediaTagRe, QString());
        QFile file(outPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }

        QTextStream stream(&file);
        // Qt6 中 QTextStream 默认使用 UTF-8 编码，无需调用 setCodec
        stream << w;
        file.close();

        return true;
    }
}

// ===========================================================================
// 纯文本导出
// ===========================================================================

bool ExportManager::exportToText(const Report::Ptr& report,
                                   const ExportConfig& config,
                                   QWidget* parent)
{
    Q_UNUSED(parent);

    QString text;

    if (config.includeTitle) {
        text += report->title() + "\n";
        text += QString(report->title().length(), '=') + "\n\n";
    }

    if (config.includeMeta) {
        text += QString("创建者: %1\n").arg(report->author());
        text += QString("实验日期: %1\n").arg(report->experimentDate().toString("yyyy-MM-dd"));
        text += QString("创建时间: %1\n\n").arg(report->createdAt().toString("yyyy-MM-dd hh:mm"));
    }

    // 连续文档纯文本 + 对象占位
    {
        QTextDocument doc;
        doc.setHtml(report->document());
        text += doc.toPlainText() + "\n\n";
    }
    // 对象占位（按插入顺序）
    const QList<ContentBlock>& objects = report->objects();
    for (const ContentBlock& object : objects) {
        switch (object.type) {
        case BlockType::Image:
            text += QString("[图片: %1]\n\n")
                        .arg(object.data.value("caption").toString());
            break;
        case BlockType::Table:
        case BlockType::DataReference:
            text += QString("[表格: %1]\n\n")
                        .arg(object.data.value("caption").toString());
            break;
        case BlockType::Chart:
            text += QString("[图表: %1]\n\n")
                        .arg(object.data.value("config").toObject().value("title").toString());
            break;
        case BlockType::Formula:
            text += QString("[公式: %1]\n\n")
                        .arg(object.data.value("latex").toString());
            break;

        case BlockType::AttachmentCard:
        case BlockType::MediaRef: {
            // D 域：纯文本摘要（附件卡片 / 音视频引用）
            const QString typeId = (object.type == BlockType::AttachmentCard)
                                       ? QStringLiteral("attachment_card")
                                       : QStringLiteral("media_ref");
            DocumentObjectProviderPtr provider = ObjectRegistry::instance().providerById(typeId);
            if (provider) {
                const QString t = provider->renderText(object.data.toVariantMap());
                if (!t.isEmpty()) text += t + QStringLiteral("\n\n");
            }
            break;
        }
        default:
            break;
        }
    }

    QFile file(config.filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    // Qt6 中 QTextStream 默认使用 UTF-8 编码，无需调用 setCodec
    stream << text;
    file.close();

    return true;
}

// ===========================================================================
// 报告转 HTML
// ===========================================================================


/**
 * @brief 从 HTML 文档中提取纯文本（用于目录等需要纯文本的场景）
 *
 * 使用 QTextDocument 解析 HTML 并提取纯文本内容。
 *
 * @param html HTML 文档或片段
 * @return 纯文本内容
 */
// 格式工具方法
// ===========================================================================

QString ExportManager::formatFilter(ExportFormat format)
{
    switch (format) {
    case ExportFormat::Pdf:  return QObject::tr("PDF 文件 (*.pdf)");
    case ExportFormat::Html: return QObject::tr("HTML 文件 (*.html *.htm)");
    case ExportFormat::Word: return QObject::tr("Word 文档 (*.doc)");
    case ExportFormat::Text: return QObject::tr("纯文本文件 (*.txt)");
    }
    return QObject::tr("所有文件 (*)");
}

QString ExportManager::formatExtension(ExportFormat format)
{
    switch (format) {
    case ExportFormat::Pdf:  return ".pdf";
    case ExportFormat::Html: return ".html";
    case ExportFormat::Word: return ".doc";
    case ExportFormat::Text: return ".txt";
    }
    return ".txt";
}

QList<ExportFormat> ExportManager::supportedFormats()
{
    return { ExportFormat::Pdf, ExportFormat::Html, ExportFormat::Word, ExportFormat::Text };
}

QString ExportManager::formatDisplayName(ExportFormat format)
{
    switch (format) {
    case ExportFormat::Pdf:  return QObject::tr("PDF");
    case ExportFormat::Html: return QObject::tr("HTML");
    case ExportFormat::Word: return QObject::tr("Word");
    case ExportFormat::Text: return QObject::tr("纯文本");
    }
    return QString();
}

QPair<QString, ExportFormat> ExportManager::getSaveFilePath(QWidget* parent,
                                                                const QString& defaultName)
{
    // 构建过滤器
    QStringList filters;
    for (ExportFormat fmt : supportedFormats()) {
        filters.append(formatFilter(fmt));
    }
    const QString filter = filters.join(";;");

    // 使用配置中的默认导出路径
    const QString defaultPath = AppConfig::instance().defaultExportPath();
    const QString fullDefaultName = defaultPath.isEmpty()
        ? defaultName
        : QDir(defaultPath).filePath(defaultName);

    const QString filePath = QFileDialog::getSaveFileName(
        parent, QObject::tr("导出报告"), fullDefaultName, filter);

    if (filePath.isEmpty()) {
        return qMakePair(QString(), ExportFormat::Pdf);
    }

    // 根据扩展名判断格式
    const QString ext = QFileInfo(filePath).suffix().toLower();
    ExportFormat format = ExportFormat::Pdf;
    if (ext == "html" || ext == "htm") format = ExportFormat::Html;
    else if (ext == "doc" || ext == "docx") format = ExportFormat::Word;
    else if (ext == "txt") format = ExportFormat::Text;

    return qMakePair(filePath, format);
}
