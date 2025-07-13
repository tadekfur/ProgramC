#include "python_pdf_generator.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTextStream>
#include <QStringConverter>
#include <QDateTime>
#include <QFile>
#include <QMetaType>

PythonPdfGenerator::PythonPdfGenerator(QObject *parent)
    : QObject(parent)
{
}

bool PythonPdfGenerator::generateOrderConfirmation(const QMap<QString, QVariant>& orderData,
                                                 const QMap<QString, QVariant>& clientData,
                                                 const QVector<QMap<QString, QVariant>>& orderItems,
                                                 const QString& outputPath)
{
    qDebug() << "[PythonPdfGenerator] Generowanie potwierdzenia zamówienia przy użyciu skryptu Python";
    
    // Przygotuj dane JSON
    QJsonObject data;
    data["order"] = orderDataToJson(orderData);
    data["client"] = clientDataToJson(clientData);
    data["orderItems"] = orderItemsToJson(orderItems);
    
    return executePythonScript("order_confirmation.py", data, outputPath);
}

bool PythonPdfGenerator::generateProductionTicket(const QMap<QString, QVariant>& orderData,
                                                const QMap<QString, QVariant>& clientData,
                                                const QVector<QMap<QString, QVariant>>& orderItems,
                                                const QString& outputPath)
{
    qDebug() << "[PythonPdfGenerator] Generowanie zlecenia produkcyjnego przy użyciu skryptu Python";
    
    // Przygotuj dane JSON
    QJsonObject data;
    data["order"] = orderDataToJson(orderData);
    data["client"] = clientDataToJson(clientData);
    data["orderItems"] = orderItemsToJson(orderItems);
    
    return executePythonScript("production_ticket.py", data, outputPath);
}

QJsonObject PythonPdfGenerator::orderDataToJson(const QMap<QString, QVariant>& orderData)
{
    QJsonObject json;
    for (auto it = orderData.constBegin(); it != orderData.constEnd(); ++it) {
        QVariant value = it.value();
        
        // Konwertuj różne typy Qt na JSON
        if (value.typeId() == QMetaType::QString) {
            json[it.key()] = value.toString();
        } else if (value.typeId() == QMetaType::Int) {
            json[it.key()] = value.toInt();
        } else if (value.typeId() == QMetaType::Double) {
            json[it.key()] = value.toDouble();
        } else if (value.typeId() == QMetaType::Bool) {
            json[it.key()] = value.toBool();
        } else if (value.typeId() == QMetaType::QDate) {
            json[it.key()] = value.toDate().toString(Qt::ISODate);
        } else if (value.typeId() == QMetaType::QDateTime) {
            json[it.key()] = value.toDateTime().toString(Qt::ISODate);
        } else {
            json[it.key()] = value.toString();
        }
    }
    return json;
}

QJsonObject PythonPdfGenerator::clientDataToJson(const QMap<QString, QVariant>& clientData)
{
    QJsonObject json;
    for (auto it = clientData.constBegin(); it != clientData.constEnd(); ++it) {
        QVariant value = it.value();
        
        if (value.typeId() == QMetaType::QString) {
            json[it.key()] = value.toString();
        } else if (value.typeId() == QMetaType::Int) {
            json[it.key()] = value.toInt();
        } else if (value.typeId() == QMetaType::Double) {
            json[it.key()] = value.toDouble();
        } else if (value.typeId() == QMetaType::Bool) {
            json[it.key()] = value.toBool();
        } else {
            json[it.key()] = value.toString();
        }
    }
    return json;
}

QJsonArray PythonPdfGenerator::orderItemsToJson(const QVector<QMap<QString, QVariant>>& orderItems)
{
    QJsonArray jsonArray;
    
    qDebug() << "[PythonPdfGenerator] Przetwarzanie" << orderItems.size() << "pozycji zamówienia";
    
    for (const QMap<QString, QVariant>& item : orderItems) {
        QJsonObject itemJson;
        
        // Debug: wyświetl dostępne pola
        qDebug() << "[PythonPdfGenerator] Pola w order item:";
        for (auto it = item.constBegin(); it != item.constEnd(); ++it) {
            qDebug() << "  " << it.key() << ":" << it.value().toString();
        }
        
        for (auto it = item.constBegin(); it != item.constEnd(); ++it) {
            QVariant value = it.value();
            
            if (value.typeId() == QMetaType::QString) {
                itemJson[it.key()] = value.toString();
            } else if (value.typeId() == QMetaType::Int) {
                itemJson[it.key()] = value.toInt();
            } else if (value.typeId() == QMetaType::Double) {
                itemJson[it.key()] = value.toDouble();
            } else if (value.typeId() == QMetaType::Bool) {
                itemJson[it.key()] = value.toBool();
            } else {
                itemJson[it.key()] = value.toString();
            }
        }
        jsonArray.append(itemJson);
    }
    
    return jsonArray;
}

bool PythonPdfGenerator::executePythonScript(const QString& scriptName, 
                                           const QJsonObject& data, 
                                           const QString& outputPath)
{
    m_lastError.clear();
    
    // Lista potencjalnych lokalizacji skryptów Python
    QStringList possiblePaths;
    
    // Katalog aplikacji / utils
    QString appDir = QCoreApplication::applicationDirPath();
    possiblePaths << appDir + "/utils/" + scriptName;
    
    // Względny utils/
    possiblePaths << "utils/" + scriptName;
    
    // Katalog roboczy / utils
    QString currentDir = QDir::currentPath();
    possiblePaths << currentDir + "/utils/" + scriptName;
    
    // Katalog źródłowy projektu
    possiblePaths << currentDir + "/../utils/" + scriptName;
    possiblePaths << currentDir + "/../../utils/" + scriptName;
    possiblePaths << "c:/ProgramC/utils/" + scriptName;
    
    // Sprawdź wszystkie możliwe ścieżki
    QString scriptPath;
    bool found = false;
    
    qDebug() << "[PythonPdfGenerator] Szukam skryptu:" << scriptName;
    for (const QString &path : possiblePaths) {
        qDebug() << "[PythonPdfGenerator] Sprawdzam:" << path;
        if (QDir().exists(path)) {
            scriptPath = path;
            found = true;
            qDebug() << "[PythonPdfGenerator] Znaleziono skrypt w:" << scriptPath;
            break;
        }
    }
    
    if (!found) {
        m_lastError = QString("Nie można znaleźć skryptu: %1. Sprawdzono lokalizacje: %2")
                      .arg(scriptName)
                      .arg(possiblePaths.join(", "));
        qDebug() << "[PythonPdfGenerator] Błąd:" << m_lastError;
        return false;
    }
    
    qDebug() << "[PythonPdfGenerator] Znaleziono skrypt w:" << scriptPath;
    
    // Utwórz tymczasowy plik JSON z danymi
    // Zamiast QTemporaryFile używamy zwykłego pliku aby uniknąć problemów z automatycznym zamykaniem
    QString tempFilePath = QDir::tempPath() + "/EtykietyManager_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".json";
    QFile jsonFile(tempFilePath);
    if (!jsonFile.open(QIODevice::WriteOnly)) {
        m_lastError = "Nie można utworzyć tymczasowego pliku JSON: " + jsonFile.errorString();
        qDebug() << "[PythonPdfGenerator] Błąd:" << m_lastError;
        return false;
    }
    
    QJsonDocument doc(data);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    if (jsonFile.write(jsonData) != jsonData.size()) {
        m_lastError = "Błąd podczas zapisywania danych JSON: " + jsonFile.errorString();
        qDebug() << "[PythonPdfGenerator] Błąd:" << m_lastError;
        jsonFile.close();
        return false;
    }
    
    jsonFile.close();
    qDebug() << "[PythonPdfGenerator] Tymczasowy plik JSON:" << tempFilePath;
    qDebug() << "[PythonPdfGenerator] Dane JSON:" << QString::fromUtf8(jsonData);
    
    // Przygotuj argumenty dla skryptu Python
    QStringList arguments;
    arguments << scriptPath;
    arguments << tempFilePath;  // Plik z danymi JSON
    arguments << outputPath;    // Ścieżka wyjściowa PDF
    
    // Wyświetl dokładne polecenie do debugowania
    QString debugCommand = QString("python \"%1\" \"%2\" \"%3\"")
                            .arg(scriptPath)
                            .arg(tempFilePath)
                            .arg(outputPath);
    qDebug() << "[PythonPdfGenerator] Uruchamianie:" << debugCommand;
    
    // Uruchom skrypt Python - używamy pełnej ścieżki do interpretera Python
    QProcess pythonProcess;
    pythonProcess.setProcessChannelMode(QProcess::MergedChannels);  // Łączy stderr z stdout dla lepszego debugowania
    pythonProcess.setWorkingDirectory(QDir::currentPath());
    
    // Na Windows używamy "python", na innych systemach może być "python3"
#ifdef Q_OS_WIN
    pythonProcess.start("python", arguments);
#else
    pythonProcess.start("python3", arguments);
#endif
    
    if (!pythonProcess.waitForStarted(10000)) {
        m_lastError = QString("Nie można uruchomić skryptu Python: %1").arg(pythonProcess.errorString());
        qDebug() << "[PythonPdfGenerator] Błąd uruchamiania:" << m_lastError;
        return false;
    }
    
    if (!pythonProcess.waitForFinished(30000)) {
        m_lastError = "Timeout podczas wykonywania skryptu Python";
        pythonProcess.kill();
        qDebug() << "[PythonPdfGenerator] Błąd:" << m_lastError;
        return false;
    }
    
    int exitCode = pythonProcess.exitCode();
    QString stdoutText = QString::fromUtf8(pythonProcess.readAllStandardOutput());
    QString stderrText = QString::fromUtf8(pythonProcess.readAllStandardError());
    
    qDebug() << "[PythonPdfGenerator] Kod wyjścia:" << exitCode;
    qDebug() << "[PythonPdfGenerator] STDOUT:" << stdoutText;
    qDebug() << "[PythonPdfGenerator] STDERR:" << stderrText;
    
    if (exitCode != 0) {
        m_lastError = QString("Skrypt Python zakończył się błędem (kod %1): %2").arg(exitCode).arg(stderrText.trimmed());
        qDebug() << "[PythonPdfGenerator] Błąd wykonania:" << m_lastError;
        return false;
    }
    
    // Sprawdź czy plik PDF został utworzony
    if (!QDir().exists(outputPath)) {
        m_lastError = QString("Plik PDF nie został utworzony: %1").arg(outputPath);
        qDebug() << "[PythonPdfGenerator] Błąd:" << m_lastError;
        return false;
    }
    
    qDebug() << "[PythonPdfGenerator] PDF został wygenerowany pomyślnie:" << outputPath;
    return true;
}
