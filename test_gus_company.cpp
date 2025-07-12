#include "network/gusclient.h"
#include <QApplication>
#include <QDebug>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    qDebug() << "[TEST] Rozpoczynam test GUS z NIP-em firmy...";
    
    GusClient* gusClient = new GusClient();
    
    // Połącz sygnały do debugowania
    QObject::connect(gusClient, &GusClient::companyDataReceived, [](const QMap<QString, QString> &data) {
        qDebug() << "[TEST] *** SUKCES! OTRZYMANO DANE FIRMY ***";
        for (auto it = data.begin(); it != data.end(); ++it) {
            qDebug() << "[TEST] " << it.key() << ":" << it.value();
        }
        QApplication::quit();
    });
    
    QObject::connect(gusClient, &GusClient::errorOccurred, [](const QString &error) {
        qDebug() << "[TEST] *** BŁĄD GUS ***" << error;
        QApplication::quit();
    });
    
    // Rozpocznij test z NIP-em firmy
    QTimer::singleShot(500, [gusClient]() {
        qDebug() << "[TEST] Testowanie z NIP-em firmy: 6381016402";
        gusClient->fetchCompanyData("6381016402");
    });
    
    // Timeout po 30 sekundach
    QTimer::singleShot(30000, []() {
        qDebug() << "[TEST] TIMEOUT - kończę test";
        QApplication::quit();
    });
    
    return app.exec();
}
