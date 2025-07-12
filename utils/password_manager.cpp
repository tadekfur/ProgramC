#include "password_manager.h"
#include <QDebug>
#include <QRegularExpression>
#include <QMessageAuthenticationCode>

QString PasswordManager::hashPassword(const QString& password) {
    if (password.isEmpty()) {
        return QString();
    }
    
    // Generuj sól
    QString salt = generateSalt();
    
    // Hashuj hasło z solą
    QByteArray saltBytes = salt.toUtf8();
    QByteArray hash = pbkdf2(password, saltBytes, HASH_ITERATIONS);
    
    // Zwróć format: salt$hash
    return salt + "$" + hash.toBase64();
}

bool PasswordManager::verifyPassword(const QString& password, const QString& hash) {
    if (password.isEmpty() || hash.isEmpty()) {
        return false;
    }
    
    // Podziel hash na sól i hash
    QStringList parts = hash.split("$");
    if (parts.size() != 2) {
        return false;
    }
    
    QString salt = parts[0];
    QString expectedHash = parts[1];
    
    // Hashuj podane hasło z tą samą solą
    QByteArray saltBytes = salt.toUtf8();
    QByteArray calculatedHash = pbkdf2(password, saltBytes, HASH_ITERATIONS);
    
    // Porównaj hashe
    return calculatedHash.toBase64() == expectedHash;
}

QString PasswordManager::generateSalt() {
    QByteArray salt;
    salt.resize(SALT_LENGTH);
    
    for (int i = 0; i < SALT_LENGTH; ++i) {
        salt[i] = QRandomGenerator::global()->bounded(256);
    }
    
    return salt.toBase64();
}

bool PasswordManager::isPasswordStrong(const QString& password) {
    if (password.length() < 8) {
        return false;
    }
    
    // Sprawdź czy ma małe litery
    QRegularExpression lowerCase("[a-z]");
    if (!lowerCase.match(password).hasMatch()) {
        return false;
    }
    
    // Sprawdź czy ma wielkie litery
    QRegularExpression upperCase("[A-Z]");
    if (!upperCase.match(password).hasMatch()) {
        return false;
    }
    
    // Sprawdź czy ma cyfry
    QRegularExpression digits("[0-9]");
    if (!digits.match(password).hasMatch()) {
        return false;
    }
    
    // Sprawdź czy ma znaki specjalne
    QRegularExpression special("[!@#$%^&*(),.?\":{}|<>]");
    if (!special.match(password).hasMatch()) {
        return false;
    }
    
    return true;
}

QString PasswordManager::generateSecurePassword(int length) {
    const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    QString password;
    
    for (int i = 0; i < length; ++i) {
        int index = QRandomGenerator::global()->bounded(chars.length());
        password.append(chars[index]);
    }
    
    return password;
}

QByteArray PasswordManager::pbkdf2(const QString& password, const QByteArray& salt, int iterations) {
    QByteArray passwordBytes = password.toUtf8();
    QByteArray result;
    
    // Simplified PBKDF2 implementation
    // In production, consider using a proper crypto library like libsodium
    for (int i = 0; i < iterations; ++i) {
        QMessageAuthenticationCode mac(QCryptographicHash::Sha256);
        mac.setKey(passwordBytes);
        mac.addData(salt);
        mac.addData(QByteArray::number(i));
        result = mac.result();
        passwordBytes = result; // Use result as key for next iteration
    }
    
    return result;
}