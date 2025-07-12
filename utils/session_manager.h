#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QMap>
#include "../models/user.h"

class SessionManager : public QObject {
    Q_OBJECT
    
public:
    static SessionManager& instance();
    
    // Session operations
    QString createSession(const User& user);
    bool validateSession(const QString& sessionId) const;
    void refreshSession(const QString& sessionId);
    void destroySession(const QString& sessionId);
    void destroyAllSessions();
    
    // User info
    User getCurrentUser() const;
    QString getCurrentSessionId() const;
    bool isLoggedIn() const;
    
    // Session info
    QDateTime getSessionExpiry(const QString& sessionId) const;
    int getSessionTimeout() const;
    void setSessionTimeout(int seconds);
    
    // Auto-logout on inactivity
    void recordActivity();
    void pauseInactivityTimer();
    void resumeInactivityTimer();
    
signals:
    void sessionExpired();
    void inactivityWarning(int secondsLeft);
    void sessionCreated(const QString& sessionId);
    void sessionDestroyed(const QString& sessionId);
    
private slots:
    void checkSessionExpiry();
    void checkInactivity();
    void showInactivityWarning();
    
private:
    SessionManager(QObject* parent = nullptr);
    QString generateSessionId();
    void cleanupExpiredSessions();
    
    struct SessionData {
        User user;
        QDateTime createdAt;
        QDateTime expiresAt;
        QDateTime lastActivity;
    };
    
    QMap<QString, SessionData> m_sessions;
    QString m_currentSessionId;
    int m_sessionTimeout; // in seconds
    
    QTimer* m_expiryTimer;
    QTimer* m_inactivityTimer;
    QTimer* m_warningTimer;
    
    QDateTime m_lastActivity;
    bool m_inactivityTimerPaused;
    
    static const int INACTIVITY_WARNING_TIME = 300; // 5 minutes before logout
    static const int CLEANUP_INTERVAL = 60000; // 1 minute
    static const int INACTIVITY_CHECK_INTERVAL = 30000; // 30 seconds
};