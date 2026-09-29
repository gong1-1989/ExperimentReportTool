/**
 * @file DocumentTextEdit.h
 * @brief 连续文档编辑控件（QTextEdit 子类）
 *
 * 支持「对象锚点」机制：
 * - 文档中的结构化对象（表格/图表/图片/公式/代码）以图片形式内嵌，
 *   图片的 ImageName 为 "object://<type>/<id>"，由 loadResource 提供预览图。
 * - 点击对象锚点图时发出 objectDoubleClicked(id) 信号，由外层打开编辑对话框。
 * - 保存时 toHtml() 保留 <img src="object://..."> 引用，与 Report::objects 配合。
 */

#ifndef DOCUMENT_TEXT_EDIT_H
#define DOCUMENT_TEXT_EDIT_H

#include <QTextEdit>
#include <QObject>
#include <QUrl>
#include <QVariant>
#include <QPixmap>
#include <QMouseEvent>
#include <QTextCursor>

// 前向声明
class ReportEditor;

class DocumentTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit DocumentTextEdit(QWidget* parent = nullptr);

    /// 绑定宿主编辑器（用于获取对象数据与预览图）
    void setHost(ReportEditor* host) { m_host = host; }

    /**
     * @brief 在光标处插入对象锚点
     * @param objectType 对象类型字符串（如 "table"）
     * @param objectId 对象 ID（如 "obj-xxx"）
     * @param width 锚点显示宽度
     * @param height 锚点显示高度
     */
    void insertObjectAnchor(const QString& objectType,
                            const QString& objectId,
                            int width,
                            int height);

    /// 当前光标处是否命中对象锚点（返回对象 ID，未命中返回空串）
    QString objectIdAtCursor() const;

protected:
    /// 提供对象预览图（object:// 资源）
    QVariant loadResource(int type, const QUrl& name) override;

    /// 点击对象锚点 → 发信号
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

signals:
    /// 点击对象锚点（objectId 非空）
    void objectDoubleClicked(const QString& objectId);

private:
    ReportEditor* m_host = nullptr;
};

#endif // DOCUMENT_TEXT_EDIT_H
