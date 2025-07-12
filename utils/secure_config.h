#pragma once

#include <QString>
#include <QProcessEnvironment>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>

class SecureConfig {
public:
    static SecureConfig& instance();
    
    // Database configuration
    QString getDatabaseHost() const;
    int getDatabasePort() const;
    QString getDatabaseName() const;
    QString getDatabaseUser() const;
    QString getDatabasePassword() const;
    QString getDatabaseType() const; // "QPSQL" or "QSQLITE"
    
    // Security settings
    QString getSecretKey() const;
    int getSessionTimeout() const;
    
private:
    SecureConfig() = default;
    QString getEnvOrDefault(const QString& envVar, const QString& defaultValue) const;
    QString getSecureStoragePath() const;
    
    mutable QSettings* m_settings = nullptr;
    QSettings* getSettings() const;
};