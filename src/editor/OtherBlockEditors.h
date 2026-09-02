/**
 * @file OtherBlockEditors.h
 * @brief 其他块类型编辑器头文件
 *
 * 包含表格、图片、代码块、分割线等非文本块类型的编辑器。
 * 这些块类型相对独立，集中在一个文件中管理。
 */

#ifndef OTHER_BLOCK_EDITORS_H
#define OTHER_BLOCK_EDITORS_H

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QHeaderView>
#include <QLineEdit>

#include "chart/ChartConfigDialog.h"
#include "chart/ChartRenderer.h"
#include "editor/BlockEditor.h"

// ===========================================================================
// 表格块编辑器
// ===========================================================================

/**
 * @brief 表格块编辑器
 *
 * 用于在报告中插入和编辑数据表格。
 * 支持动态增删行列、单元格编辑、表头设置。
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

    QTableWidget* m_table;
    QPushButton* m_addRowBtn;
    QPushButton* m_addColBtn;
    QPushButton* m_removeRowBtn;
    QPushButton* m_removeColBtn;
};

// ===========================================================================
// 图片块编辑器
// ===========================================================================

/**
 * @brief 图片块编辑器
 *
 * 用于在报告中插入图片，支持从文件选择、拖拽上传、
 * 图片说明文字、尺寸调整。
 */
class ImageBlockEditor : public BlockEditor
{
    Q_OBJECT

public:
    explicit ImageBlockEditor(const ContentBlock& block, QWidget* parent = nullptr);

    QJsonObject blockData() const override;
    void setBlockData(const QJsonObject& data) override;
    BlockType blockType() const override { return BlockType::Image; }
    QString plainText() const override { return m_captionEdit->text(); }
    bool isEmpty() const override { return m_imagePath.isEmpty(); }

private slots:
    void onSelectImage();
    void onCaptionChanged();

private:
    void updateImageDisplay();

    QLabel* m_imageLabel;
    QLineEdit* m_captionEdit;
    QPushButton* m_selectBtn;
    QString m_imagePath;
    int m_displayWidth;
};

// ===========================================================================
// 代码块编辑器
// ===========================================================================

/**
 * @brief 代码块编辑器
 *
 * 用于在报告中插入代码片段，支持语法高亮（基础版）、
 * 语言选择、复制按钮。
 */
class CodeBlockEditor : public BlockEditor
{
    Q_OBJECT

public:
    explicit CodeBlockEditor(const ContentBlock& block, QWidget* parent = nullptr);

    QJsonObject blockData() const override;
    void setBlockData(const QJsonObject& data) override;
    BlockType blockType() const override { return BlockType::CodeBlock; }
    QString plainText() const override { return m_codeEdit->toPlainText(); }
    bool isEmpty() const override { return m_codeEdit->toPlainText().isEmpty(); }
    void setFocusToEditor() override { m_codeEdit->setFocus(); }

private slots:
    void onCopyCode();
    void onLanguageChanged(int index);

private:
    void setupHighlighter();

    QPlainTextEdit* m_codeEdit;
    QComboBox* m_languageCombo;
    QPushButton* m_copyBtn;
    QString m_language;
};

// ===========================================================================
// 分割线块编辑器
// ===========================================================================

/**
 * @brief 分割线块编辑器
 *
 * 简单的分割线，用于分隔报告内容。
 * 不可编辑，仅显示一条水平线。
 */
class DividerBlockEditor : public BlockEditor
{
    Q_OBJECT

public:
    explicit DividerBlockEditor(const ContentBlock& block, QWidget* parent = nullptr);

    QJsonObject blockData() const override { return QJsonObject(); }
    void setBlockData(const QJsonObject& data) override { Q_UNUSED(data); }
    BlockType blockType() const override { return BlockType::Divider; }
    bool isEmpty() const override { return false; }

private:
    QFrame* m_divider;
};

// ===========================================================================
// 图表块编辑器（占位）
// ===========================================================================

/**
 * @brief 图表块编辑器（占位，P0-3 实现完整功能）
 *
 * 当前版本显示占位信息，P0-3 将实现基于数据表的图表生成。
 */
class ChartBlockEditor : public BlockEditor
{
    Q_OBJECT

public:
    explicit ChartBlockEditor(const ContentBlock& block, QWidget* parent = nullptr);
    ~ChartBlockEditor() override;

    QJsonObject blockData() const override;
    void setBlockData(const QJsonObject& data) override;
    BlockType blockType() const override { return BlockType::Chart; }
    bool isEmpty() const override { return m_config.dataTableId <= 0; }

    /// 设置报告 ID（用于查找该报告下的数据表）
    Q_INVOKABLE void setReportId(qint64 reportId) { m_reportId = reportId; }

private slots:
    void onConfigureChart();
    void onEditData();

private:
    void setupChartArea();
    void renderChart();

    QWidget* m_chartContainer;       ///< 图表容器
    QLabel* m_placeholderLabel;       ///< 占位标签（未配置时显示）
    QPushButton* m_configBtn;         ///< 配置按钮
    QPushButton* m_editDataBtn;       ///< 编辑数据按钮
    ChartRenderer* m_renderer;  ///< 图表渲染器
    ChartConfig m_config;       ///< 图表配置
    qint64 m_reportId;                 ///< 所属报告 ID
};

// ===========================================================================
// 块编辑器工厂
// ===========================================================================

/**
 * @brief 块编辑器工厂
 *
 * 根据块类型创建对应的 BlockEditor 实例。
 * 新增块类型时只需在此工厂中注册。
 */
class BlockEditorFactory
{
public:
    /**
     * @brief 创建块编辑器
     * @param block 内容块数据
     * @param parent 父窗口
     * @return 块编辑器实例
     */
    static BlockEditor* createEditor(const ContentBlock& block, QWidget* parent = nullptr);

    /**
     * @brief 获取所有支持的块类型列表
     * @return 块类型列表
     */
    static QList<BlockType> supportedTypes();

    /**
     * @brief 获取块类型的显示名称
     * @param type 块类型
     * @return 显示名称
     */
    static QString typeDisplayName(BlockType type);
};

#endif // OTHER_BLOCK_EDITORS_H
