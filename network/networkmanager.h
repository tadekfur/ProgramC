#pragma once
#include <QObject>
#include <QtNetwork/QUdpSocket>

class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(QObject *parent = nullptr);
    bool broadcastNotification(const QString &message, const QString &host, int port);
public slots:
    void broadcastNotificationAsync(const QString &message, const QString &host, int port);
signals:
    void notificationSent();
    void networkError(const QString &errorMsg);
};
