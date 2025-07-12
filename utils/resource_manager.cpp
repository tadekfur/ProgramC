#include "resource_manager.h"
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDir>

ResourceManager& ResourceManager::instance() {
    static ResourceManager instance;
    return instance;
}

QString ResourceManager::getConfigPath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QString ResourceManager::getTemporaryPath() const {
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
}

QString ResourceManager::getPdfOutputPath() const {
    QString path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/EtykietyManager/PDFs";
    QDir().mkpath(path);
    return path;
}

bool ResourceManager::isValidEmail(const QString &email) {
    static const QRegularExpression emailRegex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    return emailRegex.match(email).hasMatch();
}

bool ResourceManager::isValidPhoneNumber(const QString &phone) {
    static const QRegularExpression phoneRegex(R"(^[\+]?[0-9\s\-\(\)]{9,15}$)");
    return phoneRegex.match(phone).hasMatch();
}

bool ResourceManager::isValidNIP(const QString &nip) {
    QString cleanNip = nip;
    cleanNip.remove(QRegularExpression(R"([^0-9])"));
    
    if (cleanNip.length() != 10) return false;
    
    // Walidacja NIP algorytmem
    const int weights[] = {6, 5, 7, 2, 3, 4, 5, 6, 7};
    int sum = 0;
    
    for (int i = 0; i < 9; ++i) {
        sum += cleanNip[i].digitValue() * weights[i];
    }
    
    int checksum = sum % 11;
    return (checksum < 10) && (checksum == cleanNip[9].digitValue());
}

QString ResourceManager::formatCurrency(double amount) {
    return QString("%1 zł").arg(amount, 0, 'f', 2);
}

QString ResourceManager::formatDate(const QDate &date) {
    return date.toString("dd.MM.yyyy");
}

QString ResourceManager::sanitizeFileName(const QString &filename) {
    QString sanitized = filename;
    // Usuń niebezpieczne znaki
    sanitized.remove(QRegularExpression(R"([<>:"/\\|?*])"));
    // Ogranicz długość
    if (sanitized.length() > 100) {
        sanitized = sanitized.left(100);
    }
    return sanitized;
}

QString ResourceManager::removeWhitespace(const QString &text) {
    return text.simplified();
}
