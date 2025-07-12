#pragma once

#include <QString>

class EmailConfig {
public:
    static const QString SMTP_SERVER;
    static const int SMTP_PORT;
    static const QString SMTP_USERNAME;
    static const QString SMTP_PASSWORD;
    
    // Metoda do sprawdzenia poprawności konfiguracji
    static bool isConfigValid();
};
