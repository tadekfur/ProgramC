#include "email_config.h"

// Konfiguracja SMTP
const QString EmailConfig::SMTP_SERVER = "termedialabels.home.pl";
const int EmailConfig::SMTP_PORT = 587; // SMTP z STARTTLS (lepiej niż port 25)
const QString EmailConfig::SMTP_USERNAME = "potwierdzenia@termedialabels.pl";
const QString EmailConfig::SMTP_PASSWORD = "Potwierdzenia_2025!";

bool EmailConfig::isConfigValid() {
    return !SMTP_SERVER.isEmpty() && 
           !SMTP_USERNAME.isEmpty() && 
           !SMTP_PASSWORD.isEmpty() && 
           SMTP_PORT > 0;
}
