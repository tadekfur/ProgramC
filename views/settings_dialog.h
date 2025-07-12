#pragma once

#include <QDialog>
#include <QString>
#include <QListWidget>
#include "models/user.h"
#include <QVector>

class QComboBox;
class QPushButton;
class QLineEdit;
class QLabel;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    QString confirmationDir() const;
    QString productionDir() const;
    QString confirmationPrinter() const;
    QString productionPrinter() const;
    QString smtpServer() const;
    int smtpPort() const;
    QString smtpUser() const;
    QString smtpPassword() const;
    QString dbHost() const;
    QString dbName() const;
    QString dbUser() const;
    QString dbPassword() const;
    QString notificationServer() const;
    int notificationPort() const;
    int startOrderNumber() const; // Dodany getter dla początkowego numeru zamówień

private slots:
    void chooseConfirmationDir();
    void chooseProductionDir();
    void chooseConfirmationPrinter();
    void chooseProductionPrinter();
    void testDbConnection();
    void addUser();
    void editUser();
    void removeUser();
    void testNotificationServerConnection();
    void clearDatabase();  // Slot do obsługi czyszczenia bazy danych
    void checkOrdersCount();  // Slot do sprawdzenia liczby zamówień
    void testSmtpConnection(); // Slot do testowania połączenia SMTP
    void chooseMaterialsOrderPdfDir(); // Slot do wyboru katalogu PDF zamówień materiałów

private:
    QComboBox *styleCombo;
    QLineEdit *confirmationDirEdit;
    QLineEdit *productionDirEdit;
    QPushButton *chooseConfirmationDirBtn;
    QPushButton *chooseProductionDirBtn;
    QLineEdit *confirmationPrinterEdit;
    QPushButton *chooseConfirmationPrinterBtn;
    QLineEdit *productionPrinterEdit;
    QPushButton *chooseProductionPrinterBtn;
    QLineEdit *smtpServerEdit;
    QLineEdit *smtpPortEdit;
    QLineEdit *smtpUserEdit;
    QLineEdit *smtpPasswordEdit;
    QLineEdit *dbHostEdit;
    QLineEdit *dbNameEdit;
    QLineEdit *dbUserEdit;
    QLineEdit *dbPasswordEdit;
    QLineEdit *notificationServerEdit;
    QLineEdit *notificationPortEdit;
    QLineEdit *startOrderNumberEdit; // Dodane pole do obsługi początkowego numeru zamówień
    QLineEdit *materialsOrderPdfDirEdit;
    QPushButton *chooseMaterialsOrderPdfDirBtn;
    QPushButton *testDbBtn;
    QPushButton *okBtn;
    QPushButton *cancelBtn;
    QListWidget *userListWidget;
    QPushButton *addUserBtn;
    QPushButton *editUserBtn;
    QPushButton *removeUserBtn;
    QPushButton *saveBtn;
    QPushButton *clearDatabaseBtn;  // Przycisk do wyczyszczenia bazy danych
    QPushButton *checkOrdersCountBtn;  // Przycisk do sprawdzenia liczby zamówień

    void loadSettings();
    void saveSettings(bool closeDialog = true);
    void loadUserPrintSettings(const User& user);
    void saveUserPrintSettings(User& user) const;
    void loadUserMailSettings(const User& user);
    void saveUserMailSettings(User& user) const;
    void loadUserDbSettings(const User& user);
    void saveUserDbSettings(User& user) const;
};
