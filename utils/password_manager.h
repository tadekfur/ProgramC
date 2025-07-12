#pragma once

#include <QString>
#include <QCryptographicHash>
#include <QRandomGenerator>

class PasswordManager {
public:
    // Hashuje hasło z solą
    static QString hashPassword(const QString& password);
    
    // Weryfikuje hasło
    static bool verifyPassword(const QString& password, const QString& hash);
    
    // Generuje bezpieczną sól
    static QString generateSalt();
    
    // Sprawdza siłę hasła
    static bool isPasswordStrong(const QString& password);
    
    // Generuje bezpieczne hasło
    static QString generateSecurePassword(int length = 16);
    
private:
    static const int SALT_LENGTH = 32;
    static const int HASH_ITERATIONS = 10000;
    
    // PBKDF2 implementation using QCryptographicHash
    static QByteArray pbkdf2(const QString& password, const QByteArray& salt, int iterations);
};