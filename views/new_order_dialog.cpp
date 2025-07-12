#include "client_select_dialog.h"
#include "new_order_dialog.h"
#include "models/client.h"
#include "network/gusclient.h" // Dodany include dla GusClient
#include "views/client_full_dialog.h"
#include "mainwindow.h" // Dodany include dla MainWindow
#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QDateEdit>
#include <QTextEdit>
#include <QGroupBox>
#include <QScrollArea>
#include <QMessageBox>
#include <QCalendarWidget>
#include <QFont>
#include <QApplication>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include "db/dbmanager.h"
#include <QTableWidget>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QSettings>

NewOrderDialog::NewOrderDialog(QWidget *parent)
    : QDialog(parent)
{
    qDebug() << "[NewOrderDialog] Konstruktor wywołany";
    setupUi();
    // Połączenia przycisków klienta i adresu
    connect(btnSelectClient, &QPushButton::clicked, this, &NewOrderDialog::handleSelectClientFromDb);
    connect(btnAddrFromDb, &QPushButton::clicked, this, &NewOrderDialog::handleSelectDeliveryAddress);
    connect(btnAddPosition, &QPushButton::clicked, this, [this](){ addProductBlock(); });
    connect(btnSaveClient, &QPushButton::clicked, this, &NewOrderDialog::handleSaveClient);
    // Połączenie przycisku zapisu zamówienia
    qDebug() << "[NewOrderDialog] Przed connect btnSave do saveOrder, btnSave ptr:" << btnSave;
    connect(btnSave, &QPushButton::clicked, this, &NewOrderDialog::saveOrder);
    // Połącz sygnał clientAdded z MainWindow (globalne odświeżanie klientów)
    if (QWidget *w = QApplication::activeWindow()) {
        MainWindow *mw = qobject_cast<MainWindow*>(w);
        if (mw) {
            connect(this, &NewOrderDialog::clientAdded, mw, &MainWindow::clientAdded);
        }
    }
}

NewOrderDialog::~NewOrderDialog() {}

void NewOrderDialog::setupUi() {
    setWindowTitle("WPROWADŹ DANE ZAMÓWIENIA");
    setMinimumWidth(1200);
    setMaximumWidth(1800);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Tytuł
    titleLabel = new QLabel("WPROWADŹ DANE ZAMÓWIENIA");
    QFont titleFont;
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #147a3a;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // ScrollArea na całość formularza
    formArea = new QScrollArea(this);
    formArea->setWidgetResizable(true);
    QWidget *formWidget = new QWidget;
    formLayout = new QVBoxLayout(formWidget);
    formLayout->setSpacing(18);

    // --- Sekcja: Podstawowe dane zamówienia ---
    QHBoxLayout *row1 = new QHBoxLayout;
    row1->addWidget(new QLabel("Nr zamówienia:"));
    nrEdit = new QLineEdit;
    nrEdit->setReadOnly(true);
    nrEdit->setMaximumWidth(260);
    nrEdit->setMinimumWidth(180);
    row1->addWidget(nrEdit);
    row1->addSpacing(16);
    row1->addWidget(new QLabel("Data zamówienia:"));
    orderDateEdit = new QDateEdit(QDate::currentDate());
    orderDateEdit->setCalendarPopup(true);
    orderDateEdit->setMaximumWidth(150);
    row1->addWidget(orderDateEdit);
    row1->addSpacing(16);
    row1->addWidget(new QLabel("Data wysyłki:"));
    deliveryDateEdit = new QDateEdit(QDate::currentDate());
    deliveryDateEdit->setCalendarPopup(true);
    deliveryDateEdit->setMaximumWidth(150);
    row1->addWidget(deliveryDateEdit);
    row1->addStretch(1);
    formLayout->addLayout(row1);

    // --- Sekcja: Dane zamawiającego i adres dostawy w jednym rzędzie ---
    QHBoxLayout *clientsAndAddressLayout = new QHBoxLayout;
    // Dane zamawiającego
    QGroupBox *clientGroup = new QGroupBox("Dane zamawiającego");
    QGridLayout *clientGrid = new QGridLayout(clientGroup);
    clientGrid->addWidget(new QLabel("Nr klienta:"), 0, 0);
    clientNumberEdit = new QLineEdit;
    clientNumberEdit->setMaximumWidth(170);
    clientGrid->addWidget(clientNumberEdit, 0, 1);
    clientGrid->addWidget(new QLabel("Termin płatności:"), 0, 2);
    paymentTermCombo = new QComboBox;
    paymentTermCombo->addItems({"7 dni", "14 dni", "21 dni", "30 dni", "45 dni"});
    paymentTermCombo->setMaximumWidth(110);
    clientGrid->addWidget(paymentTermCombo, 0, 3);
    btnSelectClient = new QPushButton("Wstaw z bazy");
    btnSaveClient = new QPushButton("Zapisz dane klienta");
    // Usunięto btnAddClient
    QHBoxLayout *clientBtnLayout = new QHBoxLayout;
    clientBtnLayout->addWidget(btnSelectClient);
    clientBtnLayout->addWidget(btnSaveClient);
    clientGrid->addLayout(clientBtnLayout, 0, 4, 1, 2); // 2 kolumny, bo tylko dwa przyciski
    // Pola tekstowe klienta
    QStringList clientLabels = {"Firma", "Nazwa skróc.", "Osoba kontaktowa", "Nr telefonu", "E-mail", "Ulica i nr", "Kod pocztowy", "Miasto", "NIP"};
    for (int i = 0; i < clientLabels.size(); ++i) {
        clientGrid->addWidget(new QLabel(clientLabels[i] + ":"), i+1, 0);
        QLineEdit *edit = new QLineEdit;
        edit->setMaximumWidth(240);
        
        if (clientLabels[i] == "NIP") {
            // Dla pola NIP dodajemy przycisk GUS
            QHBoxLayout *nipLayout = new QHBoxLayout;
            nipLayout->setSpacing(4);
            nipLayout->addWidget(edit);
            
            // Przycisk GUS
            QPushButton *btnGUS = new QPushButton();
            btnGUS->setToolTip("Pobierz dane z GUS");
            btnGUS->setFixedSize(28, 28);
            QIcon gusIcon(":/icons/gus.png");
            if (!gusIcon.isNull()) {
                btnGUS->setIcon(gusIcon);
                btnGUS->setIconSize(QSize(22, 22));
            } else {
                btnGUS->setText("GUS");
            }
            btnGUS->setStyleSheet("QPushButton { border: none; background: transparent; } QPushButton:hover { background: #e0edff; }");
            connect(btnGUS, &QPushButton::clicked, this, [this, edit]() { this->fetchGusData(edit->text()); });
            nipLayout->addWidget(btnGUS);
            clientGrid->addLayout(nipLayout, i+1, 1, 1, 5, Qt::AlignLeft);
        } else {
            clientGrid->addWidget(edit, i+1, 1, 1, 5);
        }
        clientFields.append(edit);
    }
    clientsAndAddressLayout->addWidget(clientGroup, 2);

    // Adres dostawy
    deliveryAddrGroup = new QGroupBox("Adres dostawy");
    QGridLayout *addrGrid = new QGridLayout(deliveryAddrGroup);
    btnAddrFromDb = new QPushButton("Wstaw z bazy");
    addrGrid->addWidget(btnAddrFromDb, 0, 0, 1, 1);
    // Dodaj przycisk Zapisz adres
    btnSaveDeliveryAddr = new QPushButton("Zapisz adres");
    addrGrid->addWidget(btnSaveDeliveryAddr, 0, 1, 1, 1);
    connect(btnSaveDeliveryAddr, &QPushButton::clicked, this, &NewOrderDialog::handleSaveDeliveryAddress);
    QStringList addrLabels = {"Nazwa firmy:", "Ulica i nr:", "Kod pocztowy:", "Miejscowość:", "Osoba kont.:", "Tel.:", };
    QVector<int> addrWidths = {250, 200, 80, 150, 180, 120};
    for (int i = 0; i < addrLabels.size(); ++i) {
        QLabel *lbl = new QLabel(addrLabels[i]);
        lbl->setFixedWidth(100);
        addrGrid->addWidget(lbl, i+1, 0);
        QLineEdit *edit = new QLineEdit;
        edit->setFixedWidth(addrWidths[i]);
        addrGrid->addWidget(edit, i+1, 1);
        addrFields.append(edit);
    }
    clientsAndAddressLayout->addWidget(deliveryAddrGroup, 2);
    formLayout->addLayout(clientsAndAddressLayout);

    // --- Sekcja: Pozycje zamówienia ---
    prodContainer = new QGroupBox("Dane produkcji");
    prodMainLayout = new QVBoxLayout(prodContainer);
    prodGrid = new QGridLayout;
    prodMainLayout->addLayout(prodGrid);
    // Domyślnie dwa bloki produkcji
    addProductBlock(1);
    addProductBlock(2);
    btnAddPosition = new QPushButton("Dodaj pozycję");
    btnAddPosition->setMinimumWidth(180);
    btnAddPosition->setMaximumWidth(260);
    btnAddPosition->setMinimumHeight(44);
    btnAddPosition->setStyleSheet("font-size:14px;font-weight:bold;border-radius:7px;border:2px solid #197a3d;background:#eaffea;color:#197a3d;padding:7px 20px;margin:0 6px;");
    prodMainLayout->addWidget(btnAddPosition, 0, Qt::AlignLeft);
    formLayout->addWidget(prodContainer);

    // --- Sekcja: Uwagi ---
    QGroupBox *notesGroup = new QGroupBox("Uwagi");
    QVBoxLayout *notesLayout = new QVBoxLayout(notesGroup);
    notesEdit = new QTextEdit;
    notesEdit->setPlaceholderText("Wpisz dodatkowe uwagi do zamówienia...");
    notesLayout->addWidget(notesEdit);
    formLayout->addWidget(notesGroup);

    // --- Przycisk zapisu ---
    btnSave = new QPushButton("💾 Zapisz zamówienie");
    btnSave->setMinimumWidth(280);
    btnSave->setMinimumHeight(54);
    formLayout->addWidget(btnSave, 0, Qt::AlignCenter);

    formArea->setWidget(formWidget);
    mainLayout->addWidget(formArea);
}

// --- Dialog wyboru klienta ---
// Usunięto lokalną definicję ClientSelectDialog, bo jest globalna wersja w osobnych plikach

// --- Dialog wyboru adresu dostawy ---
class DeliveryAddressSelectDialog : public QDialog {
public:
    DeliveryAddressSelectDialog(QSqlDatabase db, int clientId, QWidget *parent = nullptr) : QDialog(parent), m_db(db), clientId(clientId) {
        setWindowTitle("Wybierz adres dostawy");
        resize(700, 400);
        QVBoxLayout *layout = new QVBoxLayout(this);
        table = new QTableWidget(this);
        table->setColumnCount(6);
        table->setHorizontalHeaderLabels({"Nazwa", "Firma", "Miasto", "Ulica", "Kod pocztowy", "ID"});
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        layout->addWidget(table);
        QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        layout->addWidget(btnBox);
        connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        loadAddresses(clientId);
    }
    void loadAddresses(int clientId) {
        table->setRowCount(0);
        QSqlQuery q(m_db);
        q.prepare("SELECT id, name, company, city, street, postal_code FROM delivery_addresses WHERE client_id = ? ORDER BY name");
        q.addBindValue(clientId);
        q.exec();
        int row = 0;
        while (q.next()) {
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(q.value(1).toString())); // nazwa
            table->setItem(row, 1, new QTableWidgetItem(q.value(2).toString())); // firma
            table->setItem(row, 2, new QTableWidgetItem(q.value(3).toString())); // miasto
            table->setItem(row, 3, new QTableWidgetItem(q.value(4).toString())); // ulica
            table->setItem(row, 4, new QTableWidgetItem(q.value(5).toString())); // kod pocztowy
            table->setItem(row, 5, new QTableWidgetItem(q.value(0).toString())); // id (ukryj w UI)
            table->setRowHeight(row, 26);
            row++;
        }
        table->setColumnHidden(5, true); // ukryj kolumnę ID
    }
    int selectedAddressId() const {
        auto sel = table->currentRow();
        if (sel < 0) return -1;
        return table->item(sel, 5)->text().toInt();
    }
    int clientId;
private:
    QSqlDatabase m_db;
    QTableWidget *table;
};

// --- Generowanie numeru zamówienia (ciąg order_sequence) ---
QString NewOrderDialog::generateOrderNumber(QSqlDatabase db) {
    int currentYear = QDate::currentDate().year();
    int lastNumber = 0;
    int lastYear = 0;
    // Pobierz ostatni numer i rok z bazy
    QSqlQuery q(db);
    if (!q.exec("SELECT last_number, year FROM order_sequence WHERE id=1 FOR UPDATE")) {
        return "";
    }
    if (q.next()) {
        lastNumber = q.value(0).toInt();
        lastYear = q.value(1).toInt();
    }
    int nextNumber = 0;
    if (lastYear != currentYear) {
        // Nowy rok – reset numeracji
        nextNumber = 1; // od nowego roku zawsze 00001
    } else if (lastNumber == 0) {
        // Pierwsze uruchomienie/migracja – start od 00601
        nextNumber = 601;
    } else {
        nextNumber = lastNumber + 1;
    }
    // Zaktualizuj sekwencję
    QSqlQuery q2(db);
    q2.prepare("UPDATE order_sequence SET last_number=?, year=? WHERE id=1");
    q2.addBindValue(nextNumber);
    q2.addBindValue(currentYear);
    if (!q2.exec()) return "";
    // Format numeru zamówienia: YYYY/MM/XXXXX
    QString year = QString::number(currentYear);
    QString month = QString::number(QDate::currentDate().month()).rightJustified(2, '0');
    QString num = QString("%1/%2/%3").arg(year).arg(month).arg(nextNumber, 5, 10, QChar('0'));
    return num;
}

// --- Walidacja formularza zamówienia ---
bool NewOrderDialog::validateOrderForm(QString &errors) {
    errors.clear();
    // Walidacja klienta
    if (clientFields[0]->text().trimmed().isEmpty())
        errors += "- Podaj nazwę firmy (zamawiający).\n";
    if (clientFields[3]->text().trimmed().isEmpty())
        errors += "- Podaj numer telefonu klienta.\n";
    if (!clientFields[4]->text().isEmpty() && !QRegularExpression(R"(^[\w\.-]+@[\w\.-]+\.[a-zA-Z]{2,}$)").match(clientFields[4]->text()).hasMatch())
        errors += "- Nieprawidłowy e-mail klienta.\n";
    // Walidacja adresu dostawy
    if (addrFields[0]->text().trimmed().isEmpty())
        errors += "- Podaj nazwę firmy (adres dostawy).\n";
    if (addrFields[1]->text().trimmed().isEmpty())
        errors += "- Podaj ulicę i nr (adres dostawy).\n";
    if (addrFields[2]->text().trimmed().isEmpty())
        errors += "- Podaj kod pocztowy (adres dostawy).\n";
    if (addrFields[3]->text().trimmed().isEmpty())
        errors += "- Podaj miejscowość (adres dostawy).\n";
    // Walidacja pozycji zamówienia
    // if (prodFieldsList.isEmpty())
    //     errors += "- Dodaj przynajmniej jedną pozycję produktu.\n";
    for (int i = 0; i < prodFieldsList.size(); ++i) {
        auto &p = prodFieldsList[i];
        // if (static_cast<QLineEdit*>(p["Szerokość"])->text().trimmed().isEmpty())
        //     errors += QString("- Pozycja %1: podaj szerokość.\n").arg(i+1);
        // if (static_cast<QLineEdit*>(p["Wysokość"])->text().trimmed().isEmpty())
        //     errors += QString("- Pozycja %1: podaj wysokość.\n").arg(i+1);
        // if (static_cast<QLineEdit*>(p["zam. ilość"])->text().trimmed().isEmpty())
        //     errors += QString("- Pozycja %1: podaj ilość.\n").arg(i+1);
        // if (static_cast<QLineEdit*>(p["nawój/długość"])->text().trimmed().isEmpty())
        //     errors += QString("- Pozycja %1: podaj nawój/długość.\n").arg(i+1);
        // if (static_cast<QComboBox*>(p["Rodzaj materiału"])->currentText().trimmed().isEmpty())
        //     errors += QString("- Pozycja %1: wybierz rodzaj materiału.\n").arg(i+1);
        // if (static_cast<QComboBox*>(p["Rdzeń"])->currentText() == "inny" && static_cast<QLineEdit*>(p["Rdzeń_inny"])->text().trimmed().isEmpty())
        //     errors += QString("- Pozycja %1: podaj wartość rdzenia (inny).\n").arg(i+1);
    }
    // Walidacja daty wysyłki (przykład: nie może być wstecz)
    if (deliveryDateEdit->date() < orderDateEdit->date())
        errors += "- Data wysyłki nie może być wcześniejsza niż data zamówienia.\n";
    // Walidacja NIP (tylko 10 cyfr, bez myślników)
    QString nip = clientFields[8]->text().trimmed();
    QString cleanNip = ClientFullDialog::cleanNip(nip);
    if (!cleanNip.isEmpty() && !QRegularExpression(R"(^\d{10}$)").match(cleanNip).hasMatch())
        errors += "- Nieprawidłowy format NIP (dozwolone: 10 cyfr).\n";
    return errors.isEmpty();
}

// --- Zapis zamówienia i pozycji do bazy ---
void NewOrderDialog::saveOrder() {
    qDebug() << "[saveOrder] Kliknięto Zapisz";
    QString errors;
    bool valid = validateOrderForm(errors);
    qDebug() << "[saveOrder] Wynik validateOrderForm:" << valid << ", errors:" << errors;
    qDebug() << "[saveOrder] NIP w polu (przed walidacją):" << clientFields[8]->text();
    qDebug() << "[saveOrder] NIP po cleanNip (przed walidacją):" << ClientFullDialog::cleanNip(clientFields[8]->text());
    if (!valid) {
        QMessageBox::warning(this, "Błąd danych", errors);
        return;
    }
    auto& dbm = DbManager::instance();
    QSqlDatabase db = dbm.database();
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Błąd bazy", "Brak połączenia z bazą danych.");
        return;
    }
    db.transaction();
    // --- Generuj numer zamówienia ---
    QString orderNumber = generateOrderNumber(db);
    if (orderNumber.isEmpty()) {
        db.rollback();
        QMessageBox::critical(this, "Błąd bazy", "Nie udało się wygenerować numeru zamówienia.");
        return;
    }
    // --- Dodaj zamówienie ---
    QSqlQuery q(db);
    q.prepare("INSERT INTO orders (order_number, order_date, delivery_date, client_id, notes, payment_term, delivery_company, delivery_street, delivery_postal_code, delivery_city, delivery_contact_person, delivery_phone, status) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    QDate orderDate = orderDateEdit->date();
    QDate deliveryDate = deliveryDateEdit->date();
    int clientId = clientNumberEdit->text().toInt();
    QString notes = notesEdit ? notesEdit->toPlainText() : "";
    QString paymentTerm = paymentTermCombo->currentText();
    q.addBindValue(orderNumber);
    q.addBindValue(orderDate);
    q.addBindValue(deliveryDate);
    q.addBindValue(clientId);
    q.addBindValue(notes);
    q.addBindValue(paymentTerm);
    for (int i = 0; i < 6; ++i) q.addBindValue(addrFields[i]->text());
    q.addBindValue(1); // status: 1 = przyjęte do realizacji
    qDebug() << "[saveOrder] Przed exec INSERT orders, clientId:" << clientId;
    if (!q.exec()) {
        db.rollback();
        QMessageBox::critical(this, "Błąd bazy", "Nie udało się dodać zamówienia: " + q.lastError().text());
        return;
    }
    int orderId = q.lastInsertId().toInt();
    qDebug() << "[saveOrder] Dodano zamówienie, orderId:" << orderId;
    // --- Dodaj pozycje zamówienia ---
    for (const auto &p : prodFieldsList) {
        QSqlQuery q2(db);
        q2.prepare("INSERT INTO order_items (order_id, width, height, material, ordered_quantity, quantity_type, roll_length, core, price, price_type, zam_rolki) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        q2.addBindValue(orderId);
        q2.addBindValue(static_cast<QLineEdit*>(p["Szerokość"])->text());
        q2.addBindValue(static_cast<QLineEdit*>(p["Wysokość"])->text());
        q2.addBindValue(static_cast<QComboBox*>(p["Rodzaj materiału"])->currentText());
        q2.addBindValue(static_cast<QLineEdit*>(p["zam. ilość"])->text());
        q2.addBindValue(static_cast<QComboBox*>(p["Typ ilości"])->currentText());
        q2.addBindValue(static_cast<QLineEdit*>(p["nawój/długość"])->text());
        QString core = static_cast<QComboBox*>(p["Rdzeń"])->currentText() == "inny" ? static_cast<QLineEdit*>(p["Rdzeń_inny"])->text() : static_cast<QComboBox*>(p["Rdzeń"])->currentText();
        q2.addBindValue(core);
        q2.addBindValue(static_cast<QLineEdit*>(p["Cena"])->text());
        q2.addBindValue(static_cast<QComboBox*>(p["CenaTyp"])->currentText());
        q2.addBindValue(static_cast<QLineEdit*>(p["zam. rolki"])->text());
        qDebug() << "[saveOrder] Dodaję pozycję zamówienia, orderId:" << orderId;
        if (!q2.exec()) {
            db.rollback();
            QMessageBox::critical(this, "Błąd bazy", "Nie udało się dodać pozycji zamówienia: " + q2.lastError().text());
            return;
        }
    }
    db.commit();
    qDebug() << "[saveOrder] Zamówienie zapisane, orderNumber:" << orderNumber;
    QMessageBox::information(this, "Sukces", "Zamówienie zostało zapisane.\nNumer: " + orderNumber);
    accept();
}

// --- Logika wyboru klienta i adresu ---
void NewOrderDialog::handleSelectClientFromDb() {
    auto& dbm = DbManager::instance();
    QSqlDatabase db = dbm.database();
    ClientSelectDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QMap<QString, QVariant> client = dlg.selectedClient();
        int clientId = client["id"].toInt();
        if (client.contains("client_number"))
            clientNumberEdit->setText(client["client_number"].toString());
        if (client.contains("name"))
            clientFields[0]->setText(client["name"].toString());
        if (client.contains("short_name"))
            clientFields[1]->setText(client["short_name"].toString());
        if (client.contains("contact_person"))
            clientFields[2]->setText(client["contact_person"].toString());
        if (client.contains("phone"))
            clientFields[3]->setText(client["phone"].toString());
        if (client.contains("email"))
            clientFields[4]->setText(client["email"].toString());
        if (client.contains("street"))
            clientFields[5]->setText(client["street"].toString());
        if (client.contains("postal_code"))
            clientFields[6]->setText(client["postal_code"].toString());
        if (client.contains("city"))
            clientFields[7]->setText(client["city"].toString());
        if (client.contains("nip")) {
            QString cleanNip = ClientFullDialog::cleanNip(client["nip"].toString());
            if (!cleanNip.isEmpty() && cleanNip.length() == 10) {
                clientFields[8]->setText(cleanNip);
            } else {
                QMessageBox::warning(this, "Błąd NIP", "Wybrany klient ma nieprawidłowy NIP (po oczyszczeniu nie jest 10 cyfr).\nNie można wybrać tego klienta do zamówienia.");
                clientFields[8]->clear();
                return;
            }
        }
    }
}

void NewOrderDialog::addProductBlock(int number) {
    // Utwórz nowy widget bloku pozycji
    QWidget *blockWidget = new QWidget;
    QGridLayout *layout = new QGridLayout(blockWidget);
    QMap<QString, QWidget*> fields;

    // Pola pozycji zamówienia
    QLineEdit *widthEdit = new QLineEdit;
    QLineEdit *heightEdit = new QLineEdit;
    QComboBox *materialCombo = new QComboBox;
    materialCombo->addItems({"Folia A", "Folia B", "Papier", "Inny"});
    QLineEdit *quantityEdit = new QLineEdit;
    QComboBox *quantityTypeCombo = new QComboBox;
    quantityTypeCombo->addItems({"szt.", "m2", "rolki"});
    QLineEdit *rollLengthEdit = new QLineEdit;
    QComboBox *coreCombo = new QComboBox;
    coreCombo->addItems({"25", "40", "76", "inny"});
    QLineEdit *coreOtherEdit = new QLineEdit;
    coreOtherEdit->setPlaceholderText("Inny rdzeń");
    coreOtherEdit->setVisible(false);
    QLineEdit *priceEdit = new QLineEdit;
    QComboBox *priceTypeCombo = new QComboBox;
    priceTypeCombo->addItems({"netto", "brutto"});
    QLineEdit *rollsEdit = new QLineEdit;

    // Obsługa widoczności pola "inny rdzeń"
    connect(coreCombo, &QComboBox::currentTextChanged, blockWidget, [coreCombo, coreOtherEdit]() {
        coreOtherEdit->setVisible(coreCombo->currentText() == "inny");
    });

    // Dodaj pola do layoutu
    int col = 0;
    layout->addWidget(new QLabel("Szerokość"), 0, col); layout->addWidget(widthEdit, 1, col++);
    layout->addWidget(new QLabel("Wysokość"), 0, col); layout->addWidget(heightEdit, 1, col++);
    layout->addWidget(new QLabel("Rodzaj materiału"), 0, col); layout->addWidget(materialCombo, 1, col++);
    layout->addWidget(new QLabel("zam. ilość"), 0, col); layout->addWidget(quantityEdit, 1, col++);
    layout->addWidget(new QLabel("Typ ilości"), 0, col); layout->addWidget(quantityTypeCombo, 1, col++);
    layout->addWidget(new QLabel("nawój/długość"), 0, col); layout->addWidget(rollLengthEdit, 1, col++);
    layout->addWidget(new QLabel("Rdzeń"), 0, col); layout->addWidget(coreCombo, 1, col++);
    layout->addWidget(coreOtherEdit, 1, col-1);
    layout->addWidget(new QLabel("Cena"), 0, col); layout->addWidget(priceEdit, 1, col++);
    layout->addWidget(new QLabel("CenaTyp"), 0, col); layout->addWidget(priceTypeCombo, 1, col++);
    layout->addWidget(new QLabel("zam. rolki"), 0, col); layout->addWidget(rollsEdit, 1, col++);

    // Przycisk usuwania pozycji
    QPushButton *removeBtn = new QPushButton("Usuń");
    layout->addWidget(removeBtn, 1, col);
    connect(removeBtn, &QPushButton::clicked, this, [this, blockWidget]() {
        removeProductBlock(blockWidget);
    });

    // Dodaj pola do mapy
    fields["Szerokość"] = widthEdit;
    fields["Wysokość"] = heightEdit;
    fields["Rodzaj materiału"] = materialCombo;
    fields["zam. ilość"] = quantityEdit;
    fields["Typ ilości"] = quantityTypeCombo;
    fields["nawój/długość"] = rollLengthEdit;
    fields["Rdzeń"] = coreCombo;
    fields["Rdzeń_inny"] = coreOtherEdit;
    fields["Cena"] = priceEdit;
    fields["CenaTyp"] = priceTypeCombo;
    fields["zam. rolki"] = rollsEdit;

    prodFieldsList.append(fields);
    prodBlocks.append(blockWidget);
    prodGrid->addWidget(blockWidget, prodBlocks.size(), 0);
}

void NewOrderDialog::removeProductBlock(QWidget *blockWidget) {
    int idx = prodBlocks.indexOf(blockWidget);
    if (idx >= 0) {
        prodGrid->removeWidget(blockWidget);
        blockWidget->deleteLater();
        prodBlocks.remove(idx);
        prodFieldsList.remove(idx);
        relayoutProductBlocks();
    }
}

void NewOrderDialog::relayoutProductBlocks() {
    // Usuń wszystkie widgety z prodGrid
    QLayoutItem *item;
    while ((item = prodGrid->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->setParent(nullptr);
        delete item;
    }
    // Dodaj ponownie wszystkie bloki
    for (int i = 0; i < prodBlocks.size(); ++i) {
        prodGrid->addWidget(prodBlocks[i], i+1, 0);
    }
}

// --- Dialog do zapisu adresu dostawy ---
class SaveDeliveryAddressDialog : public QDialog {
public:
    SaveDeliveryAddressDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Zapisz adres dostawy");
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel("Podaj nazwę adresu dostawy:"));
        nameEdit = new QLineEdit;
        layout->addWidget(nameEdit);
        QHBoxLayout *btnLayout = new QHBoxLayout;
        QPushButton *btnSave = new QPushButton("Zapisz");
        QPushButton *btnCancel = new QPushButton("Anuluj");
        btnLayout->addWidget(btnSave);
        btnLayout->addWidget(btnCancel);
        layout->addLayout(btnLayout);
        connect(btnSave, &QPushButton::clicked, this, &QDialog::accept);
        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    }
    QLineEdit *nameEdit;
};

// --- Obsługa zapisu adresu dostawy ---
void NewOrderDialog::handleSaveDeliveryAddress() {
    // Sprawdź czy jest wybrany klient
    int clientId = clientNumberEdit->text().toInt();
    if (clientId <= 0) {
        QMessageBox::warning(this, "Brak klienta", "Najpierw wybierz lub zapisz klienta, aby przypisać adres dostawy.");
        return;
    }
    // Okno dialogowe z polem "nazwa"
    QDialog dialog(this);
    dialog.setWindowTitle("Zapisz adres dostawy");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *label = new QLabel("Podaj nazwę adresu (np. Magazyn, Biuro, Dom klienta):");
    QLineEdit *nameEdit = new QLineEdit;
    layout->addWidget(label);
    layout->addWidget(nameEdit);
    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    layout->addWidget(btnBox);
    connect(btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    QString addrName = nameEdit->text().trimmed();
    if (addrName.isEmpty()) {
        QMessageBox::warning(this, "Błąd", "Podaj nazwę adresu.");
        return;
    }
    // Zapisz adres do bazy
    auto& dbm = DbManager::instance();
    QSqlDatabase db = dbm.database();
    QSqlQuery q(db);
    q.prepare("INSERT INTO delivery_addresses (client_id, name, company, street, postal_code, city, contact_person, phone) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    q.addBindValue(clientId);
    q.addBindValue(addrName);
    for (int i = 0; i < 6; ++i) q.addBindValue(addrFields[i]->text());
    if (!q.exec()) {
        QMessageBox::critical(this, "Błąd bazy", "Nie udało się zapisać adresu dostawy: " + q.lastError().text());
        return;
    }
    QMessageBox::information(this, "Sukces", "Adres dostawy został zapisany do bazy.");
}

// --- Dialog wyboru adresu dostawy ---
void NewOrderDialog::handleSelectDeliveryAddress() {
    auto& dbm = DbManager::instance();
    QSqlDatabase db = dbm.database();
    int clientId = clientNumberEdit->text().toInt();
    if (clientId <= 0) {
        QMessageBox::warning(this, "Brak klienta", "Najpierw wybierz lub zapisz klienta, aby wybrać adres dostawy.");
        return;
    }
    DeliveryAddressSelectDialog dlg(db, clientId, this);
    if (dlg.exec() == QDialog::Accepted) {
        int addrId = dlg.selectedAddressId();
        if (addrId > 0) {
            QSqlQuery q(db);
            q.prepare("SELECT company, street, postal_code, city, contact_person, phone FROM delivery_addresses WHERE id = ?");
            q.addBindValue(addrId);
            if (q.exec() && q.next()) {
                for (int i = 0; i < 6; ++i) addrFields[i]->setText(q.value(i).toString());
            }
        }
    }
}

void NewOrderDialog::handleAddNewClient() {
    // Przykładowa implementacja: wyświetl komunikat lub zaimplementuj dodawanie nowego klienta
    QMessageBox::information(this, "Nowy klient", "Funkcja dodawania nowego klienta nie została jeszcze zaimplementowana.");
}

void NewOrderDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    // Możesz dodać tu własną logikę, jeśli potrzebujesz
}

void NewOrderDialog::handleSaveClient() {
    auto& dbm = DbManager::instance();
    QMap<QString, QVariant> data;
    QString nip = clientFields[8]->text().trimmed();
    // Walidacja NIP: tylko 10 cyfr
    QString cleanNip = ClientFullDialog::cleanNip(nip);
    if (!cleanNip.isEmpty() && !QRegularExpression(R"(^\d{10}$)").match(cleanNip).hasMatch()) {
        QMessageBox::warning(this, "Błąd NIP", "Nieprawidłowy format NIP (dozwolone: 10 cyfr).");
        return;
    }
    // Sprawdź, czy klient o tym NIP już istnieje
    if (!cleanNip.isEmpty()) {
        int existingId = dbm.findClientByNip(cleanNip);
        if (existingId != -1) {
            QMessageBox::warning(this, "Duplikat klienta", "Klient o podanym NIP już istnieje w bazie.");
            return;
        }
    }
    int maxTries = 10;
    bool added = false;
    while (maxTries-- > 0) {
        int nextNr = dbm.getNextUniqueClientNumber();
        QString formattedNr = QString::number(nextNr).rightJustified(6, '0');
        if (dbm.findClientByNumber(formattedNr) != -1) {
            continue;
        }
        data["client_number"] = formattedNr;
        data["name"] = clientFields[0]->text();
        data["short_name"] = clientFields[1]->text();
        data["contact_person"] = clientFields[2]->text();
        data["phone"] = clientFields[3]->text();
        data["email"] = clientFields[4]->text();
        data["street"] = clientFields[5]->text();
        data["postal_code"] = clientFields[6]->text();
        data["city"] = clientFields[7]->text();
        data["nip"] = cleanNip; // zawsze zapisuj oczyszczony NIP
        if (data["name"].toString().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Błąd", "Podaj nazwę firmy klienta.");
            return;
        }
        qDebug() << "[handleSaveClient] Dane do zapisu:" << data;
        bool ok = dbm.addClient(data);
        qDebug() << "[handleSaveClient] Wynik addClient:" << ok;
        if (ok) {
            added = true;
            break;
        }
    }
    if (!added) {
        QMessageBox::critical(this, "Błąd bazy", "Nie udało się wygenerować unikalnego numeru klienta. Spróbuj ponownie.");
        return;
    }
    // Pobierz numer klienta z bazy po zapisie (jeśli nie był ustawiony)
    if (clientNumberEdit->text().trimmed().isEmpty()) {
        QSqlQuery q(dbm.database());
        q.prepare("SELECT client_number FROM clients WHERE nip = ? OR name = ? ORDER BY id DESC LIMIT 1");
        q.addBindValue(cleanNip); // szukaj po oczyszczonym NIP
        q.addBindValue(data["name"]);
        bool execOk = q.exec();
        qDebug() << "[handleSaveClient] SELECT client_number execOk:" << execOk;
        if (execOk && q.next()) {
            clientNumberEdit->setText(q.value(0).toString());
            qDebug() << "[handleSaveClient] Ustawiono clientNumberEdit na:" << q.value(0).toString();
        } else {
            qDebug() << "[handleSaveClient] Nie znaleziono client_number po zapisie.";
        }
    } else {
        clientNumberEdit->setText(data["client_number"].toString());
        qDebug() << "[handleSaveClient] Ustawiono clientNumberEdit na (z danych):" << data["client_number"].toString();
    }
    emit clientAdded();
    addrFields[0]->setText(data["name"].toString());
    addrFields[1]->setText(data["street"].toString());
    addrFields[2]->setText(data["postal_code"].toString());
    addrFields[3]->setText(data["city"].toString());
    addrFields[4]->setText(data["contact_person"].toString());
    addrFields[5]->setText(data["phone"].toString());
    qDebug() << "[handleSaveClient] Ustawiono pola adresu dostawy.";
    QMessageBox::information(this, "Sukces", "Dane klienta zostały zapisane. Nadano numer: " + clientNumberEdit->text());
}

void NewOrderDialog::fetchGusData(const QString& nip) {
    QString cleanNip = nip;
    cleanNip.remove(QRegularExpression("[^0-9]")); // zostaw tylko cyfry
    
    // Sprawdzenie poprawności NIP
    if (cleanNip.isEmpty() || cleanNip.length() != 10) {
        QMessageBox messageBox(this);
        messageBox.setWindowTitle("Błędny NIP");
        messageBox.setText("Wprowadź poprawny numer NIP (10 cyfr).");
        messageBox.setInformativeText("Chcesz przetestować z jednym z przykładowych NIP-ów?");
        
        QPushButton *testGusBtn = messageBox.addButton("Testowy GUS (5261040828)", QMessageBox::ActionRole);
        QPushButton *companyBtn = messageBox.addButton("Firmowy (6381016402)", QMessageBox::ActionRole);
        QPushButton *cancelBtn = messageBox.addButton("Anuluj", QMessageBox::RejectRole);
        
        messageBox.exec();
        
        if (messageBox.clickedButton() == testGusBtn) {
            cleanNip = "5261040828"; // Testowy NIP z dokumentacji GUS
        } else if (messageBox.clickedButton() == companyBtn) {
            cleanNip = "6381016402"; // NIP firmy użytkownika
        } else {
            return;
        }
    }

    qDebug() << "[GUS] Pobieranie danych z GUS dla NIP:" << cleanNip;
    
    // Pokaż informację o trwającym pobieraniu
    QMessageBox *loadingMsg = new QMessageBox(this);
    loadingMsg->setWindowTitle("GUS");
    loadingMsg->setText("Trwa pobieranie danych z GUS...");
    loadingMsg->setStandardButtons(QMessageBox::NoButton);
    loadingMsg->show();
    QCoreApplication::processEvents();
    
    // Utwórz instancję GusClient
    GusClient *gusClient = new GusClient(this);
    
    // Połącz sygnał otrzymania danych
    connect(gusClient, &GusClient::companyDataReceived, this, [this, loadingMsg](const QMap<QString, QString> &data) {
        qDebug() << "[GUS] Otrzymano dane firmy z GUS. Dostępne pola:" << data.keys();
        
        // Mapowanie pól z GUS do pól formularza
        // 0: Firma, 1: Nazwa skróc., 2: Osoba kontaktowa, 3: Nr telefonu, 
        // 4: E-mail, 5: Ulica i nr, 6: Kod pocztowy, 7: Miasto, 8: NIP
        
        // Nazwa pełna
        if (data.contains("company_name") || data.contains("Nazwa")) {
            QString nazwaFirmy = data.value("company_name", data.value("Nazwa"));
            clientFields[0]->setText(nazwaFirmy);
            qDebug() << "[GUS] Ustawiono nazwę firmy:" << nazwaFirmy;
        }
        
        // Nazwa skrócona
        if (data.contains("short_name") || data.contains("NazwaSkrocona")) {
            QString nazwaSkrocona = data.value("short_name", data.value("NazwaSkrocona"));
            if (!nazwaSkrocona.isEmpty()) {
                clientFields[1]->setText(nazwaSkrocona);
                qDebug() << "[GUS] Ustawiono nazwę skróconą:" << nazwaSkrocona;
            }
        }
        
        // Adres - ulica i nr budynku/lokalu
        QString adres;
        if (data.contains("street") || data.contains("Ulica")) {
            adres = data.value("street", data.value("Ulica", ""));
        }
        
        if (data.contains("building_number") || data.contains("NrNieruchomosci")) {
            QString nrBudynku = data.value("building_number", data.value("NrNieruchomosci", ""));
            if (!nrBudynku.isEmpty()) {
                if (!adres.isEmpty()) adres += " ";
                adres += nrBudynku;
            }
        }
        
        if ((data.contains("apartment_number") && !data.value("apartment_number").isEmpty()) || 
            (data.contains("NrLokalu") && !data.value("NrLokalu").isEmpty())) {
            QString nrLokalu = data.value("apartment_number", data.value("NrLokalu", ""));
            if (!nrLokalu.isEmpty()) {
                adres += "/" + nrLokalu;
            }
        }
        
        if (!adres.isEmpty()) {
            clientFields[5]->setText(adres);
            qDebug() << "[GUS] Ustawiono adres:" << adres;
        }
        
        // Kod pocztowy
        if (data.contains("postal_code") || data.contains("KodPocztowy")) {
            QString kodPocztowy = data.value("postal_code", data.value("KodPocztowy", ""));
            clientFields[6]->setText(kodPocztowy);
            qDebug() << "[GUS] Ustawiono kod pocztowy:" << kodPocztowy;
        }
        
        // Miejscowość
        if (data.contains("city") || data.contains("Miejscowosc")) {
            QString miasto = data.value("city", data.value("Miejscowosc", ""));
            clientFields[7]->setText(miasto);
            qDebug() << "[GUS] Ustawiono miasto:" << miasto;
        }
        
        // NIP - zawsze ustawiamy tylko cyfry (bez formatowania)
        if (data.contains("nip") || data.contains("Nip")) {
            QString nipValue = data.value("nip", data.value("Nip", ""));
            nipValue.remove(QRegularExpression("[^0-9]")); // zostaw tylko cyfry
            clientFields[8]->setText(nipValue);
            qDebug() << "[GUS] Ustawiono NIP (bez formatowania):" << nipValue;
        }
        
        // Skopiuj dane do adresu dostawy (jeśli jest pusty)
        bool adresDostawyPusty = true;
        for (int i = 0; i < 4; i++) {
            if (!addrFields[i]->text().isEmpty()) {
                adresDostawyPusty = false;
                break;
            }
        }
        
        if (adresDostawyPusty) {
            addrFields[0]->setText(clientFields[0]->text()); // Nazwa firmy
            addrFields[1]->setText(clientFields[5]->text()); // Adres
            addrFields[2]->setText(clientFields[6]->text()); // Kod pocztowy
            addrFields[3]->setText(clientFields[7]->text()); // Miasto
            qDebug() << "[GUS] Skopiowano dane do adresu dostawy";
        }
        
        // Zamknij komunikat o ładowaniu
        loadingMsg->accept();
        loadingMsg->deleteLater();
        
        QMessageBox::information(this, "GUS", "Dane firmy zostały pobrane z GUS i uzupełnione w formularzu.");
    });
    
    // Połącz sygnał błędu
    connect(gusClient, &GusClient::errorOccurred, this, [this, loadingMsg](const QString &errorMsg) {
        // Zamknij komunikat o ładowaniu
        loadingMsg->accept();
        loadingMsg->deleteLater();
        
        QMessageBox::warning(this, "Błąd pobierania danych z GUS", 
            QString("Nie udało się pobrać danych z GUS:\n%1").arg(errorMsg));
    });
    
    qDebug() << "[GUS] Wywołuję fetchCompanyData z NIP:" << cleanNip;
    gusClient->fetchCompanyData(cleanNip);
}
