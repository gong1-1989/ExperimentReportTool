/**
 * @file User.cpp
 * @brief 用户实体实现文件
 */

#include "User.h"

#include <QCryptographicHash>
#include <QRandomGenerator>

namespace {
// PBKDF2 风格迭代次数（本地桌面应用，1 万次兼顾安全与响应速度）
constexpr int kHashIterations = 10000;
// 旧版固定盐（仅用于验证存量账户，新密码一律用随机盐）
constexpr auto kLegacySalt = "ExperimentReportTool_Salt_2024";

QByteArray randomSalt()
{
    QByteArray bytes;
    bytes.resize(16);
    for (int i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<char>(QRandomGenerator::system()->bounded(256));
    }
    return bytes;
}

// 迭代 SHA256：digest = SHA256(SHA256(...(data)...)) 共 iterations 次
QByteArray iterateSha256(const QByteArray& data, int iterations)
{
    QByteArray digest = data;
    for (int i = 0; i < iterations; ++i) {
        digest = QCryptographicHash::hash(digest, QCryptographicHash::Sha256);
    }
    return digest;
}

QString sha256Hex(const QByteArray& data)
{
    return QString::fromLatin1(QCryptographicHash::hash(
        data, QCryptographicHash::Sha256).toHex());
}
}

User::User() = default;
User::~User() = default;

QString User::hashPassword(const QString& password, const QString& salt)
{
    // 随机 16 字节盐（未传入时）；传入 salt 供迁移/测试复用
    const QByteArray saltBytes = salt.isEmpty()
        ? randomSalt()
        : QByteArray::fromHex(salt.toLatin1());

    // 存储自描述格式: pbkdf2$迭代次数$盐(hex)$哈希(hex)
    return QStringLiteral("pbkdf2$%1$%2$%3")
        .arg(kHashIterations)
        .arg(QString::fromLatin1(saltBytes.toHex()))
        .arg(QString::fromLatin1(iterateSha256(saltBytes + password.toUtf8(), kHashIterations).toHex()));
}

bool User::verifyPassword(const QString& password) const
{
    // 兼容旧格式：无 pbkdf2$ 前缀 = 历史版本 SHA256(salt空 + password + 固定盐)
    if (!m_passwordHash.startsWith(QStringLiteral("pbkdf2$"))) {
        return sha256Hex((password + QString::fromLatin1(kLegacySalt)).toUtf8())
               == m_passwordHash;
    }

    // 新格式解析
    const QStringList parts = m_passwordHash.split(QLatin1Char('$'));
    if (parts.size() != 4) return false;
    bool ok = false;
    const int iterations = parts[1].toInt(&ok);
    if (!ok || iterations <= 0) return false;

    const QByteArray saltBytes = QByteArray::fromHex(parts[2].toLatin1());
    const QByteArray digest = iterateSha256(saltBytes + password.toUtf8(), iterations);
    return QString::fromLatin1(digest.toHex()) == parts[3];
}

bool User::usesLegacyHash() const
{
    return !m_passwordHash.startsWith(QStringLiteral("pbkdf2$"));
}
