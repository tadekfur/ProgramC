#include "session_manager.h"
#include "secure_config.h"
#include <QUuid>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>

SessionManager& SessionManager::instance() {
    static SessionManager instance;
    return instance;
}

SessionManager::SessionManager(QObject* parent) 
    : QObject(parent), m_inactivityTimerPaused(false) {
    
    m_sessionTimeout = SecureConfig::instance().getSessionTimeout();
    
    // Setup expiry timer
    m_expiryTimer = new QTimer(this);
    m_expiryTimer->setInterval(CLEANUP_INTERVAL);
    connect(m_expiryTimer, &QTimer::timeout, this, &SessionManager::checkSessionExpiry);
    m_expiryTimer->start();
    
    // Setup inactivity timer
    m_inactivityTimer = new QTimer(this);
    m_inactivityTimer->setInterval(INACTIVITY_CHECK_INTERVAL);
    connect(m_inactivityTimer, &QTimer::timeout, this, &SessionManager::checkInactivity);
    
    // Setup warning timer
    m_warningTimer = new QTimer(this);
    m_warningTimer->setSingleShot(true);
    connect(m_warningTimer, &QTimer::timeout, this, &SessionManager::showInactivityWarning);
}

QString SessionManager::createSession(const User& user) {
    QString sessionId = generateSessionId();
    
    QDateTime now = QDateTime::currentDateTime();
    SessionData sessionData;
    sessionData.user = user;
    sessionData.createdAt = now;
    sessionData.expiresAt = now.addSecs(m_sessionTimeout);
    sessionData.lastActivity = now;
    
    m_sessions[sessionId] = sessionData;
    m_currentSessionId = sessionId;
    m_lastActivity = now;
    
    // Start inactivity monitoring
    m_inactivityTimer->start();
    
    qDebug() << "Session created for user:" << user.login << "expires at:" << sessionData.expiresAt;
    
    emit sessionCreated(sessionId);
    return sessionId;
}

bool SessionManager::validateSession(const QString& sessionId) const {
    if (sessionId.isEmpty() || !m_sessions.contains(sessionId)) {
        return false;
    }
    
    const SessionData& session = m_sessions[sessionId];
    QDateTime now = QDateTime::currentDateTime();
    
    // Just check if session is expired, don't modify state in const method
    if (now > session.expiresAt) {
        return false;
    }
    
    return true;
}

void SessionManager::refreshSession(const QString& sessionId) {
    if (!m_sessions.contains(sessionId)) {
        return;
    }
    
    QDateTime now = QDateTime::currentDateTime();
    SessionData& session = m_sessions[sessionId];
    session.expiresAt = now.addSecs(m_sessionTimeout);
    session.lastActivity = now;
    
    if (sessionId == m_currentSessionId) {
        m_lastActivity = now;
    }
    
    qDebug() << "Session refreshed, new expiry:" << session.expiresAt;
}

void SessionManager::destroySession(const QString& sessionId) {
    if (!m_sessions.contains(sessionId)) {
        return;
    }
    
    User user = m_sessions[sessionId].user;
    m_sessions.remove(sessionId);
    
    if (sessionId == m_currentSessionId) {
        m_currentSessionId.clear();
        m_inactivityTimer->stop();
        m_warningTimer->stop();
    }
    
    qDebug() << "Session destroyed for user:" << user.login;
    emit sessionDestroyed(sessionId);
}

void SessionManager::destroyAllSessions() {
    QStringList sessionIds = m_sessions.keys();
    for (const QString& sessionId : sessionIds) {
        destroySession(sessionId);
    }
}

User SessionManager::getCurrentUser() const {
    if (m_currentSessionId.isEmpty() || !m_sessions.contains(m_currentSessionId)) {
        return User();
    }
    
    return m_sessions[m_currentSessionId].user;
}

QString SessionManager::getCurrentSessionId() const {
    return m_currentSessionId;
}

bool SessionManager::isLoggedIn() const {
    return !m_currentSessionId.isEmpty() && validateSession(m_currentSessionId);
}

QDateTime SessionManager::getSessionExpiry(const QString& sessionId) const {
    if (!m_sessions.contains(sessionId)) {
        return QDateTime();
    }
    
    return m_sessions[sessionId].expiresAt;
}

int SessionManager::getSessionTimeout() const {
    return m_sessionTimeout;
}

void SessionManager::setSessionTimeout(int seconds) {
    m_sessionTimeout = seconds;
}

void SessionManager::recordActivity() {
    QDateTime now = QDateTime::currentDateTime();
    m_lastActivity = now;
    
    if (!m_currentSessionId.isEmpty()) {
        refreshSession(m_currentSessionId);
    }
    
    // Reset warning timer
    m_warningTimer->stop();
    if (m_inactivityTimer->isActive()) {
        int timeToWarning = (m_sessionTimeout - INACTIVITY_WARNING_TIME) * 1000;
        if (timeToWarning > 0) {
            m_warningTimer->start(timeToWarning);
        }
    }
}

void SessionManager::pauseInactivityTimer() {
    m_inactivityTimerPaused = true;
    m_warningTimer->stop();
}

void SessionManager::resumeInactivityTimer() {
    m_inactivityTimerPaused = false;
    recordActivity(); // Reset activity tracking
}

void SessionManager::checkSessionExpiry() {
    QDateTime now = QDateTime::currentDateTime();
    QStringList expiredSessions;
    
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if (now > it.value().expiresAt) {
            expiredSessions.append(it.key());
        }
    }
    
    for (const QString& sessionId : expiredSessions) {
        if (sessionId == m_currentSessionId) {
            emit sessionExpired();
        }
        destroySession(sessionId);
    }
}

void SessionManager::checkInactivity() {
    if (m_inactivityTimerPaused || m_currentSessionId.isEmpty()) {
        return;
    }
    
    QDateTime now = QDateTime::currentDateTime();
    int secondsSinceActivity = m_lastActivity.secsTo(now);
    
    if (secondsSinceActivity >= m_sessionTimeout) {
        qDebug() << "Session expired due to inactivity";
        emit sessionExpired();
        destroySession(m_currentSessionId);
    }
}

void SessionManager::showInactivityWarning() {
    if (m_inactivityTimerPaused || m_currentSessionId.isEmpty()) {
        return;
    }
    
    QDateTime now = QDateTime::currentDateTime();
    int secondsSinceActivity = m_lastActivity.secsTo(now);
    int secondsLeft = m_sessionTimeout - secondsSinceActivity;
    
    if (secondsLeft > 0) {
        emit inactivityWarning(secondsLeft);
    }
}

QString SessionManager::generateSessionId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void SessionManager::cleanupExpiredSessions() {
    checkSessionExpiry();
}