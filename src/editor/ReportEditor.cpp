/**
 * @file ReportEditor.cpp
 * @brief 报告编辑器主组件实现（连续文档 + 结构化对象）
 */

#include "ReportEditor.h"
#include "ui_ReportEditor.h"
#include "editor/DocumentTextEdit.h"
#include "editor/ObjectPreviewRenderer.h"
#include "editor/AutoSaveManager.h"
#include "core/utils/Logger.h"
#include "core/utils/AppTheme.h"
#include "core/utils/AppDimensions.h"
#include "core/utils/AppConfig.h"
#include "data/repositories/DataTableRepository.h"
#include "core/models/DataTable.h"
#include "service/DataTableService.h"
#include "service/TagService.h"
#include "service/UserService.h"
#include "core/models/User.h"
#include "core/models/Tag.h"
#include "chart/ChartRenderer.h"
#include "chart/ChartConfigDialog.h"

#include <QTextEdit>
#include <QTextCursor>
#include <QTextDocument>
#include <QUrl>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextFragment>
#include <QTextCharFormat>
#include <QTextImageFormat>
#include <QTextList>
#include <QColor>
#include <QTextListFormat>
#include <QScrollBar>
#include <QPainter>
#include <QFont>
#include <QDateTime>
#include <QRegularExpression>
#include <QSet>
#include <QMap>
#include <QJsonArray>
#include <QFile>
#include <QUrl>
#include <QImageReader>

// ===========================================================================
// 构造与析构
// ===========================================================================

ReportEditor::ReportEditor(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ReportEditor)
    , m_modified(false)
    , m_readOnly(false)
    , m_loading(false)
{
    ui->setupUi(this);

    // 连续文档编辑控件：绑定宿主，支持对象锚点
    ui->m_textEdit->setHost(this);

    // 默认样式
    ui->m_textEdit->setStyleSheet(
        "QTextEdit { background: #ffffff; border: 1px solid #e0e0e0;"
        " border-radius: 6px; padding: 12px; font-size: 14px; }");
    ui->m_textEdit->setPlaceholderText(tr("在此输入报告内容…"));

    // 信号连接
    connect(ui->m_textEdit, &DocumentTextEdit::textChanged,
            this, &ReportEditor::onTextChanged);
    connect(ui->m_textEdit, &DocumentTextEdit::cursorPositionChanged,
            this, &ReportEditor::onCursorPositionChanged);
    connect(ui->m_textEdit, &DocumentTextEdit::selectionChanged,
            this, &ReportEditor::onCursorPositionChanged);   // 选中变化也刷新工具栏
    connect(ui->m_textEdit, &DocumentTextEdit::undoAvailable,
            this, &ReportEditor::onUndoAvailableChanged);
    connect(ui->m_textEdit, &DocumentTextEdit::redoAvailable,
            this, &ReportEditor::onRedoAvailableChanged);
    connect(ui->m_textEdit, &DocumentTextEdit::objectDoubleClicked,
            this, &ReportEditor::onDocumentObjectClicked);

    // 标题栏
    connect(ui->m_titleEdit, &QLineEdit::textChanged,
            this, &ReportEditor::onTitleChanged);
    connect(ui->m_statusCombo, &QComboBox::currentIndexChanged,
            this, &ReportEditor::onStatusChanged);
    connect(ui->m_dateEdit, &QDateEdit::dateChanged,
            this, &ReportEditor::onDateChanged);
    connect(ui->m_tagCombo, &QComboBox::currentIndexChanged,
            this, &ReportEditor::on_m_tagCombo_currentIndexChanged);

    // 自动保存
    m_autoSave = new AutoSaveManager(this);
    m_autoSave->setEnabled(AppConfig::instance().autoSaveEnabled());
    m_autoSave->setAutoSaveInterval(AppConfig::instance().autoSaveInterval());
    connect(m_autoSave, &AutoSaveManager::saveTriggered,
            this, &ReportEditor::saveRequested);
    connect(this, &ReportEditor::contentChanged,
            m_autoSave, &AutoSaveManager::notifyContentChanged);

    setModified(false);
    LOG_DEBUG("ReportEditor 初始化完成（连续文档 + 对象模型）");
}

ReportEditor::~ReportEditor()
{
    delete ui;
}

// ===========================================================================
// 报告加载与保存
// ===========================================================================

void ReportEditor::loadReport(const Report::Ptr& report)
{
    if (!report) return;
    m_report = report;
    m_loading = true;

    // 标题与元信息
    ui->m_titleEdit->setText(report->title());
    ui->m_authorEdit->setText(report->author());  // 创建者纯展示（QLabel，不可编辑）

    // 修改者显示（最后修改该报告的用户名）
    if (report->modifiedBy() > 0) {
        const User::Ptr modifier = UserService::getById(report->modifiedBy());
        ui->m_modifiedByLabel->setText(modifier
            ? modifier->displayNameOrUsername() : tr("未知用户"));
    } else {
        ui->m_modifiedByLabel->setText(tr("未分配"));
    }
    if (report->experimentDate().isValid()) {
        ui->m_dateEdit->setDate(report->experimentDate());
    }
    ui->m_statusCombo->setCurrentIndex(static_cast<int>(report->status()));
    populateTags();   // 填充标签下拉

    // 内容：连续文档 + 对象
    syncObjectsFromReport();
    loadDocument();

    m_loading = false;
    m_autoSave->markSaveSuccess();
    setModified(false);
    emit objectCountChanged(m_objects.size());
    refreshFormattingState();   // 加载后按真实光标位置同步工具栏（首段通常是标题）
    LOG_DEBUG(QString("报告已加载到编辑器: id=%1, title='%2'")
                 .arg(report->id()).arg(report->title()));
}

Report::Ptr ReportEditor::saveToReport()
{
    if (!m_report) return nullptr;

    collectDocument();  // 从编辑控件收集 document + 清理孤儿对象

    // 标题与元信息回写（创建者 author 固定为创建时用户名，不在此回写）
    m_report->setTitle(ui->m_titleEdit->text().trimmed());
    m_report->setStatus(static_cast<ReportStatus>(ui->m_statusCombo->currentIndex()));
    m_report->setExperimentDate(ui->m_dateEdit->date());
    m_report->setWordCount(computeWordCount());

    return m_report;
}

QString ReportEditor::reportTitle() const
{
    return ui->m_titleEdit->text().trimmed();
}

// ===========================================================================
// 文档与对象操作
// ===========================================================================

DocumentTextEdit* ReportEditor::textEdit() const
{
    return ui->m_textEdit;
}

QString ReportEditor::insertObject(BlockType type, const QJsonObject& data)
{
    if (m_readOnly) return QString();

    ContentBlock object(type);
    object.id = Report::generateObjectId();
    object.data = data;

    m_objects.append(object);
    refreshObjectPreview(object);
    registerObjectResource(object);

    // 在光标处插入锚点
    const QPixmap preview = m_previewCache.value(
        QString("object://%1/%2").arg(ContentBlock::blockTypeToString(type), object.id));
    const int width = 560;
    const int height = preview.isNull() ? 100 : qBound(60, preview.height(), 400);
    ui->m_textEdit->insertObjectAnchor(
        ContentBlock::blockTypeToString(type), object.id, width, height);

    setModified(true);
    emit contentChanged();
    emit objectCountChanged(m_objects.size());
    return object.id;
}

void ReportEditor::removeObjectById(const QString& objectId)
{
    if (m_readOnly) return;

    // 从文档中删除对应锚点（遍历文档中的图片字符，匹配 object://.../id）
    QTextDocument* doc = ui->m_textEdit->document();
    for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            const QTextCharFormat fmt = fragment.charFormat();
            if (!fmt.isImageFormat()) continue;
            const QString imageName = fmt.toImageFormat().name();
            if (imageName.startsWith("object://") && imageName.endsWith("/" + objectId)) {
                QTextCursor cursor(doc);
                cursor.setPosition(fragment.position());
                cursor.setPosition(fragment.position() + fragment.length(),
                                    QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
                block = doc->begin();  // 重启遍历（文档已变）
                break;
            }
        }
    }

    // 从对象列表移除
    for (int i = m_objects.size() - 1; i >= 0; --i) {
        if (m_objects.at(i).id == objectId) m_objects.removeAt(i);
    }
    m_previewCache.remove(QString("object://%1").arg(objectId));

    setModified(true);
    emit contentChanged();
    emit objectCountChanged(m_objects.size());
}

void ReportEditor::updateObjectData(const QString& objectId, const QJsonObject& data)
{
    for (ContentBlock& object : m_objects) {
        if (object.id == objectId) {
            object.data = data;
            refreshObjectPreview(object);
            registerObjectResource(object);
            ui->m_textEdit->viewport()->update();
            setModified(true);
            emit contentChanged();
            return;
        }
    }
}

void ReportEditor::insertDividerAtCursor()
{
    if (m_readOnly) return;
    QTextCursor cursor = ui->m_textEdit->textCursor();
    cursor.insertHtml("<hr/>");
    setModified(true);
    emit contentChanged();
}

ContentBlock ReportEditor::objectById(const QString& objectId) const
{
    for (const ContentBlock& object : m_objects) {
        if (object.id == objectId) return object;
    }
    return ContentBlock();
}

QPixmap ReportEditor::objectPreviewPixmap(const QString& imageName)
{
    const QPixmap cached = m_previewCache.value(imageName);
    if (!cached.isNull()) return cached;

    // 缓存缺失（如文档加载后对象新增）→ 自动重建预览
    const ContentBlock object = objectFromImageName(imageName);
    if (!object.id.isEmpty()) {
        refreshObjectPreview(object);
        registerObjectResource(object);
        return m_previewCache.value(imageName);
    }
    return cached;
}

/// 将对象预览图预注册到文档资源表（渲染时 QTextDocument 直接从资源表取图，
/// 与资源提供者双保险，确保锚点任何情况下都有图可显）
void ReportEditor::registerObjectResource(const ContentBlock& object)
{
    const QString name = QString("object://%1/%2")
        .arg(ContentBlock::blockTypeToString(object.type), object.id);
    const QPixmap preview = m_previewCache.value(name);
    if (!preview.isNull()) {
        ui->m_textEdit->document()->addResource(
            QTextDocument::ImageResource, QUrl(name), QVariant(preview));
    }
}

// ===========================================================================
// 查找辅助
// ===========================================================================

QStringList ReportEditor::documentParagraphs() const
{
    QStringList paragraphs;
    QTextDocument* doc = ui->m_textEdit->document();
    for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
        const QString text = block.text().trimmed();
        if (!text.isEmpty()) paragraphs.append(text);
    }
    return paragraphs;
}

void ReportEditor::healMissingObjects()
{
    if (!m_report) return;

    // 收集 document 中所有对象锚点（object://<type>/<id>）
    QRegularExpression anchorRe(QStringLiteral("object://([a-z_0-9]+)/([0-9a-fA-F-]+)"));
    QMap<QString, BlockType> anchorTypes;
    QRegularExpressionMatchIterator it = anchorRe.globalMatch(ui->m_textEdit->toHtml());
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        const QString id = m.captured(2);
        if (id.isEmpty()) continue;
        const BlockType t = ContentBlock::blockTypeFromString(m.captured(1));
        // 仅结构化对象类型需要自愈（段落/标题等不是锚点）
        if (t == BlockType::Table || t == BlockType::Image || t == BlockType::Chart
            || t == BlockType::Formula || t == BlockType::DataReference) {
            anchorTypes.insert(id, t);
        }
    }
    if (anchorTypes.isEmpty()) return;

    // 补齐缺失对象
    bool changed = false;
    QSet<QString> existing;
    for (const ContentBlock& o : m_objects) existing.insert(o.id);
    for (auto typeIt = anchorTypes.begin(); typeIt != anchorTypes.end(); ++typeIt) {
        if (!existing.contains(typeIt.key())) {
            ContentBlock obj(typeIt.value());
            obj.id = typeIt.key();
            m_objects.append(obj);
            changed = true;
        }
    }
    if (!changed) return;

    // 刷新预览并注册资源
    m_previewCache.clear();
    for (const ContentBlock& object : m_objects) {
        refreshObjectPreview(object);
        registerObjectResource(object);
    }
    ui->m_textEdit->viewport()->update();

    // 同步回报告，保存时写入，避免数据再次丢失
    if (m_report) m_report->setObjects(m_objects);
}

/// 表格自愈：Table/DataReference 对象未关联数据表时自动创建空表并回填 tableId，
/// 使模板自带/占位表格可直接被图表选择，点开即可编辑
void ReportEditor::healTableObjects()
{
    if (!m_report) return;
    bool changed = false;
    for (ContentBlock& object : m_objects) {
        if (object.type != BlockType::Table && object.type != BlockType::DataReference) continue;
        const qint64 tid = static_cast<qint64>(object.data.value("tableId").toDouble());
        if (tid > 0) continue;
        DataTable::Ptr table = DataTable::create();
        table->setReportId(m_report->id());
        QString caption = object.data.value("caption").toString().trimmed();
        table->setName(caption.isEmpty()
            ? tr("数据表 %1").arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"))
            : caption);
        if (DataTableService::save(table)) {
            object.data["tableId"] = table->id();
            changed = true;
        }
    }
    if (!changed) return;
    for (const ContentBlock& object : m_objects) {
        refreshObjectPreview(object);
        registerObjectResource(object);
    }
    ui->m_textEdit->viewport()->update();
    if (m_report) m_report->setObjects(m_objects);
}

void ReportEditor::scrollToParagraph(int paragraphIndex)
{
    QTextDocument* doc = ui->m_textEdit->document();
    int current = 0;
    for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
        if (!block.text().trimmed().isEmpty()) {
            if (current == paragraphIndex) {
                QTextCursor cursor(block);
                ui->m_textEdit->setTextCursor(cursor);
                ui->m_textEdit->setFocus();
                return;
            }
            ++current;
        }
    }
}

// ===========================================================================
// 编辑操作
// ===========================================================================

void ReportEditor::undo()
{
    if (ui->m_textEdit->document()->isUndoAvailable()) {
        ui->m_textEdit->undo();
    }
}

void ReportEditor::redo()
{
    if (ui->m_textEdit->document()->isRedoAvailable()) {
        ui->m_textEdit->redo();
    }
}

bool ReportEditor::canUndo() const
{
    return ui->m_textEdit->document()->isUndoAvailable();
}

bool ReportEditor::canRedo() const
{
    return ui->m_textEdit->document()->isRedoAvailable();
}

void ReportEditor::focusDocument()
{
    ui->m_textEdit->setFocus();
}

void ReportEditor::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
    ui->m_textEdit->setReadOnly(readOnly);
    ui->m_titleEdit->setReadOnly(readOnly);
    ui->m_statusCombo->setEnabled(!readOnly);
    ui->m_dateEdit->setEnabled(!readOnly);
    ui->m_tagCombo->setEnabled(!readOnly);
}

void ReportEditor::setModified(bool modified)
{
    if (m_modified == modified) return;
    m_modified = modified;
    emit saveStateChanged(!modified);
}

int ReportEditor::wordCount() const
{
    return computeWordCount();
}

// ===========================================================================
// 槽：文档信号
// ===========================================================================

void ReportEditor::onTextChanged()
{
    if (m_loading) return;
    setModified(true);
    emit contentChanged();
    refreshFormattingState();   // 格式操作（加粗/颜色/字号等）后工具栏状态及时刷新
}

void ReportEditor::applyParagraphAlignment(Qt::Alignment align)
{
    if (m_readOnly) return;
    QTextCursor cursor = ui->m_textEdit->textCursor();
    QTextBlockFormat fmt = cursor.blockFormat();
    fmt.setAlignment(align);
    cursor.setBlockFormat(fmt);   // 触发 textChanged → 自动进入撤销栈并标记修改
}

void ReportEditor::applyLineHeight(qreal multiplier)
{
    if (m_readOnly) return;
    if (multiplier < 0.5 || multiplier > 5.0) return;
    QTextCursor cursor = ui->m_textEdit->textCursor();
    // Word 式行为：有选区 → 行高应用到所有选中段；无选区 → 只修改光标所在段。
    // 不要 clearSelection——那会导致拖选多段时只剩光标段被改。
    QTextBlockFormat fmt = cursor.blockFormat();
    fmt.setLineHeight(qRound(multiplier * 100.0),
                      QTextBlockFormat::ProportionalHeight);
    cursor.setBlockFormat(fmt);
}

void ReportEditor::onCursorPositionChanged()
{
    refreshFormattingState();
}

void ReportEditor::refreshFormattingState()
{
    if (m_loading) return;

    QTextCursor cursor = ui->m_textEdit->textCursor();
    const QTextCharFormat fmt = cursor.charFormat();
    const QTextCharFormat blockCharFmt = cursor.blockCharFormat();

    // 字号：显式字号优先，无显式字号回退到编辑器当前字体
    int fontSize = AppTheme::FontSize::Normal;
    if (fmt.fontPointSize() > 0) {
        fontSize = qRound(fmt.fontPointSize());
    } else if (blockCharFmt.fontPointSize() > 0) {
        fontSize = qRound(blockCharFmt.fontPointSize());
    } else {
        fontSize = qRound(ui->m_textEdit->currentFont().pointSizeF());
        if (fontSize <= 0) fontSize = AppTheme::FontSize::Normal;
    }

    // 注意：QTextCursor::blockFormat() 在有选区时返回"选区第一个块"的格式，
    // 导致拖动选择其他段落时读到的是起点段（如第一行 1.75）。
    // 改用 cursor.block()（光标所在块=选区终点），保证显示与当前选中段一致。
    const QTextBlockFormat blockFmt = cursor.block().blockFormat();
    const qreal lineHeight = blockFmt.lineHeightType() == QTextBlockFormat::ProportionalHeight
        ? blockFmt.lineHeight() / 100.0 : 1.0;
    // 不再区分段落样式（标题/正文），headingLevel 恒为 0
    const int headingLevel = 0;
    // 文字颜色
    const QColor textColor = fmt.foreground().color();

    emit formattingStateChanged(
        fontSize,
        fmt.fontWeight() >= QFont::Bold,
        fmt.fontItalic(),
        fmt.fontUnderline(),
        cursor.blockNumber() + 1,
        cursor.positionInBlock() + 1,
        blockFmt.alignment(),
        lineHeight,
        headingLevel,
        textColor);
}

void ReportEditor::onUndoAvailableChanged(bool available)
{
    emit undoAvailableChanged(available);
}

void ReportEditor::onRedoAvailableChanged(bool available)
{
    emit redoAvailableChanged(available);
}

void ReportEditor::onDocumentObjectClicked(const QString& objectId)
{
    if (m_readOnly) return;   // 只读模式下不打开对象编辑窗口
    emit objectDoubleClicked(objectId);
}

// ===========================================================================
// 槽：标题栏
// ===========================================================================

void ReportEditor::onTitleChanged(const QString& title)
{
    if (m_loading) return;
    emit titleChanged(title);
    emit contentChanged();
    setModified(true);
}

void ReportEditor::onStatusChanged(int index)
{
    Q_UNUSED(index);
    if (m_loading) return;
    setModified(true);
    emit contentChanged();
}

void ReportEditor::onDateChanged(const QDate& date)
{
    Q_UNUSED(date);
    if (m_loading) return;
    setModified(true);
    emit contentChanged();
}

void ReportEditor::on_m_tagCombo_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    if (m_loading) return;
    m_tagsDirty = true;
    setModified(true);
    emit contentChanged();
}

void ReportEditor::populateTags()
{
    ui->m_tagCombo->clear();
    ui->m_tagCombo->addItem(tr("（无标签）"), 0);
    const Tag::List allTags = TagService::listAll();
    for (const Tag::Ptr& tag : allTags) {
        ui->m_tagCombo->addItem(tag->name(), tag->id());
    }
    // 选中当前报告已设置的标签（取第一个）
    if (m_report && m_report->id() > 0) {
        const Tag::List reportTags = TagService::findByReport(m_report->id());
        if (!reportTags.isEmpty()) {
            const int idx = ui->m_tagCombo->findData(reportTags.first()->id());
            if (idx >= 0) ui->m_tagCombo->setCurrentIndex(idx);
        }
    }
    m_tagsDirty = false;
}

void ReportEditor::saveReportTags()
{
    if (!m_report || m_report->id() <= 0 || !m_tagsDirty) return;
    const qint64 tagId = ui->m_tagCombo->currentData().toLongLong();
    QList<qint64> ids;
    if (tagId > 0) ids.append(tagId);
    TagService::setReportTags(m_report->id(), ids);
    m_tagsDirty = false;
}

// ===========================================================================
// 槽：工具栏
// ===========================================================================

void ReportEditor::onUndo()
{
    undo();
}

void ReportEditor::onRedo()
{
    redo();
}

// ===========================================================================
// 内部方法
// ===========================================================================

void ReportEditor::syncObjectsFromReport()
{
    m_objects = m_report ? m_report->objects() : QList<ContentBlock>();
    m_previewCache.clear();
    for (const ContentBlock& object : m_objects) {
        refreshObjectPreview(object);
        registerObjectResource(object);
    }
}

void ReportEditor::loadDocument()
{
    m_previewCache.clear();
    syncObjectsFromReport();

    if (m_report && !m_report->document().isEmpty()) {
        ui->m_textEdit->setHtml(m_report->document());
    } else {
        ui->m_textEdit->clear();
    }

    // 锚点对象自愈：补全 document 中缺失的对象（旧版本报告数据丢失时自动恢复）
    healMissingObjects();

    // 表格自愈：Table 对象无数据表引用时自动创建并回填 tableId
    healTableObjects();

    ui->m_textEdit->document()->clearUndoRedoStacks();
}

void ReportEditor::collectDocument()
{
    if (!m_report) return;

    // 从编辑控件收集 HTML
    const QString html = ui->m_textEdit->toHtml();
    m_report->setDocument(html);

    // 清理孤儿对象：文档中不再有锚点的对象移除
    // 单次正则收集文档中全部锚点（避免对每个对象做一次全文 contains，O(n×m) → O(n+m)）
    QSet<QString> anchorsInDoc;
    {
        const QRegularExpression anchorRe(QStringLiteral("object://([a-z_0-9]+)/([0-9a-fA-F-]+)"));
        QRegularExpressionMatchIterator it = anchorRe.globalMatch(html);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            anchorsInDoc.insert(match.captured(0));  // 完整锚点串，与下方构造格式一致
        }
    }

    QList<ContentBlock> kept;
    QList<ContentBlock> removed;
    for (const ContentBlock& object : m_objects) {
        const QString anchor = QString("object://%1/%2")
            .arg(ContentBlock::blockTypeToString(object.type), object.id);
        if (anchorsInDoc.contains(anchor)) {
            kept.append(object);
        } else {
            removed.append(object);
        }
    }
    m_objects = kept;
    m_report->setObjects(m_objects);

    // 联动清理：被移除对象引用的数据表，若文档中无其他对象仍引用则删除
    QSet<qint64> usedTables;
    for (const ContentBlock& object : m_objects) {
        const qint64 tid = static_cast<qint64>(object.data.value("tableId").toDouble());
        if (tid > 0) usedTables.insert(tid);
    }
    for (const ContentBlock& object : removed) {
        if (object.type == BlockType::Table || object.type == BlockType::DataReference
            || object.type == BlockType::Chart) {
            const qint64 tid = static_cast<qint64>(object.data.value("tableId").toDouble());
            if (tid > 0 && !usedTables.contains(tid)) {
                DataTableService::remove(tid);
            }
        }
    }
}

void ReportEditor::refreshObjectPreview(const ContentBlock& object)
{
    const QString key = QString("object://%1/%2")
        .arg(ContentBlock::blockTypeToString(object.type), object.id);

    // 渲染逻辑已提取到 ObjectPreviewRenderer（静态纯渲染，不依赖编辑器实例）
    m_previewCache.insert(key, ObjectPreviewRenderer::render(object));
}

ContentBlock ReportEditor::objectFromImageName(const QString& imageName) const
{
    // object://<type>/<id>
    const int slash = imageName.lastIndexOf('/');
    if (slash < 0) return ContentBlock();
    const QString id = imageName.mid(slash + 1);
    return objectById(id);
}

int ReportEditor::computeWordCount() const
{
    const QString plain = ui->m_textEdit->toPlainText();
    if (plain.isEmpty()) return 0;

    int count = 0;
    QRegularExpression cjkRegex(QStringLiteral("[\u4e00-\u9fff]"));
    auto cjkIt = cjkRegex.globalMatch(plain);
    while (cjkIt.hasNext()) { cjkIt.next(); ++count; }

    QRegularExpression wordRegex(QStringLiteral("[a-zA-Z0-9]+"));
    auto wordIt = wordRegex.globalMatch(plain);
    while (wordIt.hasNext()) { wordIt.next(); ++count; }

    return count;
}

void ReportEditor::updateEmptyLabelVisibility()
{
    // 连续文档无需空提示（QTextEdit 自带 placeholder）
}

// ===========================================================================
// 格式操作（Word 式块级应用：无选区时作用于整个段落）
// 自 ReportEditorWindow 下沉：逻辑归属编辑器，窗口仅负责按钮/对话框 UI
// ===========================================================================

void ReportEditor::setBold(bool bold)
{
    DocumentTextEdit* edit = textEdit();
    if (!edit) return;

    QTextCharFormat fmt = edit->textCursor().charFormat();
    fmt.setFontWeight(bold ? QFont::Bold : QFont::Normal);
    applyCharFormat(fmt);
}

void ReportEditor::setItalic(bool italic)
{
    DocumentTextEdit* edit = textEdit();
    if (!edit) return;

    QTextCharFormat fmt = edit->textCursor().charFormat();
    fmt.setFontItalic(italic);
    applyCharFormat(fmt);
}

void ReportEditor::setUnderline(bool underline)
{
    DocumentTextEdit* edit = textEdit();
    if (!edit) return;

    QTextCharFormat fmt = edit->textCursor().charFormat();
    fmt.setFontUnderline(underline);
    applyCharFormat(fmt);
}

void ReportEditor::setFontSize(int pointSize)
{
    if (pointSize <= 0) return;
    DocumentTextEdit* edit = textEdit();
    if (!edit) return;

    // 从当前格式拷贝（保留加粗/斜体/颜色等），只改字号并清除相对字号调整
    QTextCharFormat fmt = edit->textCursor().charFormat();
    fmt.setFontPointSize(pointSize);
    fmt.clearProperty(QTextFormat::FontSizeAdjustment);    // 清掉 h1 标题等相对字号，否则绝对字号不生效
    applyCharFormat(fmt);
}

void ReportEditor::setTextColor(const QColor& color)
{
    DocumentTextEdit* edit = textEdit();
    if (!edit) return;

    QTextCharFormat fmt = edit->textCursor().charFormat();
    fmt.setForeground(color);
    applyCharFormat(fmt);
}

void ReportEditor::setList(bool numbered)
{
    DocumentTextEdit* edit = textEdit();
    if (!edit) return;

    QTextCursor cursor = edit->textCursor();
    QTextListFormat listFormat;
    listFormat.setStyle(numbered ? QTextListFormat::ListDecimal
                                 : QTextListFormat::ListDisc);
    cursor.createList(listFormat);
    edit->setTextCursor(cursor);
    edit->setFocus();
}

void ReportEditor::setQuote()
{
    DocumentTextEdit* edit = textEdit();
    if (!edit) return;

    QTextCursor cursor = edit->textCursor();
    QTextBlockFormat blockFmt = cursor.blockFormat();
    blockFmt.setIndent(blockFmt.indent() == 0 ? 1 : 0);
    cursor.setBlockFormat(blockFmt);
    edit->setTextCursor(cursor);
    edit->setFocus();
}

void ReportEditor::applyCharFormat(const QTextCharFormat& fmt)
{
    DocumentTextEdit* edit = textEdit();
    if (!edit) return;

    QTextCursor cursor = edit->textCursor();
    if (cursor.hasSelection()) {
        // 有选中：应用到选中文字，保留选区（setCharFormat 替换语义，确保相对字号等被清除）
        cursor.setCharFormat(fmt);
        // 清除块的标题标记（HeadingLevel）：避免 toHtml 输出 h1，重载后相对字号覆盖绝对字号
        clearBlockHeading(cursor);
        edit->setTextCursor(cursor);
    } else {
        // 无选中：应用到整个段落，恢复原光标位置（Word 式）
        // 使用块迭代方式应用，不依赖 QTextCursor::select 在失焦/选区状态下的行为
        const int pos = cursor.position();
        QTextBlock block = cursor.block();
        QTextCursor blockCursor(block);
        blockCursor.setPosition(block.position() + qMax(0, block.length() - 1));
        blockCursor.setPosition(block.position(), QTextCursor::KeepAnchor);
        blockCursor.setCharFormat(fmt);
        clearBlockHeading(blockCursor);
        cursor.setPosition(qMin(pos, cursor.document()->characterCount() - 1));
        edit->setTextCursor(cursor);
    }
    // 强制重绘，确保应用立即可见
    edit->viewport()->update();
    edit->setFocus();
}

// 清除光标所在块的标题标记（HeadingLevel）。Qt 的 h1 等标题块带相对字号（FontSizeAdjustment），
// 会覆盖绝对字号并随 toHtml 序列化导致重载后字号恢复，故应用格式时一并清除。
void ReportEditor::clearBlockHeading(QTextCursor& cursor)
{
    QTextBlockFormat bf = cursor.blockFormat();
    if (bf.property(QTextFormat::HeadingLevel).toInt() != 0) {
        bf.clearProperty(QTextFormat::HeadingLevel);
        cursor.setBlockFormat(bf);
    }
}
