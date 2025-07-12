/*
 * Security Migration Tool for EtykietyManager
 * 
 * This tool helps migrate from the old insecure system to the new secure system:
 * - Migrates plain text passwords to hashed passwords
 * - Backs up existing user data
 * - Validates migration success
 * 
 * Usage: migrate_security [--backup-only] [--restore backup_file]
 */

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <iostream>

#include "utils/secure_user_manager.h"
#include "utils/password_manager.h"
#include "models/user.h"

class SecurityMigration {
public:
    SecurityMigration() {
        m_appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        m_oldUsersPath = m_appDataPath + "/users.json";
        m_backupPath = m_appDataPath + "/backup_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".json";
        
        // Ensure directory exists
        QDir dir(m_appDataPath);
        if (!dir.exists()) {
            dir.mkpath(m_appDataPath);
        }
    }
    
    bool backupExistingData() {
        std::cout << "Creating backup of existing user data..." << std::endl;
        
        QFile oldFile(m_oldUsersPath);
        if (!oldFile.exists()) {
            std::cout << "No existing users file found at: " << m_oldUsersPath.toStdString() << std::endl;
            return true; // Not an error if file doesn't exist
        }
        
        if (!oldFile.copy(m_backupPath)) {
            std::cerr << "Failed to create backup: " << oldFile.errorString().toStdString() << std::endl;
            return false;
        }
        
        std::cout << "Backup created: " << m_backupPath.toStdString() << std::endl;
        return true;
    }
    
    bool restoreFromBackup(const QString& backupFile) {
        std::cout << "Restoring from backup: " << backupFile.toStdString() << std::endl;
        
        QFile backup(backupFile);
        if (!backup.exists()) {
            std::cerr << "Backup file not found: " << backupFile.toStdString() << std::endl;
            return false;
        }
        
        // Remove current users file
        QFile::remove(m_oldUsersPath);
        
        // Copy backup to users file
        if (!backup.copy(m_oldUsersPath)) {
            std::cerr << "Failed to restore backup: " << backup.errorString().toStdString() << std::endl;
            return false;
        }
        
        std::cout << "Backup restored successfully" << std::endl;
        return true;
    }
    
    bool analyzeExistingUsers() {
        std::cout << "Analyzing existing user data..." << std::endl;
        
        QFile file(m_oldUsersPath);
        if (!file.exists()) {
            std::cout << "No existing users file found" << std::endl;
            return true;
        }
        
        if (!file.open(QIODevice::ReadOnly)) {
            std::cerr << "Failed to open users file: " << file.errorString().toStdString() << std::endl;
            return false;
        }
        
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        
        if (!doc.isArray()) {
            std::cerr << "Invalid users file format" << std::endl;
            return false;
        }
        
        QJsonArray array = doc.array();
        int totalUsers = array.size();
        int hashedUsers = 0;
        int plainTextUsers = 0;
        
        for (const QJsonValue& value : array) {
            QJsonObject obj = value.toObject();
            QString password = obj["password"].toString();
            
            if (password.contains('$')) {
                hashedUsers++;
            } else {
                plainTextUsers++;
            }
        }
        
        std::cout << "Analysis results:" << std::endl;
        std::cout << "  Total users: " << totalUsers << std::endl;
        std::cout << "  Already hashed: " << hashedUsers << std::endl;
        std::cout << "  Plain text passwords: " << plainTextUsers << std::endl;
        
        return true;
    }
    
    bool migrateUsers() {
        std::cout << "Starting user migration..." << std::endl;
        
        // First, create backup
        if (!backupExistingData()) {
            return false;
        }
        
        // Analyze current state
        analyzeExistingUsers();
        
        // Load users using SecureUserManager (which will auto-migrate)
        SecureUserManager& userManager = SecureUserManager::instance();
        userManager.loadUsersFromFile();
        
        const QVector<User>& users = userManager.users();
        std::cout << "Migration completed. Total users: " << users.size() << std::endl;
        
        // Verify migration
        return verifyMigration();
    }
    
    bool verifyMigration() {
        std::cout << "Verifying migration..." << std::endl;
        
        QFile file(m_oldUsersPath);
        if (!file.open(QIODevice::ReadOnly)) {
            std::cerr << "Failed to open users file for verification" << std::endl;
            return false;
        }
        
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray array = doc.array();
        
        bool allHashed = true;
        int checkedUsers = 0;
        
        for (const QJsonValue& value : array) {
            QJsonObject obj = value.toObject();
            QString password = obj["password"].toString();
            QString login = obj["login"].toString();
            
            if (!password.contains('$')) {
                std::cerr << "User " << login.toStdString() << " still has plain text password!" << std::endl;
                allHashed = false;
            } else {
                // Verify password format (salt$hash)
                QStringList parts = password.split('$');
                if (parts.size() != 2) {
                    std::cerr << "User " << login.toStdString() << " has invalid password hash format!" << std::endl;
                    allHashed = false;
                }
            }
            checkedUsers++;
        }
        
        if (allHashed) {
            std::cout << "✓ Migration verification successful!" << std::endl;
            std::cout << "  All " << checkedUsers << " users have properly hashed passwords" << std::endl;
        } else {
            std::cerr << "✗ Migration verification failed!" << std::endl;
        }
        
        return allHashed;
    }
    
    void showUsage() {
        std::cout << "Security Migration Tool for EtykietyManager\n" << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "  migrate_security                    - Migrate users to secure system" << std::endl;
        std::cout << "  migrate_security --analyze          - Analyze current user data" << std::endl;
        std::cout << "  migrate_security --backup-only      - Create backup only" << std::endl;
        std::cout << "  migrate_security --restore <file>   - Restore from backup" << std::endl;
        std::cout << "  migrate_security --verify           - Verify migration" << std::endl;
        std::cout << std::endl;
        std::cout << "Files:" << std::endl;
        std::cout << "  Users file: " << m_oldUsersPath.toStdString() << std::endl;
        std::cout << "  Backup path: " << m_backupPath.toStdString() << std::endl;
    }
    
private:
    QString m_appDataPath;
    QString m_oldUsersPath;
    QString m_backupPath;
};

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("Security Migration Tool");
    app.setApplicationVersion("1.0");
    
    QCommandLineParser parser;
    parser.setApplicationDescription("Migrates EtykietyManager to secure password system");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption analyzeOption("analyze", "Analyze current user data without migrating");
    QCommandLineOption backupOption("backup-only", "Create backup only");
    QCommandLineOption restoreOption("restore", "Restore from backup file", "backup_file");
    QCommandLineOption verifyOption("verify", "Verify migration");
    
    parser.addOption(analyzeOption);
    parser.addOption(backupOption);
    parser.addOption(restoreOption);
    parser.addOption(verifyOption);
    
    parser.process(app);
    
    SecurityMigration migration;
    
    if (parser.isSet("help")) {
        migration.showUsage();
        return 0;
    }
    
    if (parser.isSet(analyzeOption)) {
        return migration.analyzeExistingUsers() ? 0 : 1;
    }
    
    if (parser.isSet(backupOption)) {
        return migration.backupExistingData() ? 0 : 1;
    }
    
    if (parser.isSet(restoreOption)) {
        QString backupFile = parser.value(restoreOption);
        return migration.restoreFromBackup(backupFile) ? 0 : 1;
    }
    
    if (parser.isSet(verifyOption)) {
        return migration.verifyMigration() ? 0 : 1;
    }
    
    // Default action: migrate
    return migration.migrateUsers() ? 0 : 1;
}