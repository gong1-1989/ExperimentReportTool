/**
 * @file ObjectInsertionController.h
 * @brief 对象插入与编辑控制器（表格/图表/图片/公式/分隔线）
 *
 * 自 ReportEditorWindow 提取：负责对象类操作逻辑，
 * 对话框以 m_parent 为父窗口，编辑器操作为唯一数据依赖。
 */

#ifndef OBJECT_INSERTION_CONTROLLER_H
#define OBJECT_INSERTION_CONTROLLER_H

#include <QString>
#include <QWidget>

class ReportEditor;

/**
 * @brief 对象插入与编辑控制器（表格/图表/图片/公式/分隔线）
 */
class ObjectInsertionController
{
public:
    explicit ObjectInsertionController(QWidget* parent, ReportEditor* editor);

    /// 插入数据表对象（新建数据表 + 表格配置）
    void insertTable();
    /// 插入图片对象（选图 + 题注）
    void insertImage();
    /// 插入图表对象（选择数据表 + 图表配置）
    void insertChart();
    /// 插入公式对象（LaTeX 输入）
    void insertFormula();
    /// 插入分隔线
    void insertDivider();
    /// 插入附件卡片（D 域：选文件上传为附件 → 卡片对象）
    void insertAttachmentCard();
    /// 插入音视频引用（D 域：选音视频文件 → 引用对象）
    void insertMediaRef();
    /// 编辑对象（双击对象锚点触发：按类型打开对应编辑对话框）
    void editObject(const QString& objectId);

private:
    QWidget* m_parent;        ///< 对话框父窗口
    ReportEditor* m_editor;   ///< 目标编辑器
};

#endif // OBJECT_INSERTION_CONTROLLER_H
