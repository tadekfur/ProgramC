#include "notificationclient.h"
#include <QHostAddress>
#include <QDebug>

NotificationClient::NotificationClient(QObject *parent) : QObject(parent) {}

void NotificationClient::sendOrderDone(const QString &host, quint16 port, const QString &orderNumber) {
    QTcpSocket *socket = new QTcpSocket(this);
    socket->connectToHost(host, port);
    if (!socket->waitForConnected(1000)) {
        emit error("Brak połączenia z serwerem powiadomień");
        socket->deleteLater();
        return;
    }
    QString msg = QString("ORDER_DONE:%1").arg(orderNumber);
    socket->write(msg.toUtf8());
    socket->waitForBytesWritten(500);
    socket->disconnectFromHost();
    socket->deleteLater();
}

void NotificationClient::broadcastRemoveOrder(const QString &orderNumber, quint16 port) {
    QUdpSocket udp;
    QByteArray datagram = QString("REMOVE_ORDER:%1").arg(orderNumber).toUtf8();
    udp.writeDatagram(datagram, QHostAddress::Broadcast, port);
    qDebug() << "Broadcast REMOVE_ORDER" << orderNumber << "na porcie" << port;
}
