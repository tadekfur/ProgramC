#pragma once

#include <QString>
#include <QMap>
#include <QVariant>

/**
 * @brief Klasa do zarządzania zasobami aplikacji i czyszczenia kodu
 * Centralizuje często używane funkcje utility
 */
class ResourceManager {
public:
    static ResourceManager& instance();
    
    // Standardowe ścieżki
    QString getConfigPath() const;
    QString getTemporaryPath() const;
    QString getPdfOutputPath() const;
    
    // Walidacja danych
    static bool isValidEmail(const QString &email);
    static bool isValidPhoneNumber(const QString &phone);
    static bool isValidNIP(const QString &nip);
    
    // Formatowanie
    static QString formatCurrency(double amount);
    static QString formatDate(const QDate &date);
    
    // Czyszczenie stringów
    static QString sanitizeFileName(const QString &filename);
    static QString removeWhitespace(const QString &text);
    
private:
    ResourceManager() = default;
    Q_DISABLE_COPY_MOVE(ResourceManager)
};
