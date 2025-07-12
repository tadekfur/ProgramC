#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QSslSocket>
#include <QString>
#include <QByteArray>
#include <QTimer>

class EmailSender : public QObject {
    Q_OBJECT

public:
    struct EmailData {
        // Dane odbiorcy
        QString toEmail;
        QString toName;
        
        // Dane nadawcy
        QString senderEmail;
        QString senderName;
        
        // Dane wiadomości
        QString subject;
        QString body;
        QString attachmentPath;
        QString attachmentName;
        
        // Konfiguracja serwera SMTP
        QString smtpServer;
        int smtpPort = 0;
        QString smtpUser;
        QString smtpPassword;
        QString smtpEncryption; // "SSL", "TLS" lub puste dla braku szyfrowania
    };

    explicit EmailSender(QObject *parent = nullptr);
    
    void sendEmail(const EmailData &emailData);

signals:
    void emailSent(bool success, const QString &message);

private slots:
    void onConnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);
    void onTimeout();

private:
    void sendCommand(const QString &command);
    void processResponse(const QString &response);
    void nextStep();
    void finishWithError(const QString &error);
    void finishWithSuccess();
    QString encodeBase64(const QString &text);
    QByteArray readFileToBase64(const QString &filePath);
    QString createEmailBody();

    QTcpSocket *m_socket;
    QTimer *m_timeoutTimer;
    EmailData m_emailData;
    
    enum State {
        ConnectingState,
        EhloState,
        StartTlsState,
        EhloAfterTlsState,
        AuthState,
        AuthUserState,
        AuthPassState,
        MailFromState,
        RcptToState,
        DataState,
        BodyState,
        QuitState,
        FinishedState
    };
    
    State m_state;
    QString m_lastResponse;
    bool m_finished = false;  // Czy proces wysyłania został już zakończony
};
