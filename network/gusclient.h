#pragma once
#include <QObject>
#include <QMap>
#include <QProcess>

class GusClient : public QObject {
    Q_OBJECT
public:
    // ***** INTEGRACJA Z PYTHON RegonAPI-1.3.1 *****
    static const QString API_KEY;
    static const bool IS_PRODUCTION;
    
    explicit GusClient(QObject *parent = nullptr);
    ~GusClient();
    
    // Główna metoda pobierania danych firmy
    void fetchCompanyData(const QString &nip);

signals:
    void companyDataReceived(const QMap<QString, QString> &data);
    void errorOccurred(const QString &errorMsg);

private slots:
    void onPythonProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onPythonProcessError(QProcess::ProcessError error);

private:
    void callPythonWrapper(const QString &nip);
    bool copyDirectory(const QString &source, const QString &destination);
};
