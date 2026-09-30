/**
 * @file ReportEditorWindow.h
 * @brief 报告编辑窗口头文件
 *
 * 独立的报告编辑窗口，包含 ReportEditor 组件。
 * 支持菜单栏、工具栏、状态栏，以及保存/导出等操作。
 */

#ifndef REPORT_EDITOR_WINDOW_H
#define REPORT_EDITOR_WINDOW_H

#include <QMainWindow>
#include <QAction>
#include "service/WorkflowService.h"
#include "service/PermissionService.h"
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QToolButton>
#include <QMenu>
#include <QColor>

#include "core/models/Report.h"
#include "core/models/Tag.h"

// 前向声明 UI 类（由 uic 工具从 .ui 文件自动生成）
namespace Ui {
class ReportEditorWindow;
}

// 前向声明
class ReportEditor;
class PluginManager;
class ObjectInsertionController;
class ReportExportController;

/**
 * @brief 报告编辑窗口
 */
class ReportEditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param report 要编辑的报告（为空则新建）
     * @param parent 父窗口
     */
    explicit ReportEditorWindow(const Report::Ptr& report, QWidget* parent = nullptr);
    ~ReportEditorWindow() override;

    /// 获取当前编辑的报告
    Report::Ptr currentReport() const;

    /// 是否有未保存的更改
    bool isModified() const;

    /// 保存报告（公共方法，供主窗口调用）
    bool saveReport();

    /// 设置插件管理器
    void setPluginManager(PluginManager* manager) { m_pluginManager = manager; }

    /// 当前报告标题（供主界面状态栏等查询）
    QString reportTitle() const;

signals:
    /// 报告已保存
    void reportSaved(qint64 reportId);

    /// 窗口关闭
    void windowClosed(qint64 reportId);

protected:
    /// 关闭事件（检查未保存更改）
    void closeEvent(QCloseEvent* event) override;

private slots:
    // 文件操作
    void onNew();
    void onSave();
    void onSubmit();
    void onRecall();
    void onReviewApprove();
    void onReviewReject();
    void onApproveApprove();
    void onApproveReject();
    void onArchive();
    void createWorkflowActions();
    void updateWorkflowActions();
    void refreshRejectComment();
    void onSaveAsTemplate();
    void onWritingTools();  ///< 保存为模板
    void onSaveAs();
    void onExport();
    void onPrint();
    void onPrintPreview();
    void onPageSetup();
    void onVersionHistory();

    // 编辑操作
    void onUndo();
    void onRedo();
    void onFind();
    void onManageAttachments();

    // 格式操作
    void onBold();
    void onItalic();
    void onUnderline();
    void onFontSize(int size);
    void onTextColor();

    /// 生成对齐按钮小图标（三条横线示意）
    QIcon makeAlignIcon(Qt::Alignment align) const;
    void onList(bool numbered);
    void onQuote();
    void onInsertTable();
    void onInsertChart();
    void onInsertFormula();
    void onInsertImage();
    void onInsertDivider();
    void onObjectEdit(const QString& objectId);

    // 视图操作
    void onToggleFullscreen();
    void onZoomIn();
    void onZoomOut();
    void onResetZoom();

    // 编辑器信号
    void onContentChanged();
    void onTitleChanged(const QString& title);
    void onSaveTriggered();
    void onSaveStateChanged(bool saved);

private:
    /// 初始化 UI
    void createActions();
    void initStatusBar();
    void connectSignals();

    /// 更新窗口标题
    void updateWindowTitle();

    /// 更新动作状态
    void updateActionsState();

    /// 显示状态栏消息
    void showStatusMessage(const QString& message, int timeout = 3000);

    /// 手动保存前并发冲突检测：打开后被他人修改则提示（自动保存不检测）
    bool checkConflictBeforeSave();


    // -----------------------------------------------------------------------
    // 成员变量
    // -----------------------------------------------------------------------

    Ui::ReportEditorWindow* ui;          ///< UI 界面对象（从 .ui 文件自动生成）
    ReportEditor* m_editor;              ///< 报告编辑器组件
    Report::Ptr m_report;                ///< 当前报告
    QDateTime m_loadedUpdatedAt;        ///< 打开/重载时的 updatedAt（并发冲突检测基准）
    class PrintManager* m_printManager;  ///< 打印管理器
    PluginManager* m_pluginManager;      ///< 插件管理器
    ObjectInsertionController* m_insertionController;  ///< 对象插入/编辑控制器
    ReportExportController* m_exportController;        ///< 导出/打印控制器

    // 状态栏控件
    QLabel* m_statusSaveLabel;           ///< 保存状态
    QLabel* m_statusWordLabel;           ///< 字数
    QLabel* m_statusBlockLabel;          ///< 块数
    QLabel* m_statusPositionLabel;       ///< 光标位置

    // 动作
    QAction* m_actionSave;
    QAction* m_actionUndo;
    QAction* m_actionRedo;
    QAction* m_actionBold;
    QAction* m_actionItalic;
    QAction* m_actionUnderline;
    QAction* m_actionVersionHistory;     ///< 版本历史
    QAction* m_actionManageAttachments;  ///< 附件管理
    QAction* m_actionAlignLeft;          ///< 左对齐
    QAction* m_actionAlignCenter;        ///< 居中
    QAction* m_actionAlignRight;         ///< 右对齐
    bool m_updatingFormat = false;       ///< 防止行高下拉回显触发保存

    bool m_isNewReport;
    bool m_readOnly = false;   ///< 只读模式（无编辑权限）              ///< 是否为新建报告
    QWidget* m_workflowBar = nullptr;       ///< 顶部工作流信息栏
    QLabel* m_rejectCommentLabel = nullptr;  ///< 退回意见显示
    QAction* m_actionSubmit = nullptr;       ///< 提交报告
    QAction* m_actionRecall = nullptr;       ///< 撤回提交（创建者收回草稿）
    QAction* m_actionReviewApprove = nullptr; ///< 审核通过
    QAction* m_actionReviewReject = nullptr;  ///< 审核退回
    QAction* m_actionApproveApprove = nullptr; ///< 审批通过
    QAction* m_actionApproveReject = nullptr;  ///< 审批退回
    QAction* m_actionArchive = nullptr;         ///< 归档报告（仅已审批可归档）
    QAction* m_actionSaveAsTemplate = nullptr;  ///< 保存为模板
    QAction* m_actionWritingTools = nullptr;    ///< 写作工具（F 域）
    QAction* m_actionInsertAttachmentCard = nullptr;  ///< 插入附件卡片（D 域）
    QAction* m_actionInsertMediaRef = nullptr;        ///< 插入音视频引用（D 域）
    double m_zoomFactor;             ///< 缩放因子
};

#endif // REPORT_EDITOR_WINDOW_H
