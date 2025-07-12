#ifndef NOTIFICATIONSERVER_H
#define NOTIFICATIONSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QObject>

class NotificationServer : public QTcpServer {
    Q_OBJECT
public:
    explicit NotificationServer(QObject *parent = nullptr);
    void startServer(quint16 port = 9000);

signals:
    void orderDoneReceived(const QString &orderNumber);

protected:
    void incomingConnection(qintptr socketDescriptor) override;
};

#endif // NOTIFICATIONSERVER_H
