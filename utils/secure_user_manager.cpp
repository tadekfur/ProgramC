#include "secure_user_manager.h"
#include "password_manager.h"
#include "secure_config.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QDateTime>

SecureUserManager& SecureUserManager::instance() {
    static SecureUserManager instance;
    return instance;
}

SecureUserManager::SecureUserManager() {
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    if (!dir.exists()) {
        dir.mkpath(appDataPath);
    }
    m_filePath = appDataPath + "/users.json";
    
    loadUsersFromFile();
}

const QVector<User>& SecureUserManager::users() const {
    return m_users;
}

bool SecureUserManager::addUser(const User& user) {
    // Sprawdź czy użytkownik już istnieje
    for (const User& existingUser : m_users) {
        if (existingUser.login == user.login) {
            qWarning() << "User with login" << user.login << "already exists";
            return false;
        }
    }
    
    // Sprawdź siłę hasła (tymczasowo wyłączone)
    // if (!validatePassword(user.password)) {
    //     qWarning() << "Password for user" << user.login << "is too weak";
    //     return false;
    // }
    
    // Utwórz kopię użytkownika z zahashowanym hasłem
    User secureUser = user;
    secureUser.password = hashPassword(user.password);
    
    m_users.append(secureUser);
    saveUsersToFile();
    
    qDebug() << "User" << user.login << "added successfully";
    emit userAdded(user);
    return true;
}

bool SecureUserManager::removeUser(int index) {
    if (index < 0 || index >= m_users.size()) {
        return false;
    }
    
    User removedUser = m_users[index];
    m_users.removeAt(index);
    m_loginAttempts.remove(removedUser.login);
    saveUsersToFile();
    
    emit userRemoved(removedUser.login);
    return true;
}

bool SecureUserManager::updateUser(int index, const User& user) {
    if (index < 0 || index >= m_users.size()) {
        return false;
    }
    
    User& existingUser = m_users[index];
    
    // Jeśli hasło się zmieniło, zahashuj je
    if (user.password != existingUser.password) {
        // if (!validatePassword(user.password)) {
        //     qWarning() << "New password for user" << user.login << "is too weak";
        //     return false;
        // }
        existingUser.password = hashPassword(user.password);
    }
    
    // Zaktualizuj inne pola
    existingUser.firstName = user.firstName;
    existingUser.lastName = user.lastName;
    existingUser.position = user.position;
    existingUser.email = user.email;
    existingUser.phone = user.phone;
    existingUser.role = user.role;
    existingUser.confirmationPrinter = user.confirmationPrinter;
    existingUser.productionPrinter = user.productionPrinter;
    existingUser.confirmationDir = user.confirmationDir;
    existingUser.productionDir = user.productionDir;
    existingUser.smtpServer = user.smtpServer;
    existingUser.smtpPort = user.smtpPort;
    existingUser.smtpUser = user.smtpUser;
    existingUser.smtpPassword = user.smtpPassword;
    existingUser.style = user.style;
    
    saveUsersToFile();
    emit userUpdated(existingUser);
    return true;
}

bool SecureUserManager::authenticateUser(const QString& login, const QString& password) {
    // Sprawdź czy konto nie jest zablokowane
    if (isAccountLocked(login)) {
        qWarning() << "Account" << login << "is locked";
        recordLoginAttempt(login, false);
        return false;
    }
    
    // Znajdź użytkownika
    User user = getUser(login);
    if (user.login.isEmpty()) {
        qWarning() << "User" << login << "not found";
        recordLoginAttempt(login, false);
        return false;
    }
    
    // Sprawdź hasło - najpierw hash, potem plain text fallback
    bool isValid = verifyPassword(password, user.password);
    
    // Fallback dla plain text haseł
    if (!isValid && !user.password.contains('$')) {
        isValid = (password == user.password);
        qDebug() << "Plain text fallback for" << login << ":" << isValid;
    }
    
    recordLoginAttempt(login, isValid);
    
    if (isValid) {
        clearFailedLoginAttempts(login);
        qDebug() << "User" << login << "authenticated successfully";
    } else {
        qWarning() << "Invalid password for user" << login;
    }
    
    return isValid;
}

User SecureUserManager::getUser(const QString& login) const {
    for (const User& user : m_users) {
        if (user.login == login) {
            return user;
        }
    }
    return User(); // Zwróć pusty obiekt jeśli nie znaleziono
}

bool SecureUserManager::changePassword(const QString& login, const QString& oldPassword, const QString& newPassword) {
    if (!authenticateUser(login, oldPassword)) {
        return false;
    }
    
    // Sprawdź siłę hasła (tymczasowo wyłączone)
    // if (!validatePassword(newPassword)) {
    //     qWarning() << "New password for user" << login << "is too weak";
    //     return false;
    // }
    
    // Znajdź użytkownika i zaktualizuj hasło
    for (User& user : m_users) {
        if (user.login == login) {
            user.password = hashPassword(newPassword);
            saveUsersToFile();
            qDebug() << "Password changed for user" << login;
            return true;
        }
    }
    
    return false;
}

bool SecureUserManager::resetPassword(const QString& login) {
    User user = getUser(login);
    if (user.login.isEmpty()) {
        return false;
    }
    
    QString newPassword = PasswordManager::generateSecurePassword();
    
    // Znajdź użytkownika i zaktualizuj hasło
    for (User& u : m_users) {
        if (u.login == login) {
            u.password = hashPassword(newPassword);
            saveUsersToFile();
            qDebug() << "Password reset for user" << login << "New password:" << newPassword;
            return true;
        }
    }
    
    return false;
}

bool SecureUserManager::lockAccount(const QString& login) {
    if (!m_loginAttempts.contains(login)) {
        m_loginAttempts[login] = LoginAttemptData();
    }
    
    m_loginAttempts[login].isLocked = true;
    qDebug() << "Account" << login << "locked";
    emit accountLocked(login);
    return true;
}

bool SecureUserManager::unlockAccount(const QString& login) {
    if (m_loginAttempts.contains(login)) {
        m_loginAttempts[login].isLocked = false;
        m_loginAttempts[login].failedAttempts = 0;
        qDebug() << "Account" << login << "unlocked";
        return true;
    }
    return false;
}

bool SecureUserManager::isAccountLocked(const QString& login) const {
    if (!m_loginAttempts.contains(login)) {
        return false;
    }
    
    const LoginAttemptData& data = m_loginAttempts[login];
    
    // Sprawdź manual lock
    if (data.isLocked) {
        return true;
    }
    
    // Sprawdź auto-lock z powodu zbyt wielu prób
    if (data.failedAttempts >= MAX_LOGIN_ATTEMPTS) {
        QDateTime lockTime = data.lastAttempt.addSecs(LOCKOUT_DURATION);
        return QDateTime::currentDateTime() < lockTime;
    }
    
    return false;
}

void SecureUserManager::recordLoginAttempt(const QString& login, bool success) {
    if (!m_loginAttempts.contains(login)) {
        m_loginAttempts[login] = LoginAttemptData();
    }
    
    LoginAttemptData& data = m_loginAttempts[login];
    data.lastAttempt = QDateTime::currentDateTime();
    
    if (success) {
        data.failedAttempts = 0;
    } else {
        data.failedAttempts++;
        
        if (data.failedAttempts >= MAX_LOGIN_ATTEMPTS) {
            qWarning() << "Account" << login << "auto-locked due to too many failed attempts";
            emit accountLocked(login);
            emit suspiciousActivity(login, "Too many failed login attempts");
        }
    }
}

int SecureUserManager::getFailedLoginAttempts(const QString& login) const {
    if (!m_loginAttempts.contains(login)) {
        return 0;
    }
    return m_loginAttempts[login].failedAttempts;
}

void SecureUserManager::clearFailedLoginAttempts(const QString& login) {
    if (m_loginAttempts.contains(login)) {
        m_loginAttempts[login].failedAttempts = 0;
    }
}

void SecureUserManager::loadUsersFromFile() {
    qDebug() << "Loading users from:" << m_filePath;
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open users file:" << m_filePath;
        return;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray array = doc.array();
    
    m_users.clear();
    for (const QJsonValue& value : array) {
        QJsonObject obj = value.toObject();
        User user = User::fromJson(obj);
        m_users.append(user);
    }
    
    // Migruj stare hasła (plain text) do hashowanych
    migrateOldPasswords();
    
    qDebug() << "Loaded" << m_users.size() << "users from file";
}

void SecureUserManager::saveUsersToFile() {
    QJsonArray array;
    for (const User& user : m_users) {
        array.append(user.toJson());
    }
    
    QJsonDocument doc(array);
    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot open users file for writing:" << m_filePath;
        return;
    }
    
    file.write(doc.toJson());
    qDebug() << "Saved" << m_users.size() << "users to file";
}

bool SecureUserManager::validatePassword(const QString& password) const {
    return PasswordManager::isPasswordStrong(password);
}

void SecureUserManager::migrateOldPasswords() {
    bool migrationNeeded = false;
    
    for (User& user : m_users) {
        // Sprawdź czy hasło nie zawiera znaku '$' (znacznik hasha)
        if (!user.password.contains('$')) {
            qDebug() << "Migrating password for user" << user.login;
            user.password = hashPassword(user.password);
            migrationNeeded = true;
        }
    }
    
    if (migrationNeeded) {
        saveUsersToFile();
        qDebug() << "Password migration completed";
    }
}

QString SecureUserManager::hashPassword(const QString& password) const {
    return PasswordManager::hashPassword(password);
}

bool SecureUserManager::verifyPassword(const QString& password, const QString& hash) const {
    return PasswordManager::verifyPassword(password, hash);
}