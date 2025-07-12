#include "secure_config.h"
#include <QDebug>
#include <QCryptographicHash>
#include <QUuid>

SecureConfig& SecureConfig::instance() {
    static SecureConfig instance;
    return instance;
}

QString SecureConfig::getDatabaseHost() const {
    return getEnvOrDefault("DB_HOST", "localhost");
}

int SecureConfig::getDatabasePort() const {
    return getEnvOrDefault("DB_PORT", "5432").toInt();
}

QString SecureConfig::getDatabaseName() const {
    return getEnvOrDefault("DB_NAME", "etykiety_db");
}

QString SecureConfig::getDatabaseUser() const {
    return getEnvOrDefault("DB_USER", "postgres");
}

QString SecureConfig::getDatabasePassword() const {
    // Nigdy nie używaj domyślnego hasła w produkcji
    QString password = getEnvOrDefault("DB_PASSWORD", "");
    if (password.isEmpty()) {
        qWarning() << "DB_PASSWORD environment variable not set! Using default for development.";
        // TYMCZASOWO: zwróć domyślne hasło dla PostgreSQL
        return "tadek123";
    }
    return password;
}

QString SecureConfig::getDatabaseType() const {
    return getEnvOrDefault("DB_TYPE", "QPSQL");
}

QString SecureConfig::getSecretKey() const {
    QString key = getEnvOrDefault("APP_SECRET_KEY", "");
    if (key.isEmpty()) {
        // Generuj unikalny klucz dla tej instalacji
        QSettings* settings = getSettings();
        key = settings->value("app_secret_key").toString();
        if (key.isEmpty()) {
            key = QUuid::createUuid().toString();
            settings->setValue("app_secret_key", key);
            qDebug() << "Generated new secret key";
        }
    }
    return key;
}

int SecureConfig::getSessionTimeout() const {
    return getEnvOrDefault("SESSION_TIMEOUT", "3600").toInt(); // 1 hour default
}

QString SecureConfig::getEnvOrDefault(const QString& envVar, const QString& defaultValue) const {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    return env.value(envVar, defaultValue);
}

QString SecureConfig::getSecureStoragePath() const {
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    if (!dir.exists()) {
        dir.mkpath(appDataPath);
    }
    return appDataPath + "/secure_settings.ini";
}

QSettings* SecureConfig::getSettings() const {
    if (!m_settings) {
        m_settings = new QSettings(getSecureStoragePath(), QSettings::IniFormat);
    }
    return m_settings;
}