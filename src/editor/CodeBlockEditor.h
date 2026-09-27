/**
 * @file CodeBlockEditor.h
 * @brief 代码块编辑器头文件
 *
 * 用于在报告中插入代码片段，支持语言选择、复制按钮。
 */

#ifndef CODE_BLOCK_EDITOR_H
#define CODE_BLOCK_EDITOR_H

#include <QPlainTextEdit>
#include <QComboBox>
#include <QPushButton>

#include "editor/BlockEditor.h"

/**
 * @brief 代码块编辑器
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
    QPlainTextEdit* m_codeEdit;
    QComboBox* m_languageCombo;
    QPushButton* m_copyBtn;
    QString m_language;
};

#endif // CODE_BLOCK_EDITOR_H
