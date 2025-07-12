#include "settings_manager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QMutexLocker>
#include <QDebug>
#include <QCoreApplication>

// Static default values
const QString SettingsManager::DEFAULT_DB_PATH = "etykiety_manager.db";
const int SettingsManager::DEFAULT_NOTIFICATION_PORT = 9000;
const int SettingsManager::DEFAULT_UDP_PORT = 9001;
const QString SettingsManager::DEFAULT_STYLESHEET_PATH = "style.qss";
const QString SettingsManager::DEFAULT_LOG_PATH = "logs/etykiety_manager.log";
const QString SettingsManager::DEFAULT_PDF_OUTPUT_DIR = "pdf_output";

SettingsManager& SettingsManager::instance() {
    static SettingsManager instance;
    return instance;
}

SettingsManager::SettingsManager(QObject *parent) 
    : QObject(parent) {
    try {
        m_settings = std::make_unique<QSettings>(
            QSettings::IniFormat, 
            QSettings::UserScope,
            QCoreApplication::organizationName(),
            QCoreApplication::applicationName()
        );
        
        initializeDefaults();
        loadSettings();
        validateAndFixSettings();
        
    } catch (const std::exception &e) {
        qCritical() << "Failed to initialize SettingsManager:" << e.what();
        throw;
    }
}

void SettingsManager::initializeDefaults() {
    QMutexLocker locker(&m_mutex);
    
    if (isFirstRun()) {
        qDebug() << "First run detected, loading default settings";
        loadDefaults();
    }
}

void SettingsManager::loadDefaults() {
    // Database defaults
    setValue("database/type", "SQLite");
    setValue("database/path", DEFAULT_DB_PATH);
    setValue("database/host", "localhost");
    setValue("database/port", 5432);
    setValue("database/name", "etykiety_manager");
    setValue("database/user", "");
    setValue("database/password", "");
    
    // Network defaults
    setValue("network/notification_port", DEFAULT_NOTIFICATION_PORT);
    setValue("network/udp_port", DEFAULT_UDP_PORT);
    
    // UI defaults
    setValue("ui/stylesheet_path", DEFAULT_STYLESHEET_PATH);
    setValue("ui/maximize_on_startup", true);
    setValue("ui/last_opened_page", "Dashboard");
    
    // Logging defaults
    setValue("logging/level", "Info");
    setValue("logging/file_path", DEFAULT_LOG_PATH);
    setValue("logging/to_file", true);
    setValue("logging/to_console", true);
    
    // PDF defaults
    setValue("pdf/output_directory", DEFAULT_PDF_OUTPUT_DIR);
    setValue("pdf/default_template", "standard");
    
    // System
    setValue("system/first_run", false);
    setValue("system/version", QCoreApplication::applicationVersion());
}

void SettingsManager::validateAndFixSettings() {
    // Validate and fix database settings
    if (!validateDatabaseSettings()) {
        qWarning() << "Invalid database settings detected, resetting to defaults";
        setDatabaseType(DatabaseType::SQLite);
        setDatabasePath(DEFAULT_DB_PATH);
    }
    
    // Validate and fix network settings
    if (!validateNetworkSettings()) {
        qWarning() << "Invalid network settings detected, resetting to defaults";
        setNotificationServerPort(DEFAULT_NOTIFICATION_PORT);
        setUdpListenerPort(DEFAULT_UDP_PORT);
    }
    
    // Validate paths
    if (!validatePaths()) {
        qWarning() << "Invalid paths detected, creating directories";
        // Create necessary directories
        QDir().mkpath(QFileInfo(getLogFilePath()).absolutePath());
        QDir().mkpath(getPdfOutputDirectory());
    }
}

// Application settings
QString SettingsManager::getApplicationName() const {
    return QCoreApplication::applicationName();
}

QString SettingsManager::getApplicationVersion() const {
    return QCoreApplication::applicationVersion();
}

QString SettingsManager::getOrganizationName() const {
    return QCoreApplication::organizationName();
}

// Database settings
SettingsManager::DatabaseType SettingsManager::getDatabaseType() const {
    return stringToDatabaseType(getValue("database/type", "SQLite").toString());
}

void SettingsManager::setDatabaseType(DatabaseType type) {
    setValue("database/type", databaseTypeToString(type));
    emit databaseSettingsChanged();
}

QString SettingsManager::getDatabasePath() const {
    return getValue("database/path", DEFAULT_DB_PATH).toString();
}

void SettingsManager::setDatabasePath(const QString &path) {
    setValue("database/path", path);
    emit databaseSettingsChanged();
}

QString SettingsManager::getDatabaseHost() const {
    return getValue("database/host", "localhost").toString();
}

void SettingsManager::setDatabaseHost(const QString &host) {
    setValue("database/host", host);
    emit databaseSettingsChanged();
}

int SettingsManager::getDatabasePort() const {
    return getValue("database/port", 5432).toInt();
}

void SettingsManager::setDatabasePort(int port) {
    setValue("database/port", port);
    emit databaseSettingsChanged();
}

QString SettingsManager::getDatabaseName() const {
    return getValue("database/name", "etykiety_manager").toString();
}

void SettingsManager::setDatabaseName(const QString &name) {
    setValue("database/name", name);
    emit databaseSettingsChanged();
}

QString SettingsManager::getDatabaseUser() const {
    return getValue("database/user", "").toString();
}

void SettingsManager::setDatabaseUser(const QString &user) {
    setValue("database/user", user);
    emit databaseSettingsChanged();
}

QString SettingsManager::getDatabasePassword() const {
    return getValue("database/password", "").toString();
}

void SettingsManager::setDatabasePassword(const QString &password) {
    setValue("database/password", password);
    emit databaseSettingsChanged();
}

// Network settings
int SettingsManager::getNotificationServerPort() const {
    return getValue("network/notification_port", DEFAULT_NOTIFICATION_PORT).toInt();
}

void SettingsManager::setNotificationServerPort(int port) {
    setValue("network/notification_port", port);
    emit networkSettingsChanged();
}

int SettingsManager::getUdpListenerPort() const {
    return getValue("network/udp_port", DEFAULT_UDP_PORT).toInt();
}

void SettingsManager::setUdpListenerPort(int port) {
    setValue("network/udp_port", port);
    emit networkSettingsChanged();
}

// UI settings
QString SettingsManager::getStyleSheetPath() const {
    return getValue("ui/stylesheet_path", DEFAULT_STYLESHEET_PATH).toString();
}

void SettingsManager::setStyleSheetPath(const QString &path) {
    setValue("ui/stylesheet_path", path);
    emit uiSettingsChanged();
}

bool SettingsManager::getMaximizeOnStartup() const {
    return getValue("ui/maximize_on_startup", true).toBool();
}

void SettingsManager::setMaximizeOnStartup(bool maximize) {
    setValue("ui/maximize_on_startup", maximize);
    emit uiSettingsChanged();
}

QString SettingsManager::getLastOpenedPage() const {
    return getValue("ui/last_opened_page", "Dashboard").toString();
}

void SettingsManager::setLastOpenedPage(const QString &page) {
    setValue("ui/last_opened_page", page);
}

// Logging settings
SettingsManager::LogLevel SettingsManager::getLogLevel() const {
    return stringToLogLevel(getValue("logging/level", "Info").toString());
}

void SettingsManager::setLogLevel(LogLevel level) {
    setValue("logging/level", logLevelToString(level));
}

QString SettingsManager::getLogFilePath() const {
    return getValue("logging/file_path", DEFAULT_LOG_PATH).toString();
}

void SettingsManager::setLogFilePath(const QString &path) {
    setValue("logging/file_path", path);
}

bool SettingsManager::getLogToFile() const {
    return getValue("logging/to_file", true).toBool();
}

void SettingsManager::setLogToFile(bool enable) {
    setValue("logging/to_file", enable);
}

bool SettingsManager::getLogToConsole() const {
    return getValue("logging/to_console", true).toBool();
}

void SettingsManager::setLogToConsole(bool enable) {
    setValue("logging/to_console", enable);
}

// PDF generation settings
QString SettingsManager::getPdfOutputDirectory() const {
    return getValue("pdf/output_directory", DEFAULT_PDF_OUTPUT_DIR).toString();
}

void SettingsManager::setPdfOutputDirectory(const QString &path) {
    setValue("pdf/output_directory", path);
}

QString SettingsManager::getDefaultPdfTemplate() const {
    return getValue("pdf/default_template", "standard").toString();
}

void SettingsManager::setDefaultPdfTemplate(const QString &templateName) {
    setValue("pdf/default_template", templateName);
}

// Generic settings access
QVariant SettingsManager::getValue(const QString &key, const QVariant &defaultValue) const {
    QMutexLocker locker(&m_mutex);
    return m_settings->value(key, defaultValue);
}

void SettingsManager::setValue(const QString &key, const QVariant &value) {
    QMutexLocker locker(&m_mutex);
    m_settings->setValue(key, value);
    emit settingChanged(key, value);
}

bool SettingsManager::contains(const QString &key) const {
    QMutexLocker locker(&m_mutex);
    return m_settings->contains(key);
}

void SettingsManager::remove(const QString &key) {
    QMutexLocker locker(&m_mutex);
    m_settings->remove(key);
}

// Configuration management
void SettingsManager::saveSettings() {
    QMutexLocker locker(&m_mutex);
    m_settings->sync();
}

void SettingsManager::loadSettings() {
    QMutexLocker locker(&m_mutex);
    m_settings->sync();
}

bool SettingsManager::isFirstRun() const {
    return getValue("system/first_run", true).toBool();
}

void SettingsManager::resetToDefaults() {
    QMutexLocker locker(&m_mutex);
    m_settings->clear();
    locker.unlock();
    loadDefaults();
}

// Validation
bool SettingsManager::validateDatabaseSettings() const {
    DatabaseType type = getDatabaseType();
    
    if (type == DatabaseType::SQLite) {
        QString path = getDatabasePath();
        return !path.isEmpty();
    } else {
        QString host = getDatabaseHost();
        int port = getDatabasePort();
        QString name = getDatabaseName();
        
        return !host.isEmpty() && port > 0 && port < 65536 && !name.isEmpty();
    }
}

bool SettingsManager::validateNetworkSettings() const {
    int notifPort = getNotificationServerPort();
    int udpPort = getUdpListenerPort();
    
    return notifPort > 0 && notifPort < 65536 && 
           udpPort > 0 && udpPort < 65536 && 
           notifPort != udpPort;
}

bool SettingsManager::validatePaths() const {
    QString logPath = getLogFilePath();
    QString pdfPath = getPdfOutputDirectory();
    
    QFileInfo logInfo(logPath);
    QFileInfo pdfInfo(pdfPath);
    
    return QDir().mkpath(logInfo.absolutePath()) && 
           QDir().mkpath(pdfInfo.absolutePath());
}

// Helper methods
QString SettingsManager::databaseTypeToString(DatabaseType type) const {
    switch (type) {
        case DatabaseType::SQLite: return "SQLite";
        case DatabaseType::MySQL: return "MySQL";
        case DatabaseType::PostgreSQL: return "PostgreSQL";
        default: return "SQLite";
    }
}

SettingsManager::DatabaseType SettingsManager::stringToDatabaseType(const QString &str) const {
    if (str == "MySQL") return DatabaseType::MySQL;
    if (str == "PostgreSQL") return DatabaseType::PostgreSQL;
    return DatabaseType::SQLite; // default
}

QString SettingsManager::logLevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug: return "Debug";
        case LogLevel::Info: return "Info";
        case LogLevel::Warning: return "Warning";
        case LogLevel::Error: return "Error";
        case LogLevel::Critical: return "Critical";
        default: return "Info";
    }
}

SettingsManager::LogLevel SettingsManager::stringToLogLevel(const QString &str) const {
    if (str == "Debug") return LogLevel::Debug;
    if (str == "Warning") return LogLevel::Warning;
    if (str == "Error") return LogLevel::Error;
    if (str == "Critical") return LogLevel::Critical;
    return LogLevel::Info; // default
}
