#ifndef PYTHON_PDF_GENERATOR_H
#define PYTHON_PDF_GENERATOR_H

#include <QString>
#include <QMap>
#include <QVariant>
#include <QList>
#include <QProcess>
#include <QObject>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class PythonPdfGenerator : public QObject
{
    Q_OBJECT

public:
    explicit PythonPdfGenerator(QObject *parent = nullptr);
    
    // Generuje PDF potwierdzenia zamówienia używając skryptu Python
    bool generateOrderConfirmation(const QMap<QString, QVariant>& orderData,
                                 const QMap<QString, QVariant>& clientData,
                                 const QVector<QMap<QString, QVariant>>& orderItems,
                                 const QString& outputPath);
    
    // Generuje PDF zlecenia produkcyjnego używając skryptu Python
    bool generateProductionTicket(const QMap<QString, QVariant>& orderData,
                                const QMap<QString, QVariant>& clientData,
                                const QVector<QMap<QString, QVariant>>& orderItems,
                                const QString& outputPath);

    QString getLastError() const { return m_lastError; }

private:
    QString m_lastError;
    
    // Konwertuje dane Qt na JSON dla skryptu Python
    QJsonObject orderDataToJson(const QMap<QString, QVariant>& orderData);
    QJsonObject clientDataToJson(const QMap<QString, QVariant>& clientData);
    QJsonArray orderItemsToJson(const QVector<QMap<QString, QVariant>>& orderItems);
    
    // Uruchamia skrypt Python z argumentami JSON
    bool executePythonScript(const QString& scriptName, 
                           const QJsonObject& data, 
                           const QString& outputPath);
};

#endif // PYTHON_PDF_GENERATOR_H
