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
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>

#include "core/models/Report.h"

// 前向声明
class ReportEditor;

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

    void applyFormatToCurrentBlock(const QString& format);

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
    void onSaveAs();
    void onExport();
    void onPrint();
    void onPrintPreview();
    void onPageSetup();
    void onVersionHistory();
    void onEditTags();
    void onManageTags();
    void onManageAttachments();

    // 编辑操作
    void onUndo();
    void onRedo();
    void onFind();

    // 格式操作
    void onBold();
    void onItalic();
    void onUnderline();
    void onHeading(int level);
    void onList(bool numbered);
    void onQuote();
    void onCodeBlock();
    void onInsertTable();
    void onInsertImage();
    void onInsertDivider();

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
    void setupUi();
    void createActions();
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void connectSignals();

    /// 保存报告
    bool saveReport();

    /// 更新窗口标题
    void updateWindowTitle();

    /// 更新动作状态
    void updateActionsState();

    void showStatusMessage(const QString &msg, int timeout = 3000);

    // -----------------------------------------------------------------------
    // 成员变量
    // -----------------------------------------------------------------------

    ReportEditor* m_editor;          ///< 报告编辑器组件
    Report::Ptr m_report;            ///< 当前报告
    class PrintManager* m_printManager;  ///< 打印管理器

    // 状态栏标签
    QLabel* m_statusSaveLabel;       ///< 保存状态
    QLabel* m_statusWordLabel;       ///< 字数
    QLabel* m_statusPositionLabel;   ///< 光标位置

    // 动作
    QAction* m_actionSave;
    QAction* m_actionUndo;
    QAction* m_actionRedo;
    QAction* m_actionBold;
    QAction* m_actionItalic;
    QAction* m_actionUnderline;

    bool m_isNewReport;              ///< 是否为新建报告
    double m_zoomFactor;             ///< 缩放因子
};

#endif // REPORT_EDITOR_WINDOW_H
