#include "networkmanager.h"
#include <QtNetwork/QUdpSocket>
#include <QTimer>

NetworkManager::NetworkManager(QObject *parent) : QObject(parent) {
    // ...
}

bool NetworkManager::broadcastNotification(const QString &message, const QString &host, int port) {
    QUdpSocket socket;
    QByteArray data = message.toUtf8();
    qint64 sent = socket.writeDatagram(data, QHostAddress(host), port);
    return sent == data.size();
}

void NetworkManager::broadcastNotificationAsync(const QString &message, const QString &host, int port) {
    QUdpSocket *socket = new QUdpSocket(this);
    QByteArray data = message.toUtf8();
    qint64 sent = socket->writeDatagram(data, QHostAddress(host), port);
    if (sent == data.size()) {
        emit notificationSent();
    } else {
        emit networkError(tr("Błąd wysyłania powiadomienia: %1").arg(socket->errorString()));
    }
    socket->deleteLater();
}
