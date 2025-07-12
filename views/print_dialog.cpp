#include "print_dialog.h"
#include "db/dbmanager.h"
#include "utils/python_pdf_generator.h"
#include "utils/email_sender.h"
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>

PrintDialog::PrintDialog(int orderId, QWidget *parent)
    : QDialog(parent), m_orderId(orderId), m_emailSender(nullptr)
{
    setWindowTitle("Opcje drukowania");
    setModal(true);
    setupUI();
}

void PrintDialog::setupUI() {
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(25, 25, 25, 25);
    mainLayout->setSpacing(20);
    
    // Nagłówek
    QLabel *headerLabel = new QLabel("Wybierz opcję drukowania:");
    QFont headerFont = headerLabel->font();
    headerFont.setPointSize(16);
    headerFont.setBold(true);
    headerLabel->setFont(headerFont);
    headerLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(headerLabel);
    
    // Grupa: Potwierdzenie dla klienta
    clientConfirmationGroup = new QGroupBox("Potwierdzenie dla klienta");
    QVBoxLayout *clientLayout = new QVBoxLayout(clientConfirmationGroup);
    clientLayout->setSpacing(15);
    clientLayout->setContentsMargins(20, 25, 20, 20);
    
    btnPrintConfirmation = new QPushButton("Drukuj");
    btnPrintConfirmation->setMinimumHeight(45);
    btnPrintConfirmation->setMaximumWidth(380);
    btnPrintConfirmation->setToolTip("Generuje PDF i wyświetla podgląd");
    QFont buttonFont = btnPrintConfirmation->font();
    buttonFont.setPointSize(11); // Większa czcionka
    buttonFont.setBold(true);
    btnPrintConfirmation->setFont(buttonFont);
    
    btnPrintAndEmail = new QPushButton("Drukuj i wyślij");
    btnPrintAndEmail->setMinimumHeight(45);
    btnPrintAndEmail->setMaximumWidth(380);
    btnPrintAndEmail->setToolTip("Generuje PDF, wyświetla podgląd i wysyła automatycznie e-mailem");
    btnPrintAndEmail->setFont(buttonFont);
    
    clientLayout->addWidget(btnPrintConfirmation);
    clientLayout->addWidget(btnPrintAndEmail);
    
    // Grupa: Produkcja
    productionGroup = new QGroupBox("Produkcja");
    QVBoxLayout *productionLayout = new QVBoxLayout(productionGroup);
    productionLayout->setSpacing(15);
    productionLayout->setContentsMargins(20, 25, 20, 20);
    
    btnPrintProduction = new QPushButton("Drukuj");
    btnPrintProduction->setMinimumHeight(45);
    btnPrintProduction->setMaximumWidth(380);
    btnPrintProduction->setToolTip("Generuje PDF do druku dla produkcji");
    btnPrintProduction->setFont(buttonFont);
    
    productionLayout->addWidget(btnPrintProduction);
    
    // Dodaj grupy do głównego layoutu
    mainLayout->addWidget(clientConfirmationGroup);
    mainLayout->addWidget(productionGroup);
    
    // Przycisk zamknij
    QPushButton *btnClose = new QPushButton("Zamknij");
    btnClose->setMinimumHeight(35);
    
    QHBoxLayout *closeLayout = new QHBoxLayout;
    closeLayout->setContentsMargins(0, 10, 0, 0);
    closeLayout->addStretch();
    closeLayout->addWidget(btnClose);
    closeLayout->addStretch();
    mainLayout->addLayout(closeLayout);
    
    // Połącz sygnały
    connect(btnPrintConfirmation, &QPushButton::clicked, this, &PrintDialog::printConfirmation);
    connect(btnPrintAndEmail, &QPushButton::clicked, this, &PrintDialog::printAndEmailConfirmation);
    connect(btnPrintProduction, &QPushButton::clicked, this, &PrintDialog::printProduction);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    
    // Ustaw rozmiar okna - zwiększona wysokość dla lepszego układu
    setFixedSize(450, 480);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

void PrintDialog::printConfirmation() {
    qDebug() << "[PrintDialog] Drukowanie potwierdzenia dla zamówienia:" << m_orderId;
    
    try {
        // Pobierz dane zamówienia z bazy
        auto& dbm = DbManager::instance();
        auto orderData = dbm.getOrderById(m_orderId);
        
        if (orderData.isEmpty()) {
            QMessageBox::warning(this, "Błąd", "Nie można pobrać danych zamówienia.");
            return;
        }
        
        // Pobierz dane klienta
        int clientId = orderData["client_id"].toInt();
        QMap<QString, QVariant> clientData;
        auto clients = dbm.getClients();
        for (const auto& c : clients) {
            if (c["id"].toInt() == clientId) {
                clientData = c;
                break;
            }
        }
        
        // DEBUG: Sprawdź dostępne pola klienta
        qDebug() << "[PrintDialog] Dostępne pola klienta:";
        for (auto it = clientData.begin(); it != clientData.end(); ++it) {
            qDebug() << "  " << it.key() << ":" << it.value();
        }
        
        // Pobierz pozycje zamówienia
        auto orderItems = dbm.getOrderItems(m_orderId);
        
        // Pobierz ścieżkę z ustawień
        QSettings settings;
        QString dir = settings.value("pdf/confirmationDir", "").toString();
        if (dir.isEmpty()) {
            // Użyj domyślnego katalogu z dokumentów użytkownika  
            dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/EtykietyManager/Potwierdzenia";
            settings.setValue("pdf/confirmationDir", dir);
            qDebug() << "[PrintDialog] Ustawiono domyślny katalog potwierdzeń dla printConfirmation:" << dir;
        }
        
        // Utwórz katalog jeśli nie istnieje
        QDir().mkpath(dir);
        
        // Generuj PDF potwierdzenia
        QString fileName = QString("%1_%2_POTWIERDZENIE.pdf")
            .arg(orderData["order_number"].toString())
            .arg(clientData["name"].toString().replace(" ", "_"));
        QString outputPath = dir + "/" + fileName;
        
        // Użyj nowego generatora Python
        PythonPdfGenerator pdfGenerator;
        if (!pdfGenerator.generateOrderConfirmation(orderData, clientData, orderItems, outputPath)) {
            QMessageBox::warning(this, "Błąd", 
                QString("Nie udało się wygenerować pliku PDF:\n%1").arg(pdfGenerator.getLastError()));
            return;
        }
        
        // Otwórz podgląd PDF
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(outputPath))) {
            QMessageBox::warning(this, "Błąd", "Nie można otworzyć pliku PDF w podglądzie.");
        } else {
            QMessageBox::information(this, "Sukces", "PDF został wygenerowany i otwarty w podglądzie.");
        }
        
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Błąd", QString("Wystąpił błąd: %1").arg(e.what()));
    }
}

void PrintDialog::printAndEmailConfirmation() {
    qDebug() << "[PrintDialog] Drukowanie i wysyłanie potwierdzenia dla zamówienia:" << m_orderId;
    
    try {
        // Najpierw wygeneruj PDF
        auto& dbm = DbManager::instance();
        auto orderData = dbm.getOrderById(m_orderId);
        
        if (orderData.isEmpty()) {
            QMessageBox::warning(this, "Błąd", "Nie można pobrać danych zamówienia.");
            return;
        }
        
        // Pobierz dane klienta
        int clientId = orderData["client_id"].toInt();
        QMap<QString, QVariant> clientData;
        auto clients = dbm.getClients();
        for (const auto& c : clients) {
            if (c["id"].toInt() == clientId) {
                clientData = c;
                break;
            }
        }
        
        // Pobierz pozycje zamówienia
        auto orderItems = dbm.getOrderItems(m_orderId);
        
        // Pobierz ścieżkę z ustawień
        QSettings settings;
        QString dir = settings.value("pdf/confirmationDir", "").toString();
        if (dir.isEmpty()) {
            // Użyj domyślnego katalogu z dokumentów użytkownika  
            dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/EtykietyManager/Potwierdzenia";
            settings.setValue("pdf/confirmationDir", dir);
            qDebug() << "[PrintDialog] Ustawiono domyślny katalog potwierdzeń dla printAndEmailConfirmation:" << dir;
        }
        
        // Utwórz katalog jeśli nie istnieje
        QDir().mkpath(dir);
        
        // Generuj PDF potwierdzenia
        QString fileName = QString("%1_%2_POTWIERDZENIE.pdf")
            .arg(orderData["order_number"].toString())
            .arg(clientData["name"].toString().replace(" ", "_"));
        QString outputPath = dir + "/" + fileName;
        
        // Użyj nowego generatora Python
        PythonPdfGenerator pdfGenerator;
        if (!pdfGenerator.generateOrderConfirmation(orderData, clientData, orderItems, outputPath)) {
            QMessageBox::warning(this, "Błąd", 
                QString("Nie udało się wygenerować pliku PDF:\n%1").arg(pdfGenerator.getLastError()));
            return;
        }
        
        // Sprawdź czy klient ma adres e-mail
        QString clientEmail = clientData["email"].toString().trimmed();
        if (clientEmail.isEmpty()) {
            QMessageBox::warning(this, "Błąd", 
                QString("Klient %1 nie ma podanego adresu e-mail.\nNie można wysłać potwierdzenia.")
                .arg(clientData["name"].toString()));
            return;
        }
        
        // Otwórz podgląd PDF
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(outputPath))) {
            QMessageBox::warning(this, "Ostrzeżenie", "Nie można otworzyć pliku PDF w podglądzie.");
        }
        
        // Wyślij e-mail z załącznikiem PDF
        if (!m_emailSender) {
            m_emailSender = new EmailSender(this);
            
            // Podłączamy sygnał tylko raz, przy tworzeniu obiektu
            connect(m_emailSender, &EmailSender::emailSent, this, &PrintDialog::onEmailSent);
        }
        
        EmailSender::EmailData emailData;
        emailData.toEmail = clientEmail;
        emailData.toName = clientData["name"].toString();
        emailData.subject = QString("Potwierdzenie zamowienia nr %1 - Termedia")
                           .arg(orderData["order_number"].toString());
        emailData.body = QString("Dzień dobry,\n\n"
                                "Dziękujemy za złożenie zamówienia w firmie Termedia.\n\n"
                                "W załączniku przesyłamy oficjalne potwierdzenie zamówienia nr %1.\n\n"
                                "Szczegóły zamówienia:\n"
                                "• Numer zamówienia: %2\n"
                                "• Nr klienta: %3\n"
                                "• Planowana data dostawy: %4\n"
                                "• Klient: %5\n\n"
                                "Prosimy o zachowanie tego potwierdzenia do momentu realizacji zamówienia.\n\n"
                                "W przypadku pytań lub potrzeby wprowadzenia zmian, prosimy o kontakt:\n"
                                "• Email: dorota.chmiel@termedialabels.pl\n"
                                "• Email: bok@termedialabels.pl\n"
                                "• Telefon: zgodnie z danymi kontaktowymi Państwa opiekuna handlowego\n\n"
                                "Dziękujemy za zaufanie i współpracę.\n\n"
                                "Zespół Termedia\n"
                                "www.termedialabels.pl")
                        .arg(orderData["order_number"].toString())
                        .arg(orderData["order_number"].toString())
                        .arg(clientData["client_number"].toString())
                        .arg(orderData["delivery_date"].toDate().toString("dd.MM.yyyy"))
                        .arg(clientData["name"].toString());
        emailData.attachmentPath = outputPath;
        emailData.attachmentName = fileName;
        
        // Zablokuj przyciski podczas wysyłania
        btnPrintAndEmail->setEnabled(false);
        btnPrintAndEmail->setText("Wysyłanie...");
        
        // Uruchom wysyłanie e-maila
        qDebug() << "[PrintDialog] Rozpoczynanie wysyłania e-maila na:" << clientEmail;
        m_emailSender->sendEmail(emailData);
        
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Błąd", QString("Wystąpił błąd: %1").arg(e.what()));
    }
}

void PrintDialog::printProduction() {
    qDebug() << "[PrintDialog] Drukowanie dla produkcji zamówienia:" << m_orderId;
    
    try {
        // Pobierz dane zamówienia z bazy
        auto& dbm = DbManager::instance();
        auto orderData = dbm.getOrderById(m_orderId);
        
        if (orderData.isEmpty()) {
            QMessageBox::warning(this, "Błąd", "Nie można pobrać danych zamówienia.");
            return;
        }
        
        // Pobierz dane klienta
        int clientId = orderData["client_id"].toInt();
        QMap<QString, QVariant> clientData;
        auto clients = dbm.getClients();
        for (const auto& c : clients) {
            if (c["id"].toInt() == clientId) {
                clientData = c;
                break;
            }
        }
        
        // Pobierz pozycje zamówienia
        auto orderItems = dbm.getOrderItems(m_orderId);
        
        // Pobierz ścieżkę z ustawień
        QSettings settings;
        QString dir = settings.value("pdf/productionDir", "").toString();
        if (dir.isEmpty()) {
            // Użyj domyślnego katalogu z dokumentów użytkownika  
            dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/EtykietyManager/Produkcja";
            settings.setValue("pdf/productionDir", dir);
            qDebug() << "[PrintDialog] Ustawiono domyślny katalog produkcji:" << dir;
        }
        
        // Utwórz katalog jeśli nie istnieje
        QDir().mkpath(dir);
        
        // Generuj PDF dla produkcji
        QString fileName = QString("%1_%2_PRODUKCJA.pdf")
            .arg(orderData["order_number"].toString())
            .arg(clientData["name"].toString().replace(" ", "_"));
        QString outputPath = dir + "/" + fileName;
        
        // Użyj nowego generatora Python
        PythonPdfGenerator pdfGenerator;
        if (!pdfGenerator.generateProductionTicket(orderData, clientData, orderItems, outputPath)) {
            QMessageBox::warning(this, "Błąd", 
                QString("Nie udało się wygenerować pliku PDF:\n%1").arg(pdfGenerator.getLastError()));
            return;
        }
        
        // Otwórz podgląd PDF
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(outputPath))) {
            QMessageBox::warning(this, "Błąd", "Nie można otworzyć pliku PDF w podglądzie.");
        } else {
            QMessageBox::information(this, "Sukces", "PDF dla produkcji został wygenerowany i otwarty.");
        }
        
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Błąd", QString("Wystąpił błąd: %1").arg(e.what()));
    }
}

void PrintDialog::onEmailSent(bool success, const QString &message) {
    qDebug() << "[PrintDialog] onEmailSent wywołane - sukces:" << success << "wiadomość:" << message;
    
    // Przywróć stan przycisku
    btnPrintAndEmail->setEnabled(true);
    btnPrintAndEmail->setText("Drukuj i wyślij");
    
    if (success) {
        QMessageBox::information(this, "Sukces", 
            QString("E-mail został wysłany pomyślnie!\n\n%1").arg(message));
    } else {
        QMessageBox::critical(this, "Błąd wysyłania e-maila", 
            QString("Nie udało się wysłać e-maila:\n\n%1").arg(message));
    }
}
