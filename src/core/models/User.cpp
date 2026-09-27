/**
 * @file User.cpp
 * @brief 用户实体实现文件
 */

#include "User.h"
#include <QCryptographicHash>

User::User() = default;
User::~User() = default;

QString User::hashPassword(const QString& password, const QString& salt)
{
    // 使用 SHA256 + 固定盐（简单实现，生产环境建议用 bcrypt/argon2）
    const QString combined = salt + password + "ExperimentReportTool_Salt_2024";
    const QByteArray hash = QCryptographicHash::hash(
        combined.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toHex());
}

bool User::verifyPassword(const QString& password) const
{
    return hashPassword(password) == m_passwordHash;
}
