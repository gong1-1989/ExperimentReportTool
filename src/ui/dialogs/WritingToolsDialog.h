#pragma once
#include "BaseDialog.h"
#include "extension/DocumentTool.h"

class QComboBox;
class QTableWidget;

namespace Ui { class WritingToolsDialog; }

/**
 * @brief 写作工具对话框（F 域）
 *
 * 主程序构造 DocumentContext 传入；工具执行结果在表格中展示；
 * 片段插入通过 exec 返回后由调用方写回编辑器（insertRequested/insertText/insertPos）。
 */
class WritingToolsDialog : public BaseDialog
{
    Q_OBJECT

public:
    explicit WritingToolsDialog(const DocumentContext& ctx, QWidget* parent = nullptr);
    ~WritingToolsDialog() override;

    bool insertRequested() const { return m_insertRequested; }
    QString insertText() const { return m_insertText; }
    int insertPos() const { return m_insertPos; }

private slots:
    void onRun();
    void onInsert();

private:
    Ui::WritingToolsDialog* ui;
    DocumentContext m_ctx;
    QString m_activeToolId;
    QStringList m_snippetNames;   ///< 片段名列表（用于插入选择）
    bool m_insertRequested = false;
    QString m_insertText;
    int m_insertPos = -1;
};
