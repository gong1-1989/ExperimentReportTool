/**
 * @file AuditLogDialog.h
 * @brief 审计日志查看对话框
 */

#ifndef AUDIT_LOG_DIALOG_H
#define AUDIT_LOG_DIALOG_H

#include "BaseDialog.h"
#include <QTableWidget>

class AuditLogDialog : public BaseDialog
{
    Q_OBJECT

public:
    explicit AuditLogDialog(QWidget* parent = nullptr);

private:
    void refresh();

    QTableWidget* m_table;
};

#endif // AUDIT_LOG_DIALOG_H
