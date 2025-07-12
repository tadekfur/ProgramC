#pragma once

#include <QMainWindow>
#include <QWidget>
#include "models/user.h"
#include "network/notificationserver.h"
#include <QUdpSocket>

class SidebarManager;
class PageManager;
class QLabel;
class QTimer;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    void setUser(const User &user);
    
protected:
    void closeEvent(QCloseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;

signals:
    void logoutRequested();
    void clientAdded();

public slots:
    void refreshDashboard();

private slots:
    void updateSessionStatus();
    void recordUserActivity();

private:
    void setupUI();
    void setupNotifications();
    void setupKeyboardShortcuts();
    void setupSessionManagement();
    void connectSignals();
    void openSettingsDialog();
    void handleLogout();
    void handleNotificationMessage(const QString &message);
    void handleOrderRemoval(const QString &orderNumber);
    void sendOrderRemovalNotification(const QString &orderNumber);

    QWidget *m_centralWidget;
    SidebarManager *m_sidebarManager;
    PageManager *m_pageManager;
    
    User m_currentUser;

    NotificationServer *m_notifServer;
    QUdpSocket *m_udpListener;
    
    // Session management
    QLabel *m_sessionStatusLabel;
    QTimer *m_sessionUpdateTimer;
};