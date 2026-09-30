/**
 * @file ObjectInsertionController.cpp
 * @brief 对象插入与编辑控制器实现
 *
 * 自 ReportEditorWindow 迁移（onInsertTable/Image/Chart/Formula/Divider、onObjectEdit），
 * 对话框父窗口由构造参数提供，编辑器为唯一数据依赖，无窗口私有状态耦合。
 */

#include "ObjectInsertionController.h"

#include "editor/ReportEditor.h"
#include "editor/DataTableEditorDialog.h"
#include "chart/ChartConfigDialog.h"
#include "data/repositories/DataTableRepository.h"
#include "core/models/DataTable.h"
#include "ui/UiHelper.h"
#include "core/models/Report.h"
#include "service/AttachmentService.h"
#include "core/models/Attachment.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QFileInfo>

ObjectInsertionController::ObjectInsertionController(QWidget* parent, ReportEditor* editor)
    : m_parent(parent)
    , m_editor(editor)
{
}

void ObjectInsertionController::insertTable()
{
    // 新建数据表并插入表格对象
    DataTable::Ptr table = DataTable::create();
    table->setReportId(m_editor->reportId());   // 归属当前报告（外键约束）
    DataTableEditorDialog dialog(table, m_parent);
    if (dialog.exec() != QDialog::Accepted) return;

    DataTable::Ptr saved = dialog.tableData();
    if (!saved || saved->id() <= 0) return;

    QJsonObject data;
    data["tableId"] = saved->id();
    data["caption"] = saved->name();
    m_editor->insertObject(BlockType::Table, data);
    m_editor->focusDocument();
}

void ObjectInsertionController::insertImage()
{
    const QString filePath = QFileDialog::getOpenFileName(
        m_parent, QObject::tr("选择图片"), QDir::homePath(),
        QObject::tr("图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (filePath.isEmpty()) return;

    bool ok = false;
    const QString caption = QInputDialog::getText(
        m_parent, QObject::tr("图片说明"), QObject::tr("题注（可留空）:"), QLineEdit::Normal,
        QString(), &ok);

    QJsonObject data;
    data["path"] = filePath;
    data["caption"] = ok ? caption : QString();
    m_editor->insertObject(BlockType::Image, data);
    m_editor->focusDocument();
}

void ObjectInsertionController::insertChart()
{
    if (!m_editor) return;

    // 需要先有数据表：列出当前报告的数据表 + 全局数据表供选择
    DataTable::List tables = DataTableRepository::findByReport(m_editor->reportId());
    tables += DataTableRepository::findGlobal();
    if (tables.isEmpty()) {
        UiHelper::warning(m_parent, QObject::tr("插入图表"),
                          QObject::tr("请先在报告中插入并填写数据表，再基于数据表创建图表。"));
        return;
    }

    ChartConfigDialog dialog(tables, ChartConfig(), m_parent);
    if (dialog.exec() != QDialog::Accepted) return;

    ChartConfig config = dialog.config();
    QJsonObject data;
    data["tableId"] = config.dataTableId;
    data["config"] = config.toJson();
    m_editor->insertObject(BlockType::Chart, data);
    m_editor->focusDocument();
}

void ObjectInsertionController::insertFormula()
{
    bool ok = false;
    const QString latex = QInputDialog::getMultiLineText(
        m_parent, QObject::tr("插入公式"), QObject::tr("LaTeX 公式（示例: E=mc^2）:"), QString(), &ok);
    if (!ok) return;

    QJsonObject data;
    data["latex"] = latex;
    m_editor->insertObject(BlockType::Formula, data);
    m_editor->focusDocument();
}

void ObjectInsertionController::insertDivider()
{
    m_editor->insertDividerAtCursor();
    m_editor->focusDocument();
}
void ObjectInsertionController::insertAttachmentCard()
{
    if (!m_editor) return;
    const QString filePath = QFileDialog::getOpenFileName(
        m_parent, QObject::tr("选择附件文件"), QDir::homePath(),
        QObject::tr("所有文件 (*.*)"));
    if (filePath.isEmpty()) return;

    const Attachment::Ptr att = AttachmentService::uploadFile(m_editor->reportId(), filePath);
    if (!att) {
        UiHelper::warning(m_parent, QObject::tr("插入附件"),
                          QObject::tr("附件上传失败，请检查文件是否可读。"));
        return;
    }
    bool ok = false;
    const QString caption = QInputDialog::getText(
        m_parent, QObject::tr("附件说明"), QObject::tr("题注（可留空）:"), QLineEdit::Normal,
        QString(), &ok);

    QJsonObject data;
    data["fileName"] = att->fileName();
    data["filePath"] = att->storedPath();
    data["fileSize"] = att->fileSize();
    data["mimeType"] = att->mimeType();
    data["uploadedAt"] = att->uploadedAt().toString(Qt::ISODate);
    data["caption"] = ok ? caption : QString();
    m_editor->insertObject(BlockType::AttachmentCard, data);
    m_editor->focusDocument();
}

void ObjectInsertionController::insertMediaRef()
{
    if (!m_editor) return;
    const QString filePath = QFileDialog::getOpenFileName(
        m_parent, QObject::tr("选择音视频文件"), QDir::homePath(),
        QObject::tr("媒体文件 (*.mp4 *.avi *.mov *.mkv *.wmv *.flv *.mp3 *.wav *.flac *.m4a *.ogg)"));
    if (filePath.isEmpty()) return;

    const Attachment::Ptr att = AttachmentService::uploadFile(m_editor->reportId(), filePath);
    if (!att) {
        UiHelper::warning(m_parent, QObject::tr("插入音视频"),
                          QObject::tr("媒体文件上传失败，请检查文件是否可读。"));
        return;
    }
    const QString suffix = QFileInfo(att->storedPath()).suffix().toLower();
    const bool isVideo = QStringList{ "mp4", "avi", "mov", "mkv", "wmv", "flv" }.contains(suffix);
    const bool isAudio = QStringList{ "mp3", "wav", "flac", "m4a", "ogg" }.contains(suffix);
    if (!isVideo && !isAudio) {
        UiHelper::warning(m_parent, QObject::tr("插入音视频"),
                          QObject::tr("不支持的文件类型（%1），请选择音视频文件。").arg(suffix));
        AttachmentService::remove(att->id());
        return;
    }

    QJsonObject data;
    data["mediaType"] = isVideo ? QStringLiteral("video") : QStringLiteral("audio");
    data["title"] = att->fileName();
    data["filePath"] = att->storedPath();
    m_editor->insertObject(BlockType::MediaRef, data);
    m_editor->focusDocument();
}


void ObjectInsertionController::editObject(const QString& objectId)
{
    if (!m_editor || objectId.isEmpty()) return;

    ContentBlock object = m_editor->objectById(objectId);
    if (object.id.isEmpty()) {
        // 对象数据缺失（旧版本创建的报告）→ 先自愈补齐占位对象，再继续编辑
        m_editor->healMissingObjects();
        object = m_editor->objectById(objectId);
        if (object.id.isEmpty()) {
            UiHelper::warning(m_parent, QObject::tr("对象数据缺失"),
                              QObject::tr("该对象的引用已失效，无法编辑。"));
            return;
        }
    }

    switch (object.type) {
    case BlockType::Table:
    case BlockType::DataReference: {
        // 编辑数据表；模板占位对象（无 tableId）→ 新建数据表
        const qint64 tableId = static_cast<qint64>(object.data.value("tableId").toDouble());
        DataTable::Ptr table = tableId > 0 ? DataTableRepository::findById(tableId) : nullptr;
        if (!table) {
            table = DataTable::create();
            table->setReportId(m_editor->reportId());   // 归属当前报告（外键约束）
        }
        DataTableEditorDialog dialog(table, m_parent);
        if (dialog.exec() == QDialog::Accepted && dialog.tableData()) {
            QJsonObject data = object.data;
            data["tableId"] = dialog.tableData()->id();
            m_editor->updateObjectData(objectId, data);
        }
        break;
    }
    case BlockType::Chart: {
        // 编辑图表配置；模板占位对象（无数据表）→ 先选择/新建数据表
        qint64 tableId = static_cast<qint64>(object.data.value("tableId").toDouble());
        DataTable::Ptr table = tableId > 0 ? DataTableRepository::findById(tableId) : nullptr;
        if (!table) {
            // 选择数据表：当前报告的数据表 + 全局数据表
            DataTable::List tables = DataTableRepository::findByReport(m_editor->reportId());
            tables += DataTableRepository::findGlobal();
            if (tables.isEmpty()) {
                UiHelper::warning(m_parent, QObject::tr("编辑图表"),
                                  QObject::tr("请先编辑表格对象并填写数据，再配置图表。"));
                break;
            }
            QStringList names;
            QList<qint64> ids;
            for (const DataTable::Ptr& t : tables) {
                names << t->name();
                ids << t->id();
            }
            bool ok = false;
            const QString chosen = QInputDialog::getItem(
                m_parent, QObject::tr("选择数据表"), QObject::tr("数据表:"), names, 0, false, &ok);
            if (!ok) break;
            const int idx = names.indexOf(chosen);
            if (idx < 0) break;
            table = tables.at(idx);
            tableId = ids.at(idx);
        }

        ChartConfig config = ChartConfig::fromJson(object.data.value("config").toObject());
        config.dataTableId = tableId;
        ChartConfigDialog dialog(DataTable::List{table}, config, m_parent);
        if (dialog.exec() == QDialog::Accepted) {
            QJsonObject data = object.data;
            data["tableId"] = tableId;
            data["config"] = dialog.config().toJson();
            m_editor->updateObjectData(objectId, data);
        }
        break;
    }
    case BlockType::Image: {
        bool ok = false;
        const QString caption = QInputDialog::getText(
            m_parent, QObject::tr("图片属性"), QObject::tr("题注:"), QLineEdit::Normal,
            object.data.value("caption").toString(), &ok);
        if (ok) {
            QJsonObject data = object.data;
            data["caption"] = caption;
            m_editor->updateObjectData(objectId, data);
        }
        break;
    }
    case BlockType::Formula: {
        bool ok = false;
        const QString latex = QInputDialog::getMultiLineText(
            m_parent, QObject::tr("编辑公式"), QObject::tr("LaTeX:"),
            object.data.value("latex").toString(), &ok);
        if (ok) {
            QJsonObject data;
            data["latex"] = latex;
            m_editor->updateObjectData(objectId, data);
        }
        break;
    }
    case BlockType::AttachmentCard:
    case BlockType::MediaRef:
        // D 域对象：无可编辑字段（删除可通过删除键/对象菜单）
        UiHelper::info(m_parent, QObject::tr("编辑对象"),
                       QObject::tr("该对象为卡片/引用类型，不支持内容编辑；如需移除请删除对象。"));
        break;
    default:
        break;
    }
}
