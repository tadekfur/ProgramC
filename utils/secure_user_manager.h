#pragma once
#include <QObject>
#include <QVector>
#include "../models/user.h"

class SecureUserManager : public QObject {
    Q_OBJECT
    
public:
    static SecureUserManager& instance();
    
    // User management
    const QVector<User>& users() const;
    bool addUser(const User& user);
    bool removeUser(int index);
    bool updateUser(int index, const User& user);
    
    // Authentication
    bool authenticateUser(const QString& login, const QString& password);
    User getUser(const QString& login) const;
    
    // Password management
    bool changePassword(const QString& login, const QString& oldPassword, const QString& newPassword);
    bool resetPassword(const QString& login); // Generates new password
    
    // Account management
    bool lockAccount(const QString& login);
    bool unlockAccount(const QString& login);
    bool isAccountLocked(const QString& login) const;
    
    // Login attempts tracking
    void recordLoginAttempt(const QString& login, bool success);
    int getFailedLoginAttempts(const QString& login) const;
    void clearFailedLoginAttempts(const QString& login);
    
    // Persistence
    void loadUsersFromFile();
    void saveUsersToFile();
    
signals:
    void userAdded(const User& user);
    void userUpdated(const User& user);
    void userRemoved(const QString& login);
    void accountLocked(const QString& login);
    void suspiciousActivity(const QString& login, const QString& activity);
    
private:
    SecureUserManager();
    
    struct LoginAttemptData {
        int failedAttempts = 0;
        QDateTime lastAttempt;
        bool isLocked = false;
    };
    
    QVector<User> m_users;
    QMap<QString, LoginAttemptData> m_loginAttempts;
    QString m_filePath;
    
    static const int MAX_LOGIN_ATTEMPTS = 5;
    static const int LOCKOUT_DURATION = 900; // 15 minutes
    
    bool validatePassword(const QString& password) const;
    void migrateOldPasswords(); // For upgrading from plain text passwords
    QString hashPassword(const QString& password) const;
    bool verifyPassword(const QString& password, const QString& hash) const;
};