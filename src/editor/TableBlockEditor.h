/**
 * @file TableBlockEditor.h
 * @brief 表格块编辑器头文件
 *
 * 用于在报告中插入和编辑数据表格。
 * 支持动态增删行列、单元格编辑、表头设置。
 */

#ifndef TABLE_BLOCK_EDITOR_H
#define TABLE_BLOCK_EDITOR_H

#include <QTableWidget>
#include <QPushButton>

#include "editor/BlockEditor.h"

/**
 * @brief 表格块编辑器
 */
class TableBlockEditor : public BlockEditor
{
    Q_OBJECT

public:
    explicit TableBlockEditor(const ContentBlock& block, QWidget* parent = nullptr);

    QJsonObject blockData() const override;
    void setBlockData(const QJsonObject& data) override;
    BlockType blockType() const override { return BlockType::Table; }
    QString plainText() const override;
    bool isEmpty() const override { return m_table->rowCount() == 0; }

private slots:
    void onAddRow();
    void onAddColumn();
    void onRemoveRow();
    void onRemoveColumn();
    void onCellChanged(int row, int col);

private:
    void setupTable();
    void updateTableHeight();  ///< 根据行数动态调整表格高度

    QTableWidget* m_table;
    QPushButton* m_addRowBtn;
    QPushButton* m_addColBtn;
    QPushButton* m_removeRowBtn;
    QPushButton* m_removeColBtn;
};

#endif // TABLE_BLOCK_EDITOR_H
