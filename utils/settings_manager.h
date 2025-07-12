#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QVariant>
#include <QMutex>
#include <memory>

/**
 * @brief Singleton class for managing application settings
 * 
 * This class provides centralized access to application configuration
 * with thread-safe operations and validation.
 */
class SettingsManager : public QObject {
    Q_OBJECT

public:
    enum class DatabaseType {
        SQLite,
        MySQL,
        PostgreSQL
    };

    enum class LogLevel {
        Debug,
        Info,
        Warning,
        Error,
        Critical
    };

    // Singleton access
    static SettingsManager& instance();
    
    // Prevent copying and moving
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;
    SettingsManager(SettingsManager&&) = delete;
    SettingsManager& operator=(SettingsManager&&) = delete;

    // Application settings
    QString getApplicationName() const;
    QString getApplicationVersion() const;
    QString getOrganizationName() const;
    
    // Database settings
    DatabaseType getDatabaseType() const;
    void setDatabaseType(DatabaseType type);
    QString getDatabasePath() const;
    void setDatabasePath(const QString &path);
    QString getDatabaseHost() const;
    void setDatabaseHost(const QString &host);
    int getDatabasePort() const;
    void setDatabasePort(int port);
    QString getDatabaseName() const;
    void setDatabaseName(const QString &name);
    QString getDatabaseUser() const;
    void setDatabaseUser(const QString &user);
    QString getDatabasePassword() const;
    void setDatabasePassword(const QString &password);
    
    // Network settings
    int getNotificationServerPort() const;
    void setNotificationServerPort(int port);
    int getUdpListenerPort() const;
    void setUdpListenerPort(int port);
    
    // UI settings
    QString getStyleSheetPath() const;
    void setStyleSheetPath(const QString &path);
    bool getMaximizeOnStartup() const;
    void setMaximizeOnStartup(bool maximize);
    QString getLastOpenedPage() const;
    void setLastOpenedPage(const QString &page);
    
    // Logging settings
    LogLevel getLogLevel() const;
    void setLogLevel(LogLevel level);
    QString getLogFilePath() const;
    void setLogFilePath(const QString &path);
    bool getLogToFile() const;
    void setLogToFile(bool enable);
    bool getLogToConsole() const;
    void setLogToConsole(bool enable);
    
    // PDF generation settings
    QString getPdfOutputDirectory() const;
    void setPdfOutputDirectory(const QString &path);
    QString getDefaultPdfTemplate() const;
    void setDefaultPdfTemplate(const QString &templateName);
    
    // Generic settings access
    QVariant getValue(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);
    bool contains(const QString &key) const;
    void remove(const QString &key);
    
    // Configuration management
    void loadDefaults();
    void saveSettings();
    void loadSettings();
    bool isFirstRun() const;
    void resetToDefaults();
    
    // Validation
    bool validateDatabaseSettings() const;
    bool validateNetworkSettings() const;
    bool validatePaths() const;

signals:
    void settingChanged(const QString &key, const QVariant &value);
    void databaseSettingsChanged();
    void networkSettingsChanged();
    void uiSettingsChanged();

private:
    explicit SettingsManager(QObject *parent = nullptr);
    ~SettingsManager() override = default;
    
    void initializeDefaults();
    void validateAndFixSettings();
    QString databaseTypeToString(DatabaseType type) const;
    DatabaseType stringToDatabaseType(const QString &str) const;
    QString logLevelToString(LogLevel level) const;
    LogLevel stringToLogLevel(const QString &str) const;

    std::unique_ptr<QSettings> m_settings;
    mutable QMutex m_mutex;
    
    // Default values
    static const QString DEFAULT_DB_PATH;
    static const int DEFAULT_NOTIFICATION_PORT;
    static const int DEFAULT_UDP_PORT;
    static const QString DEFAULT_STYLESHEET_PATH;
    static const QString DEFAULT_LOG_PATH;
    static const QString DEFAULT_PDF_OUTPUT_DIR;
};
