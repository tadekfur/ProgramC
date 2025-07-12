#include "email_sender.h"
#include "email_config.h"
#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDateTime>
#include <QDebug>
#include <QSslConfiguration>
#include <QTimer>

EmailSender::EmailSender(QObject *parent)
    : QObject(parent), m_socket(nullptr), m_timeoutTimer(new QTimer(this)), m_state(ConnectingState)
{
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(30000); // 30 sekund timeout
    connect(m_timeoutTimer, &QTimer::timeout, this, &EmailSender::onTimeout);
}

void EmailSender::sendEmail(const EmailData &emailData) {
    qDebug() << "[EmailSender] === ROZPOCZYNANIE WYSYŁANIA E-MAILA ===";
    qDebug() << "[EmailSender] Do:" << emailData.toEmail;
    qDebug() << "[EmailSender] Temat:" << emailData.subject;
    qDebug() << "[EmailSender] Załącznik:" << emailData.attachmentPath;
    
    // Sprawdź konfigurację SMTP szczegółowo
    qDebug() << "[EmailSender] SMTP Server:" << EmailConfig::SMTP_SERVER;
    qDebug() << "[EmailSender] SMTP Port:" << EmailConfig::SMTP_PORT;
    qDebug() << "[EmailSender] SMTP Username:" << EmailConfig::SMTP_USERNAME;
    qDebug() << "[EmailSender] Czy konfiguracja jest poprawna:" << EmailConfig::isConfigValid();
    
    if (!EmailConfig::isConfigValid()) {
        qDebug() << "[EmailSender] Błąd: Niepoprawna konfiguracja SMTP";
        emit emailSent(false, "Niepoprawna konfiguracja SMTP");
        return;
    }

    if (emailData.toEmail.isEmpty()) {
        qDebug() << "[EmailSender] Błąd: Brak adresu e-mail odbiorcy";
        emit emailSent(false, "Brak adresu e-mail odbiorcy");
        return;
    }

    m_emailData = emailData;
    m_state = ConnectingState;
    m_finished = false;  // Reset flagi zakończenia

    // Utwórz nowe połączenie
    if (m_socket) {
        qDebug() << "[EmailSender] Usuwanie starego połączenia";
        m_socket->deleteLater();
    }
    
    qDebug() << "[EmailSender] Tworzenie nowego QSslSocket (do STARTTLS)";
    m_socket = new QSslSocket(this);
    
    // Sprawdź obsługę SSL
    qDebug() << "[EmailSender] Qt SSL Support:" << QSslSocket::supportsSsl();
    
    // Połącz sygnały SSL
    QSslSocket* sslSocket = qobject_cast<QSslSocket*>(m_socket);
    if (sslSocket) {
        connect(sslSocket, &QSslSocket::sslErrors, this, [this](const QList<QSslError> &errors) {
            qDebug() << "[EmailSender] SYGNAŁ: sslErrors - błędy SSL:" << errors.size();
            for (const QSslError &error : errors) {
                qDebug() << "[EmailSender] SSL Error:" << error.errorString();
            }
            // Ignoruj błędy SSL dla uproszczenia
            qobject_cast<QSslSocket*>(m_socket)->ignoreSslErrors();
        });
        
        connect(sslSocket, &QSslSocket::encrypted, this, [this]() {
            qDebug() << "[EmailSender] SYGNAŁ: encrypted - połączenie szyfrowane TLS nawiązane";
        });
    }
    
    // Połącz wszystkie sygnały z dodatkowymi debugami
    connect(m_socket, &QTcpSocket::stateChanged, this, [this](QAbstractSocket::SocketState state) {
        qDebug() << "[EmailSender] SYGNAŁ: stateChanged - nowy stan:" << state;
    });
    
    connect(m_socket, &QTcpSocket::connected, this, [this]() {
        qDebug() << "[EmailSender] SYGNAŁ: connected - połączono z serwerem";
        onConnected();
    });
    
    connect(m_socket, &QTcpSocket::readyRead, this, [this]() {
        qDebug() << "[EmailSender] SYGNAŁ: readyRead - dane gotowe do odczytu";
        onReadyRead();
    });
    
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, [this](QAbstractSocket::SocketError error) {
        qDebug() << "[EmailSender] SYGNAŁ: errorOccurred - błąd:" << error;
        onError(error);
    });
    
    connect(m_socket, &QTcpSocket::disconnected, this, [this]() {
        qDebug() << "[EmailSender] SYGNAŁ: disconnected - rozłączono";
    });

    qDebug() << "[EmailSender] Rozpoczynanie połączenia z" << EmailConfig::SMTP_SERVER << ":" << EmailConfig::SMTP_PORT;
    qDebug() << "[EmailSender] Stan przed połączeniem:" << m_socket->state();
    
    m_timeoutTimer->start();
    m_socket->connectToHost(EmailConfig::SMTP_SERVER, EmailConfig::SMTP_PORT);
    
    qDebug() << "[EmailSender] Stan po wywołaniu connectToHost:" << m_socket->state();
    qDebug() << "[EmailSender] Czekanie na połączenie...";
}

void EmailSender::onConnected() {
    qDebug() << "[EmailSender] Połączono z serwerem SMTP - oczekiwanie na powitalną wiadomość";
    // Nie zmieniamy stanu tutaj - czekamy na odpowiedź 220 w processResponse
}

void EmailSender::onReadyRead() {
    QByteArray data = m_socket->readAll();
    QString response = QString::fromUtf8(data).trimmed();
    m_lastResponse = response;
    
    qDebug() << "[EmailSender] Odpowiedź serwera:" << response;
    
    processResponse(response);
}

void EmailSender::processResponse(const QString &response) {
    qDebug() << "[EmailSender] Przetwarzanie odpowiedzi w stanie:" << m_state << "Odpowiedź:" << response;
    
    // Sprawdź czy odpowiedź jest pomyślna
    bool ok = false;
    int responseCode = response.left(3).toInt();
    
    // Sprawdź czy to jest wieloliniowa odpowiedź (kod-myślnik vs kod-spacja)
    // Wieloliniowa odpowiedź kończy się linią z kodem-spacja
    QStringList lines = response.split("\r\n", Qt::SkipEmptyParts);
    bool isCompleteMultiline = true;
    
    for (const QString &line : lines) {
        if (line.length() > 3) {
            // Jeśli jakakolwiek linia ma myślnik po kodzie, to jeszcze nie koniec
            if (line.at(3) == '-') {
                // Ale sprawdź czy ostatnia linia nie ma spacji (czyli koniec)
                QString lastLine = lines.last();
                if (lastLine.length() > 3 && lastLine.at(3) == ' ') {
                    isCompleteMultiline = true; // Kompletna wieloliniowa odpowiedź
                } else {
                    isCompleteMultiline = false; // Jeszcze czekamy na więcej
                }
                break;
            }
        }
    }
    
    if (!isCompleteMultiline) {
        qDebug() << "[EmailSender] Niekompletna wieloliniowa odpowiedź - oczekiwanie na zakończenie";
        return; // Czekamy na pełną odpowiedź
    }
    
    // Użyj kodu z ostatniej linii (która powinna mieć spację po kodzie)
    QString lastLine = lines.last();
    responseCode = lastLine.left(3).toInt();
    
    qDebug() << "[EmailSender] Kompletna odpowiedź, kod:" << responseCode;
    
    switch (responseCode) {
    case 220: // Service ready / Ready to start TLS
    case 235: // Authentication successful  
    case 250: // Requested mail action okay, completed
    case 334: // Server challenge (base64)
    case 354: // Start mail input
    case 221: // Service closing transmission channel
        ok = true;
        break;
    default:
        ok = false;
        break;
    }
              
    if (!ok) {
        finishWithError(QString("Błąd serwera SMTP (kod %1): %2").arg(responseCode).arg(response));
        return;
    }

    switch (m_state) {
    case ConnectingState:
        if (responseCode == 220) {
            qDebug() << "[EmailSender] Otrzymano powitanie serwera, wysyłanie EHLO";
            sendCommand("EHLO " + EmailConfig::SMTP_SERVER);
            m_state = EhloState;
        }
        break;
        
    case EhloState:
        if (responseCode == 250) {
            qDebug() << "[EmailSender] EHLO pomyślne";
            
            // Sprawdź czy serwer obsługuje STARTTLS
            if (response.contains("STARTTLS", Qt::CaseInsensitive)) {
                qDebug() << "[EmailSender] Serwer obsługuje STARTTLS, rozpoczynanie szyfrowania";
                sendCommand("STARTTLS");
                m_state = StartTlsState;
            } else if (response.contains("AUTH", Qt::CaseInsensitive)) {
                qDebug() << "[EmailSender] Serwer obsługuje autentykację bez STARTTLS, rozpoczynanie AUTH LOGIN";
                sendCommand("AUTH LOGIN");
                m_state = AuthState;
            } else {
                qDebug() << "[EmailSender] Serwer nie wymaga autentykacji ani szyfrowania, przejście do MAIL FROM";
                sendCommand("MAIL FROM:<" + EmailConfig::SMTP_USERNAME + ">");
                m_state = MailFromState;
            }
        }
        break;
        
    case StartTlsState:
        if (responseCode == 220) {
            qDebug() << "[EmailSender] Rozpoczynanie szyfrowania TLS";
            // Konwertuj QTcpSocket na QSslSocket
            QSslSocket* sslSocket = qobject_cast<QSslSocket*>(m_socket);
            if (sslSocket) {
                sslSocket->startClientEncryption();
                // Czekamy na nawiązanie TLS, potem wysyłamy EHLO ponownie
                QTimer::singleShot(2000, this, [this]() {
                    qDebug() << "[EmailSender] TLS nawiązane, wysyłanie EHLO ponownie";
                    sendCommand("EHLO " + EmailConfig::SMTP_SERVER);
                    m_state = EhloAfterTlsState;
                });
            } else {
                qDebug() << "[EmailSender] BŁĄD: Socket nie jest QSslSocket, nie można uruchomić TLS";
                finishWithError("Błąd TLS: Nieprawidłowy typ socket");
            }
        }
        break;
        
    case EhloAfterTlsState:
        if (responseCode == 250) {
            qDebug() << "[EmailSender] EHLO po TLS pomyślne, rozpoczynanie autentykacji";
            sendCommand("AUTH LOGIN");
            m_state = AuthState;
        }
        break;
        
    case AuthState:
        if (responseCode == 334) {
            qDebug() << "[EmailSender] Serwer wymaga nazwy użytkownika";
            sendCommand(encodeBase64(EmailConfig::SMTP_USERNAME));
            m_state = AuthUserState;
        }
        break;
        
    case AuthUserState:
        if (responseCode == 334) {
            qDebug() << "[EmailSender] Serwer wymaga hasła";
            sendCommand(encodeBase64(EmailConfig::SMTP_PASSWORD));
            m_state = AuthPassState;
        }
        break;
        
    case AuthPassState:
        if (responseCode == 235) {
            qDebug() << "[EmailSender] Autentykacja pomyślna, wysyłanie MAIL FROM";
            sendCommand("MAIL FROM:<" + EmailConfig::SMTP_USERNAME + ">");
            m_state = MailFromState;
        }
        break;
        
    case MailFromState:
        if (responseCode == 250) {
            qDebug() << "[EmailSender] MAIL FROM zaakceptowane, wysyłanie RCPT TO:" << m_emailData.toEmail;
            sendCommand("RCPT TO:<" + m_emailData.toEmail + ">");
            m_state = RcptToState;
        }
        break;
        
    case RcptToState:
        if (responseCode == 250) {
            qDebug() << "[EmailSender] RCPT TO zaakceptowane, rozpoczynanie DATA";
            sendCommand("DATA");
            m_state = DataState;
        }
        break;
        
    case DataState:
        if (responseCode == 354) {
            qDebug() << "[EmailSender] Serwer gotowy na dane e-maila";
            QString emailBody = createEmailBody();
            qDebug() << "[EmailSender] Wysyłanie treści e-maila (" << emailBody.length() << " znaków)";
            sendCommand(emailBody);
            sendCommand(".");
            m_state = BodyState;
        }
        break;
        
    case BodyState:
        if (responseCode == 250) {
            qDebug() << "[EmailSender] E-mail zaakceptowany przez serwer, wysyłanie QUIT";
            sendCommand("QUIT");
            m_state = QuitState;
        }
        break;
        
    case QuitState:
        if (responseCode == 221 || responseCode == 250) {
            qDebug() << "[EmailSender] Zamykanie połączenia";
            finishWithSuccess();
        }
        break;
        
    default:
        qDebug() << "[EmailSender] Nieznany stan:" << m_state;
        break;
    }
}

QString EmailSender::createEmailBody() {
    qDebug() << "[EmailSender] === TWORZENIE TREŚCI E-MAILA ===";
    
    QString boundary = "----=_NextPart_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    qDebug() << "[EmailSender] Boundary:" << boundary;
    
    QString body;
    
    // Nagłówki podstawowe - bez polskich znaków w nazwie nadawcy
    body += "From: Termedialabels <" + EmailConfig::SMTP_USERNAME + ">\r\n";
    body += "To: " + m_emailData.toName + " <" + m_emailData.toEmail + ">\r\n";
    body += "Reply-To: Biuro Obslugi Klienta <bok@termedialabels.pl>\r\n";
    body += "Return-Path: <" + EmailConfig::SMTP_USERNAME + ">\r\n";
    
    // Lepsze kodowanie tematu
    QString encodedSubject = m_emailData.subject;
    bool hasNonAscii = false;
    for (const QChar &ch : encodedSubject) {
        if (ch.unicode() > 127) {
            hasNonAscii = true;
            break;
        }
    }
    
    if (hasNonAscii) {
        encodedSubject = "=?UTF-8?B?" + m_emailData.subject.toUtf8().toBase64() + "?=";
    }
    body += "Subject: " + encodedSubject + "\r\n";
    
    // Nagłówki antyspamowe
    body += "MIME-Version: 1.0\r\n";
    body += "Date: " + QDateTime::currentDateTime().toString("ddd, d MMM yyyy hh:mm:ss +0100") + "\r\n";
    body += "Message-ID: <" + QString::number(QDateTime::currentMSecsSinceEpoch()) + "@termedialabels.pl>\r\n";
    body += "X-Mailer: EtykietyManager 1.0\r\n";
    body += "X-Priority: 3 (Normal)\r\n";
    body += "X-MSMail-Priority: Normal\r\n";
    body += "Importance: Normal\r\n";
    body += "Auto-Submitted: auto-generated\r\n";
    body += "X-Auto-Response-Suppress: All\r\n";
    body += "Precedence: bulk\r\n";
    body += "Content-Type: multipart/mixed; boundary=\"" + boundary + "\"\r\n";
    body += "\r\n";
    
    qDebug() << "[EmailSender] Nagłówki utworzone";
    
    // Część tekstowa - używamy zwykłego tekstu zamiast base64
    body += "--" + boundary + "\r\n";
    body += "Content-Type: text/plain; charset=UTF-8\r\n";
    body += "Content-Transfer-Encoding: 8bit\r\n";
    body += "\r\n";
    body += m_emailData.body + "\r\n";
    
    qDebug() << "[EmailSender] Część tekstowa dodana";
    
    // Załącznik PDF (jeśli istnieje)
    if (!m_emailData.attachmentPath.isEmpty() && QFile::exists(m_emailData.attachmentPath)) {
        qDebug() << "[EmailSender] Dodawanie załącznika:" << m_emailData.attachmentPath;
        QByteArray fileData = readFileToBase64(m_emailData.attachmentPath);
        
        if (!fileData.isEmpty()) {
            body += "--" + boundary + "\r\n";
            body += "Content-Type: application/pdf; name=\"" + m_emailData.attachmentName + "\"\r\n";
            body += "Content-Transfer-Encoding: base64\r\n";
            body += "Content-Disposition: attachment; filename=\"" + m_emailData.attachmentName + "\"\r\n";
            body += "Content-Description: Potwierdzenie zamowienia\r\n";
            body += "\r\n";
            
            // Podziel base64 na linie po 76 znaków (standard RFC)
            QString base64String = QString::fromLatin1(fileData);
            for (int i = 0; i < base64String.length(); i += 76) {
                body += base64String.mid(i, 76) + "\r\n";
            }
            
            qDebug() << "[EmailSender] Załącznik dodany, rozmiar:" << fileData.size() << "bajtów (base64)";
        } else {
            qDebug() << "[EmailSender] Błąd: Nie można odczytać załącznika";
        }
    } else {
        qDebug() << "[EmailSender] Brak załącznika lub plik nie istnieje:" << m_emailData.attachmentPath;
    }
    
    body += "--" + boundary + "--\r\n";
    
    qDebug() << "[EmailSender] Treść e-maila utworzona, całkowity rozmiar:" << body.length() << "znaków";
    
    return body;
}

void EmailSender::sendCommand(const QString &command) {
    if (command.contains("AUTH") || command.contains(EmailConfig::SMTP_PASSWORD)) {
        qDebug() << "[EmailSender] Wysyłanie: [UKRYTE - hasło/auth]";
    } else {
        QString displayCommand = command.left(100) + (command.length() > 100 ? "..." : "");
        qDebug() << "[EmailSender] Wysyłanie:" << displayCommand;
    }
    
    QByteArray data = (command + "\r\n").toUtf8();
    qint64 written = m_socket->write(data);
    qDebug() << "[EmailSender] Zapisano" << written << "bajtów z" << data.size();
    
    // Dodaj więcej czasu na wysłanie danych
    m_socket->flush();
    m_socket->waitForBytesWritten(5000); // Czekaj do 5 sekund na wysłanie
}

void EmailSender::onError(QAbstractSocket::SocketError error) {
    qDebug() << "[EmailSender] onError wywołane - błąd:" << error << "Stan:" << m_state << "Finished:" << m_finished;
    
    // Jeśli proces został już zakończony, ignoruj dodatkowe błędy
    if (m_finished) {
        qDebug() << "[EmailSender] Ignorowanie błędu po zakończeniu procesu:" << error;
        return;
    }
    
    // Jeśli e-mail już został wysłany pomyślnie (stan FinishedState), ignoruj błędy zamknięcia połączenia
    if (m_state == FinishedState || 
        (m_state == QuitState && (error == QAbstractSocket::RemoteHostClosedError || 
                                  error == QAbstractSocket::SocketTimeoutError))) {
        qDebug() << "[EmailSender] Ignorowanie błędu zamknięcia po pomyślnym wysłaniu:" << error;
        return;
    }
    
    QString errorString = m_socket->errorString();
    qDebug() << "[EmailSender] Błąd połączenia:" << error << errorString;
    finishWithError("Błąd połączenia: " + errorString);
}

void EmailSender::onTimeout() {
    qDebug() << "[EmailSender] === TIMEOUT DIAGNOSTYKA ===";
    qDebug() << "[EmailSender] Stan połączenia:" << (m_socket ? m_socket->state() : -1);
    qDebug() << "[EmailSender] Ostatni stan EmailSender:" << m_state;
    qDebug() << "[EmailSender] Ostatnia odpowiedź serwera:" << m_lastResponse;
    qDebug() << "[EmailSender] Bytes dostępne do odczytu:" << (m_socket ? m_socket->bytesAvailable() : -1);
    qDebug() << "[EmailSender] Błąd socket:" << (m_socket ? m_socket->errorString() : "Brak socket");
    
    finishWithError("Przekroczono limit czasu połączenia (30s)");
}

void EmailSender::finishWithError(const QString &error) {
    if (m_finished) {
        qDebug() << "[EmailSender] Już zakończone - ignorowanie błędu:" << error;
        return;
    }
    
    qDebug() << "[EmailSender] Błąd:" << error;
    qDebug() << "[EmailSender] EMIT emailSent(false, błąd)";
    m_finished = true;
    m_timeoutTimer->stop();
    if (m_socket) {
        m_socket->disconnectFromHost();
    }
    emit emailSent(false, error);
}

void EmailSender::finishWithSuccess() {
    if (m_finished) {
        qDebug() << "[EmailSender] Już zakończone - ignorowanie sukcesu";
        return;
    }
    
    qDebug() << "[EmailSender] === E-mail wysłany pomyślnie ===";
    qDebug() << "[EmailSender] EMIT emailSent(true, sukces)";
    m_finished = true;
    m_state = FinishedState;  // Zmień stan na zakończony
    m_timeoutTimer->stop();
    
    // Odłącz wszystkie sygnały socket, żeby uniknąć dodatkowych wywołań
    if (m_socket) {
        m_socket->disconnect();
        m_socket->disconnectFromHost();
    }
    
    emit emailSent(true, "E-mail został wysłany pomyślnie");
}

QString EmailSender::encodeBase64(const QString &text) {
    return text.toUtf8().toBase64();
}

QByteArray EmailSender::readFileToBase64(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "[EmailSender] Nie można otworzyć pliku:" << filePath;
        return QByteArray();
    }
    
    QByteArray fileData = file.readAll();
    return fileData.toBase64();
}
