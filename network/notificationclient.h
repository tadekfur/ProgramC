#ifndef NOTIFICATIONCLIENT_H
#define NOTIFICATIONCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QUdpSocket>

class NotificationClient : public QObject {
    Q_OBJECT
public:
    explicit NotificationClient(QObject *parent = nullptr);
    void sendOrderDone(const QString &host, quint16 port, const QString &orderNumber);
    void broadcastRemoveOrder(const QString &orderNumber, quint16 port = 9001);

signals:
    void error(const QString &msg);
};

#endif // NOTIFICATIONCLIENT_H
