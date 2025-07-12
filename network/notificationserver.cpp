#include "notificationserver.h"
#include <QThread>
#include <QDebug>

NotificationServer::NotificationServer(QObject *parent)
    : QTcpServer(parent) {}

void NotificationServer::startServer(quint16 port) {
    if (!listen(QHostAddress::Any, port)) {
        qWarning() << "Nie można uruchomić serwera powiadomień na porcie" << port;
    } else {
        qDebug() << "Serwer powiadomień nasłuchuje na porcie" << port;
    }
}

void NotificationServer::incomingConnection(qintptr socketDescriptor) {
    QTcpSocket *socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);
    connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
        QByteArray data = socket->readAll();
        QString msg = QString::fromUtf8(data).trimmed();
        if (msg.startsWith("ORDER_DONE:")) {
            QString orderNumber = msg.section(':', 1);
            emit orderDoneReceived(orderNumber);
        }
        socket->disconnectFromHost();
        socket->deleteLater();
    });
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
}
