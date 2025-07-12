#include <QDialog>
#include "settings_dialog.h"
#include "user_dialog.h"
#include "models/user.h"
#include "utils/secure_user_manager.h"
#include "db/dbmanager.h"
#include <QVector>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>
#include <QHostInfo>
#include <QSqlQuery>
#include "utils/email_config.h"

#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QSettings>
#include <QPrinterInfo>
#include <QPrintDialog>
#include <QPrinter>
#include <QMessageBox>
#include <QApplication>
#include <QToolButton>
#include <QIcon>
#include <QFrame>
#include <QInputDialog>
#include <QTabWidget>
#include <QSqlDatabase>
#include <QSqlError>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    qDebug() << "[DEBUG] SettingsDialog konstruktor START";
    setWindowTitle("Ustawienia aplikacji");
    setMinimumWidth(700);
    setMinimumHeight(500);
    setStyleSheet("QDialog { background: #f6f8fa; border-radius: 12px; } ");

    QTabWidget *tabs = new QTabWidget(this);
    // --- Baza danych ---
    QWidget *dbTab = new QWidget;
    QVBoxLayout *dbLayout = new QVBoxLayout(dbTab);
    dbLayout->addWidget(new QLabel("[Baza danych]"));
    dbHostEdit = new QLineEdit(dbTab);
    dbNameEdit = new QLineEdit(dbTab);
    dbUserEdit = new QLineEdit(dbTab);
    dbPasswordEdit = new QLineEdit(dbTab);
    dbPasswordEdit->setEchoMode(QLineEdit::Password);
    QToolButton *showDbPassBtn = new QToolButton(dbTab);
    showDbPassBtn->setIcon(QIcon(":/icons/eye.png"));
    showDbPassBtn->setCheckable(true);
    connect(showDbPassBtn, &QToolButton::toggled, this, [this, showDbPassBtn](bool checked){
        dbPasswordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
        showDbPassBtn->setIcon(QIcon(":/icons/eye.png"));
    });
    notificationServerEdit = new QLineEdit(dbTab);
    notificationPortEdit = new QLineEdit(dbTab);
    testDbBtn = new QPushButton("Testuj połączenie", dbTab);
    QPushButton *testNotifyBtn = new QPushButton("Testuj serwer powiadomień", dbTab);
    clearDatabaseBtn = new QPushButton("Wyczyść wszystkie zamówienia", dbTab);
    clearDatabaseBtn->setStyleSheet("QPushButton { background-color: #dc3545; color: white; font-weight: bold; }");
    clearDatabaseBtn->setToolTip("UWAGA: Nieodwracalnie usuwa wszystkie zamówienia z bazy danych!");
    dbLayout->addWidget(new QLabel("Host bazy danych:")); dbLayout->addWidget(dbHostEdit);
    dbLayout->addWidget(new QLabel("Nazwa bazy:")); dbLayout->addWidget(dbNameEdit);
    dbLayout->addWidget(new QLabel("Użytkownik bazy:")); dbLayout->addWidget(dbUserEdit);
    dbLayout->addWidget(new QLabel("Hasło bazy:"));
    QHBoxLayout *dbPassLayout = new QHBoxLayout;
    dbPassLayout->addWidget(dbPasswordEdit);
    dbPassLayout->addWidget(showDbPassBtn);
    dbLayout->addLayout(dbPassLayout);
    dbLayout->addWidget(new QLabel("Serwer powiadomień:")); dbLayout->addWidget(notificationServerEdit);
    dbLayout->addWidget(new QLabel("Port powiadomień:")); dbLayout->addWidget(notificationPortEdit);
    dbLayout->addWidget(testDbBtn);
    dbLayout->addWidget(testNotifyBtn);
    dbLayout->addWidget(new QLabel("Początkowy numer zamówienia:"));
    startOrderNumberEdit = new QLineEdit(dbTab);
    startOrderNumberEdit->setMaximumWidth(120);
    dbLayout->addWidget(startOrderNumberEdit);

    // Sekcja niebezpiecznych operacji
    dbLayout->addWidget(new QLabel(""));  // Odstęp
    QLabel *dangerLabel = new QLabel("NIEBEZPIECZNE OPERACJE:");
    dangerLabel->setStyleSheet("color: #dc3545; font-weight: bold; font-size: 12px;");
    dbLayout->addWidget(dangerLabel);
    dbLayout->addWidget(clearDatabaseBtn);
    
    dbLayout->addStretch();
    tabs->addTab(dbTab, "Baza danych");

    // --- E-mail ---
    QWidget *mailTab = new QWidget;
    QVBoxLayout *mailLayout = new QVBoxLayout(mailTab);
    mailLayout->addWidget(new QLabel("[E-mail]"));
    smtpServerEdit = new QLineEdit(mailTab);
    smtpPortEdit = new QLineEdit(mailTab);
    smtpUserEdit = new QLineEdit(mailTab);
    smtpPasswordEdit = new QLineEdit(mailTab);
    smtpPasswordEdit->setEchoMode(QLineEdit::Password);
    QPushButton *testSmtpBtn = new QPushButton("Testuj połączenie e-mail", mailTab); // NOWY PRZYCISK
    mailLayout->addWidget(new QLabel("Serwer e-mail:")); mailLayout->addWidget(smtpServerEdit);
    mailLayout->addWidget(new QLabel("Port e-mail:")); mailLayout->addWidget(smtpPortEdit);
    mailLayout->addWidget(new QLabel("Użytkownik e-mail:")); mailLayout->addWidget(smtpUserEdit);
    mailLayout->addWidget(new QLabel("Hasło e-mail:")); mailLayout->addWidget(smtpPasswordEdit);
    mailLayout->addWidget(testSmtpBtn); // DODAJ PRZYCISK DO LAYOUTU
    mailLayout->addStretch();
    tabs->addTab(mailTab, "E-mail");
    connect(testSmtpBtn, &QPushButton::clicked, this, &SettingsDialog::testSmtpConnection);

    // Ustaw focus na polu hasła po przełączeniu na zakładkę E-mail
    connect(tabs, &QTabWidget::currentChanged, this, [this, tabs, mailTab](int idx) {
        if (tabs->widget(idx) == mailTab) {
            smtpPasswordEdit->setFocus();
        }
    });

    // --- Drukarki i katalogi ---
    QWidget *printTab = new QWidget;
    QVBoxLayout *printLayout = new QVBoxLayout(printTab);
    printLayout->addWidget(new QLabel("[Drukarki i katalogi]"));
    confirmationPrinterEdit = new QLineEdit(printTab);
    confirmationPrinterEdit->setReadOnly(true);
    chooseConfirmationPrinterBtn = new QPushButton("Wybierz drukarkę...", printTab);
    productionPrinterEdit = new QLineEdit(printTab);
    productionPrinterEdit->setReadOnly(true);
    chooseProductionPrinterBtn = new QPushButton("Wybierz drukarkę...", printTab);
    confirmationDirEdit = new QLineEdit(printTab);
    chooseConfirmationDirBtn = new QPushButton("Wybierz...", printTab);
    productionDirEdit = new QLineEdit(printTab);
    chooseProductionDirBtn = new QPushButton("Wybierz...", printTab);
    materialsOrderPdfDirEdit = new QLineEdit(printTab);
    chooseMaterialsOrderPdfDirBtn = new QPushButton("Wybierz...", printTab);
    printLayout->addWidget(new QLabel("Drukarka potwierdzenia:"));
    QHBoxLayout *row1 = new QHBoxLayout; row1->addWidget(confirmationPrinterEdit); row1->addWidget(chooseConfirmationPrinterBtn); printLayout->addLayout(row1);
    printLayout->addWidget(new QLabel("Drukarka produkcyjna:"));
    QHBoxLayout *row2 = new QHBoxLayout; row2->addWidget(productionPrinterEdit); row2->addWidget(chooseProductionPrinterBtn); printLayout->addLayout(row2);
    printLayout->addWidget(new QLabel("Katalog potwierdzeń PDF:"));
    QHBoxLayout *row3 = new QHBoxLayout; row3->addWidget(confirmationDirEdit); row3->addWidget(chooseConfirmationDirBtn); printLayout->addLayout(row3);
    printLayout->addWidget(new QLabel("Katalog zleceń produkcyjnych PDF:"));
    QHBoxLayout *row4 = new QHBoxLayout; row4->addWidget(productionDirEdit); row4->addWidget(chooseProductionDirBtn); printLayout->addLayout(row4);
    printLayout->addWidget(new QLabel("Katalog zamówień materiałów PDF:"));
    QHBoxLayout *row5 = new QHBoxLayout; row5->addWidget(materialsOrderPdfDirEdit); row5->addWidget(chooseMaterialsOrderPdfDirBtn); printLayout->addLayout(row5);
    printLayout->addStretch();
    tabs->addTab(printTab, "Drukarki i katalogi");

    // --- Użytkownicy ---
    QWidget *usersTab = new QWidget;
    QVBoxLayout *usersLayout = new QVBoxLayout(usersTab);
    usersLayout->addWidget(new QLabel("[Użytkownicy]"));
    userListWidget = new QListWidget(usersTab);
    addUserBtn = new QPushButton("Dodaj użytkownika", usersTab);
    editUserBtn = new QPushButton("Edytuj", usersTab);
    removeUserBtn = new QPushButton("Usuń", usersTab);
    QHBoxLayout *userBtnLayout = new QHBoxLayout;
    userBtnLayout->addWidget(addUserBtn);
    userBtnLayout->addWidget(editUserBtn);
    userBtnLayout->addWidget(removeUserBtn);
    usersLayout->addWidget(new QLabel("Użytkownicy:"));
    usersLayout->addWidget(userListWidget);
    usersLayout->addLayout(userBtnLayout);
    usersLayout->addStretch();
    tabs->addTab(usersTab, "Użytkownicy");

    // --- Wygląd (opcjonalnie) ---
    // Możesz dodać osobną zakładkę jeśli chcesz więcej opcji wyglądu

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabs);
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    okBtn = new QPushButton("OK", this);
    saveBtn = new QPushButton("Zapisz", this);
    cancelBtn = new QPushButton("Anuluj", this);
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);
    setLayout(mainLayout);
    loadSettings();

    connect(addUserBtn, &QPushButton::clicked, this, &SettingsDialog::addUser);
    connect(editUserBtn, &QPushButton::clicked, this, &SettingsDialog::editUser);
    connect(removeUserBtn, &QPushButton::clicked, this, &SettingsDialog::removeUser);
    // Dodaj walidację i obsługę błędów przy edycji/dodawaniu użytkownika
    connect(chooseConfirmationDirBtn, &QPushButton::clicked, this, &SettingsDialog::chooseConfirmationDir);
    connect(chooseProductionDirBtn, &QPushButton::clicked, this, &SettingsDialog::chooseProductionDir);
    connect(chooseConfirmationPrinterBtn, &QPushButton::clicked, this, &SettingsDialog::chooseConfirmationPrinter);
    connect(chooseProductionPrinterBtn, &QPushButton::clicked, this, &SettingsDialog::chooseProductionPrinter);
    connect(testDbBtn, &QPushButton::clicked, this, &SettingsDialog::testDbConnection);
    connect(testNotifyBtn, &QPushButton::clicked, this, &SettingsDialog::testNotificationServerConnection);
    connect(clearDatabaseBtn, &QPushButton::clicked, this, &SettingsDialog::clearDatabase);
    connect(saveBtn, &QPushButton::clicked, this, [this](){ saveSettings(false); });
    connect(okBtn, &QPushButton::clicked, this, [this](){ saveSettings(true); });
    connect(cancelBtn, &QPushButton::clicked, this, &SettingsDialog::reject);

    userListWidget->clear();
    for (const User &user : SecureUserManager::instance().users()) {
        userListWidget->addItem(user.firstName + " " + user.lastName + " (" + user.login + ", " +
            (user.role == User::Role::Admin ? "Admin" : user.role == User::Role::Operator ? "Operator" : "Gość") + ")");
    }

    // Synchronizacja pól przy zmianie użytkownika na liście
    connect(userListWidget, &QListWidget::currentRowChanged, this, [this](int row){
        if (row < 0 || row >= SecureUserManager::instance().users().size()) return;
        const User& user = SecureUserManager::instance().users()[row];
        loadUserPrintSettings(user);
        loadUserMailSettings(user);
        loadUserDbSettings(user);
    });

    // Wstępne ustawienia serwera powiadomień
    QString defaultHost = QHostInfo::localHostName();
    notificationServerEdit->setText(defaultHost);
    notificationPortEdit->setText("9000"); // domyślny port

    // --- WALIDACJA POCZĄTKOWEGO NUMERU ZAMÓWIENIA ---
    connect(startOrderNumberEdit, &QLineEdit::editingFinished, this, [this]() {
        auto& dbm = DbManager::instance();
        QSqlDatabase db = dbm.database();
        if (!db.isOpen()) {
            QMessageBox::critical(this, "Błąd bazy", "Brak połączenia z bazą danych. Nie można zweryfikować numeru zamówienia.");
            return;
        }
        QSqlQuery q(db);
        int lastNumber = 0;
        if (q.exec("SELECT last_number FROM order_sequence WHERE id=1")) {
            if (q.next()) lastNumber = q.value(0).toInt();
        }
        int val = startOrderNumberEdit->text().toInt();
        if (val <= lastNumber) {
            QMessageBox::warning(this, "Nieprawidłowy numer", QString("Początkowy numer zamówienia nie może być mniejszy lub równy ostatniemu użytemu (%1).\nWprowadź większą wartość.").arg(lastNumber));
            startOrderNumberEdit->setText(QString::number(lastNumber + 1));
        }
    });
    qDebug() << "[DEBUG] Podpinam sygnał chooseMaterialsOrderPdfDirBtn->clicked";
    connect(chooseMaterialsOrderPdfDirBtn, &QPushButton::clicked, this, &SettingsDialog::chooseMaterialsOrderPdfDir);
    qDebug() << "[DEBUG] SettingsDialog konstruktor END";
}

void SettingsDialog::chooseConfirmationDir() {
    QString dir = QFileDialog::getExistingDirectory(this, "Wybierz katalog potwierdzeń PDF");
    if (!dir.isEmpty()) confirmationDirEdit->setText(dir);
}

void SettingsDialog::chooseProductionDir() {
    QString dir = QFileDialog::getExistingDirectory(this, "Wybierz katalog zleceń produkcyjnych PDF");
    if (!dir.isEmpty()) productionDirEdit->setText(dir);
}

void SettingsDialog::chooseMaterialsOrderPdfDir() {
    qDebug() << "[DEBUG] SLOT chooseMaterialsOrderPdfDir() wywołany";
    QString dir = QFileDialog::getExistingDirectory(this, "Wybierz katalog zamówień materiałów PDF");
    qDebug() << "[DEBUG] QFileDialog::getExistingDirectory zwrócił:" << dir;
    if (!dir.isEmpty()) materialsOrderPdfDirEdit->setText(dir);
}

void SettingsDialog::chooseConfirmationPrinter() {
    QPrinter printer;
    QPrintDialog dlg(&printer, this);
    dlg.setWindowTitle("Wybierz drukarkę do potwierdzeń");
    if (dlg.exec() == QDialog::Accepted) {
        confirmationPrinterEdit->setText(printer.printerName());
    }
}

void SettingsDialog::chooseProductionPrinter() {
    QPrinter printer;
    QPrintDialog dlg(&printer, this);
    dlg.setWindowTitle("Wybierz drukarkę produkcyjną");
    if (dlg.exec() == QDialog::Accepted) {
        productionPrinterEdit->setText(printer.printerName());
    }
}

void SettingsDialog::testDbConnection() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", "test_connection");
    db.setHostName(dbHostEdit->text());
    db.setDatabaseName(dbNameEdit->text());
    db.setUserName(dbUserEdit->text());
    db.setPassword(dbPasswordEdit->text());
    bool ok = db.open();
    if (ok) {
        QMessageBox::information(this, "Test połączenia", "Połączenie z bazą danych powiodło się.");
        db.close();
    } else {
        QString details =
            "Błąd: " + db.lastError().text() +
            "\nHost: " + dbHostEdit->text() +
            "\nBaza: " + dbNameEdit->text() +
            "\nUżytkownik: " + dbUserEdit->text();
        QMessageBox::critical(this, "Błąd połączenia", "Nie udało się połączyć z bazą danych:\n" + details);
    }
    QSqlDatabase::removeDatabase("test_connection");
}

void SettingsDialog::testNotificationServerConnection() {
    int port = notificationPortEdit->text().toInt();
    QUdpSocket udp;
    QByteArray testMsg = "TEST_NOTIFICATION_BROADCAST";
    // Wymuś broadcast na 255.255.255.255
    QHostAddress broadcastAddr = QHostAddress::Broadcast;
    udp.setSocketOption(QAbstractSocket::MulticastTtlOption, 1);
    bool ok = udp.writeDatagram(testMsg, broadcastAddr, port) != -1;
    if (!ok) {
        QMessageBox::critical(this, "Błąd UDP", QString("Nie udało się wysłać broadcastu UDP na 255.255.255.255:%1\n%2")
            .arg(port).arg(udp.errorString()));
        return;
    }
    QMessageBox::information(this, "Test powiadomień UDP", QString("Wysłano broadcast UDP na 255.255.255.255:%1.\nUwaga: brak odpowiedzi nie oznacza błędu, jeśli serwer nie odsyła odpowiedzi.")
        .arg(port));
}

QString SettingsDialog::confirmationDir() const { return confirmationDirEdit->text(); }
QString SettingsDialog::productionDir() const { return productionDirEdit->text(); }
QString SettingsDialog::confirmationPrinter() const { return confirmationPrinterEdit->text(); }
QString SettingsDialog::productionPrinter() const { return productionPrinterEdit->text(); }
QString SettingsDialog::smtpServer() const { return smtpServerEdit->text(); }
int SettingsDialog::smtpPort() const { return smtpPortEdit->text().toInt(); }
QString SettingsDialog::smtpUser() const { return smtpUserEdit->text(); }
QString SettingsDialog::smtpPassword() const { return smtpPasswordEdit->text(); }
QString SettingsDialog::dbHost() const { return dbHostEdit->text(); }
QString SettingsDialog::dbName() const { return dbNameEdit->text(); }
QString SettingsDialog::dbUser() const { return dbUserEdit->text(); }
QString SettingsDialog::dbPassword() const { return dbPasswordEdit->text(); }
QString SettingsDialog::notificationServer() const { return notificationServerEdit->text(); }
int SettingsDialog::notificationPort() const { return notificationPortEdit->text().toInt(); }
int SettingsDialog::startOrderNumber() const {
    return startOrderNumberEdit->text().toInt();
}

void SettingsDialog::loadSettings() {
    QSettings s;
    qDebug() << "QSettings file:" << s.fileName();
    qDebug() << "[DEBUG] loadSettings() START";
    qDebug() << "[DEBUG] Przed odczytem: confirmationDirEdit=" << confirmationDirEdit->text()
             << ", productionDirEdit=" << productionDirEdit->text()
             << ", smtpServerEdit=" << smtpServerEdit->text()
             << ", smtpPortEdit=" << smtpPortEdit->text();
    confirmationDirEdit->setText(s.value("pdf/confirmationDir", "").toString());
    productionDirEdit->setText(s.value("pdf/productionDir", "").toString());
    confirmationPrinterEdit->setText(s.value("print/confirmationPrinter", "").toString());
    productionPrinterEdit->setText(s.value("print/productionPrinter", "").toString());
    // --- SMTP: domyślne wartości z EmailConfig tylko jeśli brak w QSettings ---
    QString smtpServer = s.value("email/server", QString()).toString();
    if (smtpServer.isEmpty()) smtpServer = EmailConfig::SMTP_SERVER;
    smtpServerEdit->setText(smtpServer);
    QString smtpPort = s.value("email/port", QString()).toString();
    if (smtpPort.isEmpty() || smtpPort == "0") smtpPort = QString::number(EmailConfig::SMTP_PORT);
    smtpPortEdit->setText(smtpPort);
    QString smtpUser = s.value("email/user", QString()).toString();
    if (smtpUser.isEmpty()) smtpUser = EmailConfig::SMTP_USERNAME;
    smtpUserEdit->setText(smtpUser);
    QString smtpPass = s.value("email/password", QString()).toString();
    if (smtpPass.isEmpty()) smtpPass = EmailConfig::SMTP_PASSWORD;
    smtpPasswordEdit->setText(smtpPass);
    dbHostEdit->setText(s.value("db/host", "localhost").toString());
    dbNameEdit->setText(s.value("db/name", "etykiety_db").toString());
    dbUserEdit->setText(s.value("db/user", "postgres").toString());
    QString dbPass = s.value("db/password", "").toString();
    dbPasswordEdit->setText(dbPass);
    qDebug() << "Odczytane hasło bazy:" << dbPass;
    notificationServerEdit->setText(s.value("notify/server", QHostInfo::localHostName()).toString());
    notificationPortEdit->setText(s.value("notify/port", "9000").toString());
    startOrderNumberEdit->setText(s.value("orders/startNumber", "1").toString());
    materialsOrderPdfDirEdit->setText(s.value("pdf/materialsOrderDir", "").toString());
    qDebug() << "[DEBUG] Po odczycie: confirmationDirEdit=" << confirmationDirEdit->text()
             << ", productionDirEdit=" << productionDirEdit->text()
             << ", smtpServerEdit=" << smtpServerEdit->text()
             << ", smtpPortEdit=" << smtpPortEdit->text();
    qDebug() << "[DEBUG] loadSettings() END";
}

void SettingsDialog::saveSettings(bool closeDialog) {
    QSettings s;
    qDebug() << "QSettings file:" << s.fileName();
    qDebug() << "[DEBUG] saveSettings() START";
    qDebug() << "[DEBUG] Przed zapisem: confirmationDirEdit=" << confirmationDir()
             << ", productionDirEdit=" << productionDir()
             << ", smtpServerEdit=" << smtpServer()
             << ", smtpPortEdit=" << smtpPort();
    s.setValue("pdf/confirmationDir", confirmationDir());
    s.setValue("pdf/productionDir", productionDir());
    s.setValue("print/confirmationPrinter", confirmationPrinter());
    s.setValue("print/productionPrinter", productionPrinter());
    s.setValue("email/server", smtpServer());
    s.setValue("email/port", smtpPort());
    s.setValue("email/user", smtpUser());
    s.setValue("email/password", smtpPassword());
    s.setValue("db/host", dbHost());
    s.setValue("db/name", dbName());
    s.setValue("db/user", dbUser());
    if (!dbPassword().isEmpty()) {
        s.setValue("db/password", dbPassword());
        qDebug() << "Zapisuję hasło bazy:" << dbPassword();
    } else {
        qDebug() << "Nie nadpisuję hasła bazy (pole puste)";
    }
    s.setValue("notify/server", notificationServer());
    s.setValue("notify/port", notificationPort());
    s.setValue("orders/startNumber", startOrderNumberEdit->text());
    s.setValue("pdf/materialsOrderDir", materialsOrderPdfDirEdit->text());
    qDebug() << "[DEBUG] Po zapisie: confirmationDirEdit=" << confirmationDir()
             << ", productionDirEdit=" << productionDir()
             << ", smtpServerEdit=" << smtpServer()
             << ", smtpPortEdit=" << smtpPort();
    qDebug() << "[DEBUG] saveSettings() END";
    if (closeDialog) accept();
}

void SettingsDialog::loadUserPrintSettings(const User& user) {
    confirmationPrinterEdit->setText(user.confirmationPrinter);
    productionPrinterEdit->setText(user.productionPrinter);
    confirmationDirEdit->setText(user.confirmationDir);
    productionDirEdit->setText(user.productionDir);
}

void SettingsDialog::saveUserPrintSettings(User& user) const {
    user.confirmationPrinter = confirmationPrinterEdit->text();
    user.productionPrinter = productionPrinterEdit->text();
    user.confirmationDir = confirmationDirEdit->text();
    user.productionDir = productionDirEdit->text();
}

void SettingsDialog::loadUserMailSettings(const User& user) {
    smtpServerEdit->setText(user.smtpServer);
    smtpPortEdit->setText(user.smtpPort);
    smtpUserEdit->setText(user.smtpUser);
    smtpPasswordEdit->setText(user.smtpPassword);
}
void SettingsDialog::saveUserMailSettings(User& user) const {
    user.smtpServer = smtpServerEdit->text();
    user.smtpPort = smtpPortEdit->text();
    user.smtpUser = smtpUserEdit->text();
    user.smtpPassword = smtpPasswordEdit->text();
}
void SettingsDialog::loadUserDbSettings(const User& user) {
    dbHostEdit->setText(user.dbHost);
    dbNameEdit->setText(user.dbName);
    dbUserEdit->setText(user.dbUser);
    // Ustaw hasło tylko jeśli user.dbPassword nie jest puste
    if (!user.dbPassword.isEmpty()) {
        dbPasswordEdit->setText(user.dbPassword);
    }
}
void SettingsDialog::saveUserDbSettings(User& user) const {
    user.dbHost = dbHostEdit->text();
    user.dbName = dbNameEdit->text();
    user.dbUser = dbUserEdit->text();
    user.dbPassword = dbPasswordEdit->text();
}

void SettingsDialog::addUser() {
    UserDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        User user = dlg.getUser();
        // Walidacja unikalności loginu
        for (const User &u : SecureUserManager::instance().users()) {
            if (u.login.compare(user.login, Qt::CaseInsensitive) == 0) {
                QMessageBox::warning(this, "Błąd", "Użytkownik o podanym loginie już istnieje.");
                return;
            }
        }
        saveUserPrintSettings(user);
        saveUserMailSettings(user);
        saveUserDbSettings(user);
        SecureUserManager::instance().addUser(user);
        SecureUserManager::instance().saveUsersToFile(); // <-- DODANO ZAPIS
        userListWidget->addItem(user.firstName + " " + user.lastName + " (" + user.login + ", " +
            (user.role == User::Role::Admin ? "Admin" : user.role == User::Role::Operator ? "Operator" : "Gość") + ")");
        userListWidget->setCurrentRow(userListWidget->count() - 1);
    }
}

void SettingsDialog::editUser() {
    int row = userListWidget->currentRow();
    if (row < 0 || row >= SecureUserManager::instance().users().size()) return;
    User user = SecureUserManager::instance().users()[row];
    loadUserPrintSettings(user);
    loadUserMailSettings(user);
    loadUserDbSettings(user);
    UserDialog dlg(this);
    dlg.setUser(user);
    if (dlg.exec() == QDialog::Accepted) {
        user = dlg.getUser();
        for (int i = 0; i < SecureUserManager::instance().users().size(); ++i) {
            if (i != row && SecureUserManager::instance().users()[i].login.compare(user.login, Qt::CaseInsensitive) == 0) {
                QMessageBox::warning(this, "Błąd", "Użytkownik o podanym loginie już istnieje.");
                return;
            }
        }
        saveUserPrintSettings(user);
        saveUserMailSettings(user);
        saveUserDbSettings(user);
        SecureUserManager::instance().updateUser(row, user);
        SecureUserManager::instance().saveUsersToFile(); // <-- DODANO ZAPIS
        userListWidget->item(row)->setText(
            user.firstName + " " + user.lastName + " (" + user.login + ", " +
            (user.role == User::Role::Admin ? "Admin" : user.role == User::Role::Operator ? "Operator" : "Gość") + ")");
        userListWidget->setCurrentRow(row);
    }
}

void SettingsDialog::removeUser() {
    int row = userListWidget->currentRow();
    if (row < 0 || row >= SecureUserManager::instance().users().size()) return;
    const QVector<User> &users = SecureUserManager::instance().users();
    // Blokada usunięcia ostatniego administratora
    int adminCount = 0;
    for (const User &u : users) if (u.role == User::Role::Admin) ++adminCount;
    if (users[row].role == User::Role::Admin && adminCount <= 1) {
        QMessageBox::warning(this, "Błąd", "Nie można usunąć ostatniego administratora.");
        return;
    }
    // Potwierdzenie usunięcia
    if (QMessageBox::question(this, "Usuń użytkownika", "Czy na pewno chcesz usunąć wybranego użytkownika?") != QMessageBox::Yes)
        return;
    SecureUserManager::instance().removeUser(row);
    SecureUserManager::instance().saveUsersToFile(); // <-- DODANO ZAPIS
    delete userListWidget->takeItem(row);
}

void SettingsDialog::clearDatabase() {
    // Potwierdzenie z ostrzeżeniem
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "OSTRZEŻENIE - Wyczyszczenie bazy danych",
        "Ta operacja jest NIEODWRACALNA!\n\n"
        "Zostaną usunięte wszystkie zamówienia i pozycje zamówień z bazy danych.\n"
        "Klienci i ich adresy dostawy pozostaną bez zmian.\n\n"
        "Czy na pewno chcesz kontynuować?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Drugie potwierdzenie dla pewności
    reply = QMessageBox::question(
        this,
        "OSTATNIE OSTRZEŻENIE",
        "To jest ostatnia szansa na anulowanie!\n\n"
        "Czy NAPEWNO chcesz usunąć wszystkie zamówienia?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Wykonaj czyszczenie bazy danych
    DbManager& dbManager = DbManager::instance();
    if (dbManager.clearAllOrders()) {
        QMessageBox::information(
            this,
            "Sukces",
            "Wszystkie zamówienia zostały pomyślnie usunięte z bazy danych.\n\n"
            "Aplikacja zostanie teraz odświeżona."
        );
        
        // Zamknij okno ustawień
        accept();
        
        // Można tutaj dodać sygnał do odświeżenia widoków w głównym oknie
        // lub po prostu poinformować użytkownika o konieczności restartu
    } else {
        QMessageBox::critical(
            this,
            "Błąd",
            "Wystąpił błąd podczas czyszczenia bazy danych.\n"
            "Sprawdź logi aplikacji lub skontaktuj się z administratorem."
        );
    }
}

void SettingsDialog::checkOrdersCount() {
    DbManager& dbManager = DbManager::instance();
    int ordersCount = dbManager.getOrdersCount();
    
    if (ordersCount >= 0) {
        QMessageBox::information(
            this,
            "Liczba zamówień",
            QString("Aktualna liczba zamówień w bazie danych: %1").arg(ordersCount)
        );
    } else {
        QMessageBox::critical(
            this,
            "Błąd",
            "Wystąpił błąd podczas sprawdzania liczby zamówień.\n"
            "Sprawdź logi aplikacji lub skontaktuj się z administratorem."
        );
    }
}

#include <QTcpSocket>

void SettingsDialog::testSmtpConnection() {
    QString server = smtpServerEdit->text();
    int port = smtpPortEdit->text().toInt();
    QTcpSocket socket;
    socket.connectToHost(server, port);
    if (!socket.waitForConnected(3000)) {
        QMessageBox::critical(this, "Test SMTP", "Błąd połączenia z serwerem e-mail: " + socket.errorString());
        return;
    }
    QMessageBox::information(this, "Test SMTP", "Połączenie z serwerem e-mail powiodło się!");
    socket.disconnectFromHost();
}
