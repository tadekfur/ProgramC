#include "gusclient.h"
#include <QUrl>
#include <QMessageBox>
#include <QRegularExpression>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QCoreApplication>

// ***** INTEGRACJA Z PYTHON RegonAPI-1.3.1 *****
// PRAWDZIWY KLUCZ API GUS - GOTOWY DO TESTOWANIA!
const QString GusClient::API_KEY = "c9f317f699c84d9e9f0f";

// Konfiguracja środowisk - teraz używamy Python RegonAPI
const bool GusClient::IS_PRODUCTION = true;

GusClient::GusClient(QObject *parent) : QObject(parent) {
    qDebug() << "[GUS] Inicjalizacja GusClient z integracją Python RegonAPI-1.3.1";
    qDebug() << "[GUS] Klucz API:" << API_KEY;
    qDebug() << "[GUS] Środowisko produkcyjne:" << IS_PRODUCTION;
}

GusClient::~GusClient() {
    qDebug() << "[GUS] Destruktor GusClient - Python RegonAPI obsługuje zarządzanie sesją";
}

void GusClient::fetchCompanyData(const QString &nip) {
    qDebug() << "[GUS] fetchCompanyData called with NIP:" << nip;
    qDebug() << "[GUS] Używam Python RegonAPI-1.3.1";
    qDebug() << "[GUS] Klucz API:" << API_KEY;
    qDebug() << "[GUS] Środowisko produkcyjne:" << IS_PRODUCTION;
    
    // Usuń wszystkie znaki specjalne
    QString cleanNip = nip;
    cleanNip.remove(QRegularExpression("[^0-9]")); // zostaw tylko cyfry
    if (cleanNip.isEmpty()) {
        emit errorOccurred("Podano pusty numer NIP");
        return;
    }
    if (cleanNip.length() != 10) {
        emit errorOccurred("Nieprawidłowa długość numeru NIP (wymagane 10 cyfr)");
        return;
    }
    
    qDebug() << "[GUS] Oczyszczony NIP:" << cleanNip;
    
    // Wywołaj Python wrapper
    callPythonWrapper(cleanNip);
}

void GusClient::callPythonWrapper(const QString &nip) {
    qDebug() << "[GUS] *** WYWOŁANIE PYTHON RegonAPI WRAPPER ***";
    
    // Możliwe ścieżki do Python wrapper
    QStringList possiblePaths = {
        QDir::currentPath() + "/python/gus_wrapper.py",
        QDir::currentPath() + "/../python/gus_wrapper.py",
        QDir::currentPath() + "/../../python/gus_wrapper.py",
        QDir(QCoreApplication::applicationDirPath()).absolutePath() + "/python/gus_wrapper.py"
    };
    
    QString pythonScript;
    bool found = false;
    
    // Sprawdź wszystkie możliwe ścieżki
    for (const QString &path : possiblePaths) {
        QString normalizedPath = QDir::toNativeSeparators(path);
        qDebug() << "[GUS] Sprawdzam ścieżkę:" << normalizedPath;
        
        if (QFile::exists(normalizedPath)) {
            pythonScript = normalizedPath;
            found = true;
            qDebug() << "[GUS] Znaleziono Python wrapper:" << pythonScript;
            break;
        }
    }
    
    // Jeśli nie znaleziono, skopiuj plik z głównego katalogu projektu do bieżącego katalogu
    if (!found) {
        // Najpierw sprawdź, czy plik istnieje w głównym katalogu projektu
        QString sourcePath = QDir::toNativeSeparators("C:/ProgramC/python/gus_wrapper.py");
        
        if (QFile::exists(sourcePath)) {
            // Utwórz katalog python w bieżącym katalogu
            QDir dir(QDir::currentPath());
            if (!dir.exists("python")) {
                dir.mkdir("python");
            }
            
            // Docelowa ścieżka
            QString targetPath = QDir::toNativeSeparators(QDir::currentPath() + "/python/gus_wrapper.py");
            
            // Skopiuj plik
            if (QFile::copy(sourcePath, targetPath)) {
                qDebug() << "[GUS] Skopiowano Python wrapper z:" << sourcePath << "do:" << targetPath;
                pythonScript = targetPath;
                found = true;
            } else {
                qDebug() << "[GUS] Nie udało się skopiować Python wrapper z:" << sourcePath << "do:" << targetPath;
            }
        }
    }
    
    // Jeśli nadal nie znaleziono, zgłoś błąd
    if (!found) {
        QString errorMsg = "Nie znaleziono Python wrapper. Sprawdzone ścieżki:";
        for (const QString &path : possiblePaths) {
            errorMsg += "\n- " + QDir::toNativeSeparators(path);
        }
        errorMsg += "\n- C:/ProgramC/python/gus_wrapper.py";
        emit errorOccurred(errorMsg);
        return;
    }
    
    // Upewnij się, że biblioteka RegonAPI-1.3.1 jest dostępna w bieżącym katalogu
    QString regonApiDir = QDir::currentPath() + "/RegonAPI-1.3.1";
    QString sourceRegonApiDir = "C:/ProgramC/RegonAPI-1.3.1";
    
    if (!QDir(regonApiDir).exists() && QDir(sourceRegonApiDir).exists()) {
        qDebug() << "[GUS] RegonAPI-1.3.1 nie znaleziony w bieżącym katalogu, próbuję skopiować z:" << sourceRegonApiDir;
        
        // Skopiuj katalog RegonAPI-1.3.1 do bieżącego katalogu
        QDir dir(QDir::currentPath());
        if (copyDirectory(sourceRegonApiDir, regonApiDir)) {
            qDebug() << "[GUS] Skopiowano RegonAPI-1.3.1 do:" << regonApiDir;
        } else {
            qDebug() << "[GUS] Nie udało się skopiować RegonAPI-1.3.1 do:" << regonApiDir;
            qDebug() << "[GUS] Kontynuuję, zakładając że Python znajdzie bibliotekę w oryginalnej lokalizacji";
        }
    }
    
    qDebug() << "[GUS] Użyję Python script:" << pythonScript;
    
    // Przygotuj argumenty
    QStringList arguments;
    arguments << pythonScript;
    arguments << "--nip" << nip;
    arguments << "--api-key" << API_KEY;
    if (IS_PRODUCTION) {
        arguments << "--production";
    }
    arguments << "--debug";  // Włącz debug dla diagnostyki
    
    qDebug() << "[GUS] Python argumenty:" << arguments;
    
    // Utwórz proces Python
    QProcess *pythonProcess = new QProcess(this);
    
    // Połącz sygnały
    connect(pythonProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &GusClient::onPythonProcessFinished);
    connect(pythonProcess, &QProcess::errorOccurred,
            this, &GusClient::onPythonProcessError);
    
    // Uruchom Python - spróbuj różne komendy
    QStringList pythonExecutables = {"python", "python3"};
    QString pythonExecutable;
    
    for (const QString &cmd : pythonExecutables) {
        QProcess testProcess;
        testProcess.start(cmd, QStringList() << "--version");
        if (testProcess.waitForStarted(1000)) {
            testProcess.waitForFinished(1000);
            if (testProcess.exitCode() == 0) {
                pythonExecutable = cmd;
                qDebug() << "[GUS] Znaleziono działającą komendę Python:" << cmd;
                break;
            }
        }
    }
    
    if (pythonExecutable.isEmpty()) {
        pythonExecutable = "python"; // Domyślnie
    }
    
    // Ustaw kodowanie dla procesu Python
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PYTHONIOENCODING", "utf-8");
    pythonProcess->setProcessEnvironment(env);
    
    qDebug() << "[GUS] Uruchamiam:" << pythonExecutable << arguments;
    pythonProcess->start(pythonExecutable, arguments);
    
    if (!pythonProcess->waitForStarted(5000)) {
        emit errorOccurred("Nie udało się uruchomić Python: " + pythonProcess->errorString());
        pythonProcess->deleteLater();
        return;
    }
    
    qDebug() << "[GUS] Python proces uruchomiony, oczekuję na wynik...";
}

void GusClient::onPythonProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    QProcess *process = qobject_cast<QProcess*>(sender());
    if (!process) {
        emit errorOccurred("Błąd wewnętrzny: brak odniesienia do procesu Python");
        return;
    }
    
    qDebug() << "[GUS] Python proces zakończony, kod:" << exitCode << "status:" << exitStatus;
    
    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        QString errorOutput = process->readAllStandardError();
        QString stdoutOutput = process->readAllStandardOutput();
        
        qDebug() << "[GUS] Python błąd stderr:" << errorOutput;
        qDebug() << "[GUS] Python stdout przy błędzie:" << stdoutOutput;
        
        // Spróbuj parsować stdout jako JSON, może zawierać informacje o błędzie
        QString errorMessage = "Python proces zakończony błędem (kod " + QString::number(exitCode) + ")";
        
        if (!stdoutOutput.isEmpty()) {
            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(stdoutOutput.toUtf8(), &parseError);
            
            if (parseError.error == QJsonParseError::NoError) {
                QJsonObject result = jsonDoc.object();
                if (result.contains("error")) {
                    errorMessage += ": " + result["error"].toString();
                }
            }
        }
        
        if (!errorOutput.isEmpty()) {
            errorMessage += "\nBłąd: " + errorOutput;
        }
        
        // Sprawdź, czy został utworzony plik gus_error.log
        QStringList logPaths = {
            QDir::currentPath() + "/python/gus_error.log",
            QDir::currentPath() + "/../python/gus_error.log",
            QDir::currentPath() + "/../../python/gus_error.log",
            QDir(QCoreApplication::applicationDirPath()).absolutePath() + "/python/gus_error.log"
        };
        
        for (const QString &logPath : logPaths) {
            QFile logFile(logPath);
            if (logFile.exists() && logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString logContent = QString::fromUtf8(logFile.readAll());
                logFile.close();
                qDebug() << "[GUS] Znaleziono plik logów:" << logPath;
                qDebug() << "[GUS] Zawartość pliku logów:" << logContent;
                errorMessage += "\n\nSzczegóły w pliku log: " + logContent;
                break;
            }
        }
        
        emit errorOccurred(errorMessage);
        process->deleteLater();
        return;
    }
    
    // Odczytaj wynik JSON z poprawnym kodowaniem UTF-8
    QByteArray jsonOutput = process->readAllStandardOutput();
    QString jsonString = QString::fromUtf8(jsonOutput);
    
    // Wypisz w logach skróconą wersję (pierwsze 200 znaków)
    if (jsonString.length() > 200) {
        qDebug() << "[GUS] Python wynik JSON (pierwsze 200 znaków):" << jsonString.left(200) + "...";
    } else {
        qDebug() << "[GUS] Python wynik JSON:" << jsonString;
    }
    
    // Parsuj JSON
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonOutput, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        emit errorOccurred("Błąd parsowania JSON: " + parseError.errorString());
        process->deleteLater();
        return;
    }
    
    QJsonObject result = jsonDoc.object();
    
    // Sprawdź czy operacja się powiodła
    if (!result["success"].toBool()) {
        QString error = result["error"].toString();
        if (error.isEmpty()) {
            error = "Nieznany błąd Python RegonAPI";
        }
        emit errorOccurred("RegonAPI błąd: " + error);
        process->deleteLater();
        return;
    }
    
    // Wyciągnij dane firmy
    QJsonObject dataObj = result["data"].toObject();
    QMap<QString, QString> companyData;
    
    for (auto it = dataObj.begin(); it != dataObj.end(); ++it) {
        companyData[it.key()] = it.value().toString();
    }
    
    qDebug() << "[GUS] *** SUKCES! OTRZYMANO DANE Z PYTHON RegonAPI ***";
    qDebug() << "[GUS] Liczba pól:" << companyData.size();
    
    // Loguj wszystkie otrzymane dane
    for (auto it = companyData.begin(); it != companyData.end(); ++it) {
        qDebug() << "[GUS] " << it.key() << "=" << it.value();
    }
    
    emit companyDataReceived(companyData);
    process->deleteLater();
}

bool GusClient::copyDirectory(const QString &source, const QString &destination) {
    QDir sourceDir(source);
    if (!sourceDir.exists())
        return false;
    
    QDir destDir(destination);
    if (!destDir.exists()) {
        if (!destDir.mkpath("."))
            return false;
    }
    
    bool success = true;
    
    // Kopiowanie plików z katalogu źródłowego
    QStringList files = sourceDir.entryList(QDir::Files);
    for (const QString &file : files) {
        QString srcFilePath = source + "/" + file;
        QString destFilePath = destination + "/" + file;
        success &= QFile::copy(srcFilePath, destFilePath);
    }
    
    // Rekurencyjne kopiowanie podkatalogów
    QStringList dirs = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for (const QString &dir : dirs) {
        QString srcDirPath = source + "/" + dir;
        QString destDirPath = destination + "/" + dir;
        success &= copyDirectory(srcDirPath, destDirPath);
    }
    
    return success;
}

void GusClient::onPythonProcessError(QProcess::ProcessError error) {
    QProcess *process = qobject_cast<QProcess*>(sender());
    QString errorString;
    
    switch (error) {
        case QProcess::FailedToStart:
            errorString = "Nie udało się uruchomić Python - sprawdź czy Python jest zainstalowany";
            break;
        case QProcess::Crashed:
            errorString = "Python proces uległ awarii";
            break;
        case QProcess::Timedout:
            errorString = "Python proces przekroczył limit czasu";
            break;
        case QProcess::ReadError:
            errorString = "Błąd odczytu z Python procesu";
            break;
        case QProcess::WriteError:
            errorString = "Błąd zapisu do Python procesu";
            break;
        default:
            errorString = "Nieznany błąd Python procesu";
            break;
    }
    
    qDebug() << "[GUS] Python proces błąd:" << errorString;
    emit errorOccurred(errorString);
    
    if (process) {
        process->deleteLater();
    }
}
