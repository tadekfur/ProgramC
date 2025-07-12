#include "client_full_dialog.h"
#include "db/dbmanager.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include "network/gusclient.h"

ClientFullDialog::ClientFullDialog(QWidget *parent, Mode mode)
    : QDialog(parent), dialogMode(mode) {
    setupUI();
    if (dialogMode == Add) {
        setWindowTitle("Dodawanie nowego klienta");
        loadNextClientNumber();
        clientNumberEdit->setReadOnly(true);
    } else {
        setWindowTitle("Edycja danych klienta");
    }
}

void ClientFullDialog::setupUI() {
    setWindowTitle("Dodaj nowego klienta");
    resize(1000, 700); // powiększ okno na starcie
    clientNumberEdit = new QLineEdit(this);
    clientNumberEdit->setReadOnly(true);
    nameEdit = new QLineEdit(this);
    shortNameEdit = new QLineEdit(this);
    contactPersonEdit = new QLineEdit(this);
    phoneEdit = new QLineEdit(this);
    emailEdit = new QLineEdit(this);
    streetEdit = new QLineEdit(this);
    postalCodeEdit = new QLineEdit(this);
    cityEdit = new QLineEdit(this);
    nipEdit = new QLineEdit(this);
    countryCombo = new QComboBox(this);
    countryCombo->addItems({"PL", "DE", "CZ", "SK", "UA", "LT", "GB", "FR", "IT", "ES", "NL", "BE", "DK", "SE", "NO", "US", "INNY"});
    errorLabel = new QLabel(this);
    errorLabel->setStyleSheet("color: red");
    errorLabel->setVisible(false);
    QFormLayout *form = new QFormLayout;
    form->addRow("Numer klienta:", clientNumberEdit);
    form->addRow("Nazwa:", nameEdit);
    form->addRow("Nazwa skrócona:", shortNameEdit);
    form->addRow("Osoba kontaktowa:", contactPersonEdit);
    form->addRow("Telefon:", phoneEdit);
    form->addRow("E-mail:", emailEdit);
    form->addRow("Ulica:", streetEdit);
    form->addRow("Kod pocztowy:", postalCodeEdit);
    form->addRow("Miasto:", cityEdit);
    // Dodanie wiersza dla NIP z przyciskiem GUS
    QHBoxLayout *nipLayout = new QHBoxLayout;
    nipLayout->addWidget(nipEdit);
    btnGus = new QPushButton("GUS");
    nipLayout->addWidget(btnGus);
    form->addRow("NIP:", nipLayout);
    // form->addRow("NIP:", nipEdit); // Usuń lub zakomentuj tę linię
    form->addRow("Kraj NIP:", countryCombo);
    // Adresy dostawy
    addressesTable = new QTableWidget(0, 8, this);
    addressesTable->setHorizontalHeaderLabels({"Nazwa", "Firma", "Ulica", "Kod pocztowy", "Miasto", "Osoba kontaktowa", "Telefon", "ID klienta"});
    for (int i = 0; i < 8; ++i) {
        addressesTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Interactive);
    }
    addressesTable->horizontalHeader()->setSectionsMovable(true);
    addressesTable->horizontalHeader()->setSectionsClickable(true);
    addressesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    addressesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    addressesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    addressesTable->resizeColumnsToContents();
    // Przywróć szerokości kolumn jeśli są zapisane
    QSettings settings("EtykietyManager", "ClientFullDialog");
    QByteArray state = settings.value("addressesTableState").toByteArray();
    if (!state.isEmpty())
        addressesTable->horizontalHeader()->restoreState(state);
    btnAddAddress = new QPushButton("Dodaj adres");
    btnRemoveAddress = new QPushButton("Usuń adres");
    btnEditAddress = new QPushButton("Edytuj adres");
    QHBoxLayout *addrBtnLayout = new QHBoxLayout;
    addrBtnLayout->addWidget(btnAddAddress);
    addrBtnLayout->addWidget(btnRemoveAddress);
    addrBtnLayout->addWidget(btnEditAddress);
    QVBoxLayout *addrLayout = new QVBoxLayout;
    addrLayout->addWidget(new QLabel("Adresy dostawy:"));
    addrLayout->addWidget(addressesTable);
    addrLayout->addLayout(addrBtnLayout);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(form);
    mainLayout->addWidget(errorLabel);
    mainLayout->addLayout(addrLayout);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
    connect(btnAddAddress, &QPushButton::clicked, this, &ClientFullDialog::addDeliveryAddressRow);
    connect(btnRemoveAddress, &QPushButton::clicked, this, &ClientFullDialog::removeSelectedAddressRow);
    connect(btnEditAddress, &QPushButton::clicked, this, &ClientFullDialog::editSelectedAddressRow);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ClientFullDialog::validateAndAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(countryCombo, &QComboBox::currentTextChanged, this, [this](const QString &code){
        if (code == "PL") {
            nipEdit->setPlaceholderText("000-000-00-00");
        } else {
            nipEdit->setPlaceholderText("np. DE123456789");
        }
    });
    // Ustaw szerokości pól: tylko pole Nazwa szerokie
    nameEdit->setMinimumWidth(350);
    shortNameEdit->setMinimumWidth(150);
    contactPersonEdit->setMinimumWidth(150);
    phoneEdit->setMinimumWidth(120);
    emailEdit->setMinimumWidth(200);
    streetEdit->setMinimumWidth(200);
    postalCodeEdit->setMinimumWidth(90);
    cityEdit->setMinimumWidth(150);
    nipEdit->setMinimumWidth(120);
    countryCombo->setMinimumWidth(80);
    connect(btnGus, &QPushButton::clicked, this, &ClientFullDialog::onGusClicked);
}

void ClientFullDialog::addDeliveryAddressRow() {
    DeliveryAddressDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        addAddressToTable(dlg.addressData());
    }
}

void ClientFullDialog::removeSelectedAddressRow() {
    int row = addressesTable->currentRow();
    if (row >= 0) addressesTable->removeRow(row);
}

void ClientFullDialog::setClientData(const QMap<QString, QVariant> &data) {
    clientNumberEdit->setText(data.value("client_number").toString());
    clientNumberEdit->setReadOnly(true); // Blokuj edycję także przy edycji istniejącego klienta
    nameEdit->setText(data.value("name").toString());
    shortNameEdit->setText(data.value("short_name").toString());
    contactPersonEdit->setText(data.value("contact_person").toString());
    phoneEdit->setText(data.value("phone").toString());
    emailEdit->setText(data.value("email").toString());
    streetEdit->setText(data.value("street").toString());
    postalCodeEdit->setText(data.value("postal_code").toString());
    cityEdit->setText(data.value("city").toString());
    nipEdit->setText(data.value("nip").toString());
    // Ustaw id klienta jeśli istnieje
    if (data.contains("id")) clientId = data.value("id").toInt();
}

QMap<QString, QVariant> ClientFullDialog::clientData() const {
    QMap<QString, QVariant> data;
    // Zawsze zapisuj numer klienta jako 6-cyfrowy string z zerami
    QString nr = clientNumberEdit->text();
    data["client_number"] = nr.rightJustified(6, '0');
    data["name"] = nameEdit->text();
    data["short_name"] = shortNameEdit->text();
    data["contact_person"] = contactPersonEdit->text();
    data["phone"] = phoneEdit->text();
    data["email"] = emailEdit->text();
    data["street"] = streetEdit->text();
    data["postal_code"] = postalCodeEdit->text();
    data["city"] = cityEdit->text();
    data["nip"] = ClientFullDialog::cleanNip(nipEdit->text());
    // Dodaj id jeśli jest znane (przechowywane w polu ukrytym lub jako pole klasy)
    if (clientId > 0) data["id"] = clientId;
    return data;
}

QList<QMap<QString, QVariant>> ClientFullDialog::deliveryAddresses() const {
    QList<QMap<QString, QVariant>> list;
    for (int row = 0; row < addressesTable->rowCount(); ++row) {
        QMap<QString, QVariant> addr;
        addr["name"] = addressesTable->item(row, 0) ? addressesTable->item(row, 0)->text() : "";
        addr["company"] = addressesTable->item(row, 1) ? addressesTable->item(row, 1)->text() : "";
        addr["street"] = addressesTable->item(row, 2) ? addressesTable->item(row, 2)->text() : "";
        addr["postal_code"] = addressesTable->item(row, 3) ? addressesTable->item(row, 3)->text() : "";
        addr["city"] = addressesTable->item(row, 4) ? addressesTable->item(row, 4)->text() : "";
        addr["contact_person"] = addressesTable->item(row, 5) ? addressesTable->item(row, 5)->text() : "";
        addr["phone"] = addressesTable->item(row, 6) ? addressesTable->item(row, 6)->text() : "";
        addr["client_id"] = addressesTable->item(row, 7) ? addressesTable->item(row, 7)->text() : "";
        list.append(addr);
    }
    return list;
}

void ClientFullDialog::loadNextClientNumber() {
    if (dialogMode == Edit) return; // Nie zmieniaj numeru w trybie edycji
    auto& db = DbManager::instance();
    int nextNr = db.getNextUniqueClientNumber();
    qDebug() << "[DEBUG] Wygenerowano nowy numer klienta:" << nextNr; // LOG
    clientNumberEdit->setText(QString("%1").arg(nextNr, 6, 10, QChar('0')));
    // NIE rezerwuj numeru tutaj!
}

void ClientFullDialog::generateClientNumber() {
    loadNextClientNumber();
}

bool isValidPolishNIP(const QString &nip) {
    // Sprawdzenie długości i cyfr
    QString digits = nip;
    digits.remove('-');
    if (digits.length() != 10 || !digits.contains(QRegularExpression("^\\d{10}$")))
        return false;
    // Weryfikacja sumy kontrolnej
    int w[9] = {6, 5, 7, 2, 3, 4, 5, 6, 7};
    int sum = 0;
    for (int i = 0; i < 9; ++i) sum += digits[i].digitValue() * w[i];
    int control = sum % 11;
    return control == digits[9].digitValue();
}

QString formatPolishNIP(const QString &nip) {
    QString digits = nip;
    digits.remove('-');
    if (digits.length() != 10) return nip;
    return digits.mid(0,3) + "-" + digits.mid(3,3) + "-" + digits.mid(6,2) + "-" + digits.mid(8,2);
}

bool isValidForeignNIP(const QString &nip, const QString &country) {
    QString n = nip.trimmed();
    if (country == "DE") // Niemcy: DE + 9 cyfr
        return n.startsWith("DE") && n.length() == 11 && n.mid(2).contains(QRegularExpression("^\\d{9}$"));
    if (country == "CZ") // Czechy: CZ + 8-10 cyfr
        return n.startsWith("CZ") && n.length() >= 10 && n.length() <= 12 && n.mid(2).contains(QRegularExpression("^\\d{8,10}$"));
    if (country == "SK") // Słowacja: SK + 10 cyfr
        return n.startsWith("SK") && n.length() == 12 && n.mid(2).contains(QRegularExpression("^\\d{10}$"));
    if (country == "UA") // Ukraina: UA + 8-10 cyfr/liter
        return n.startsWith("UA") && n.length() >= 10 && n.length() <= 12 && n.mid(2).contains(QRegularExpression("^[A-Za-z0-9]{8,10}$"));
    if (country == "LT") // Litwa: LT + 9 lub 12 cyfr
        return n.startsWith("LT") && (n.length() == 11 || n.length() == 14) && (n.mid(2).contains(QRegularExpression("^\\d{9}$")) || n.mid(2).contains(QRegularExpression("^\\d{12}$")));
    if (country == "GB") // Wielka Brytania: GB + 9 cyfr
        return n.startsWith("GB") && n.length() == 11 && n.mid(2).contains(QRegularExpression("^\\d{9}$"));
    if (country == "FR") // Francja: FR + 11 znaków (cyfry/litery)
        return n.startsWith("FR") && n.length() == 13 && n.mid(2).contains(QRegularExpression("^[A-Za-z0-9]{11}$"));
    if (country == "IT") // Włochy: IT + 11 cyfr
        return n.startsWith("IT") && n.length() == 13 && n.mid(2).contains(QRegularExpression("^\\d{11}$"));
    if (country == "ES") // Hiszpania: ES + 9 znaków
        return n.startsWith("ES") && n.length() == 11 && n.mid(2).contains(QRegularExpression("^[A-Za-z0-9]{9}$"));
    if (country == "NL") // Holandia: NL + 9 cyfr + B + 2 cyfry
        return n.startsWith("NL") && n.length() == 14 && n.mid(2,9).contains(QRegularExpression("^\\d{9}$")) && n.mid(11,1)=="B" && n.mid(12,2).contains(QRegularExpression("^\\d{2}$"));
    if (country == "BE") // Belgia: BE + 10 cyfr
        return n.startsWith("BE") && n.length() == 12 && n.mid(2).contains(QRegularExpression("^\\d{10}$"));
    if (country == "DK") // Dania: DK + 8 cyfr
        return n.startsWith("DK") && n.length() == 10 && n.mid(2).contains(QRegularExpression("^\\d{8}$"));
    if (country == "SE") // Szwecja: SE + 12 cyfr
        return n.startsWith("SE") && n.length() == 14 && n.mid(2).contains(QRegularExpression("^\\d{12}$"));
    if (country == "NO") // Norwegia: NO + 9 cyfr + MVA
        return n.startsWith("NO") && n.length() == 11 && n.mid(2,6).contains(QRegularExpression("^\\d{9}$"));
    if (country == "US") // USA: dowolny format, min 8 znaków
        return n.length() >= 8;
    // Domyślnie: min 8 znaków, litery/cyfry
    return n.length() >= 8 && n.length() <= 20 && n.contains(QRegularExpression("^[A-Za-z0-9]+$"));
}

bool ClientFullDialog::validate() {
    QString nip = nipEdit->text().trimmed();
    QString country = countryCombo->currentText();
    if (nameEdit->text().isEmpty() || nip.isEmpty()) {
        errorLabel->setText("Nazwa i NIP są wymagane.");
        errorLabel->setVisible(true);
        return false;
    }
    // Walidacja kodu pocztowego (PL: XX-XXX)
    QString postal = postalCodeEdit->text().trimmed();
    if (country == "PL" && !postal.contains(QRegularExpression("^\\d{2}-\\d{3}$"))) {
        errorLabel->setText("Nieprawidłowy kod pocztowy (format: XX-XXX)");
        errorLabel->setVisible(true);
        return false;
    }
    if (country == "PL") {
        if (!isValidPolishNIP(nip)) {
            errorLabel->setText("Nieprawidłowy polski NIP.");
            errorLabel->setVisible(true);
            return false;
        }
        nipEdit->setText(formatPolishNIP(nip));
    } else {
        if (!isValidForeignNIP(nip, country)) {
            errorLabel->setText("Nieprawidłowy NIP dla kraju " + country + ".");
            errorLabel->setVisible(true);
            return false;
        }
    }
    errorLabel->setVisible(false);
    return true;
}

// Utility: oczyszcza NIP z myślników, spacji i innych znaków, zostawia tylko cyfry
QString ClientFullDialog::cleanNip(const QString &nip) {
    QString result = nip;
    result.remove(QRegularExpression("[^0-9]"));
    return result;
}

void ClientFullDialog::validateAndAccept() {
    // Walidacja NIP: dopuszczalne formaty xxxxxxxxxx lub xxx-xxx-xx-xx
    QString nip = nipEdit->text().trimmed();
    QString cleanNip = ClientFullDialog::cleanNip(nip); // poprawne wywołanie statycznej metody
    if (!cleanNip.isEmpty() &&
        !QRegularExpression(R"(^\d{10}$)").match(cleanNip).hasMatch()) {
        QMessageBox::warning(this, "Błąd NIP", "Nieprawidłowy format NIP (dozwolone: xxxxxxxxxx lub xxx-xxx-xx-xx).");
        return;
    }

    if (!validate()) return;
    QMap<QString, QVariant> client = clientData();
    QList<QMap<QString, QVariant>> addresses = deliveryAddresses();
    auto& db = DbManager::instance();
    int clientNumber = client["client_number"].toString().rightJustified(6, '0').toInt();
    int clientId = client.contains("id") ? client["id"].toInt() : -1;
    // Sprawdzenie duplikatu NIP przy dodawaniu
    if (dialogMode == Add) {
        int existingId = db.findClientByNip(cleanNip);
        if (existingId != -1) {
            QString existingName = db.getClientNameById(existingId);
            QMessageBox::warning(this, "Duplikat klienta", QString("Klient o podanym NIP już istnieje w bazie: %1").arg(existingName));
            return;
        }
        int maxTries = 10;
        bool added = false;
        while (maxTries-- > 0) {
            qDebug() << "[DEBUG] Próba dodania klienta z numerem:" << client["client_number"].toString();
            if (db.findClientByNumber(client["client_number"].toString().rightJustified(6, '0')) != -1) {
                loadNextClientNumber();
                client["client_number"] = clientNumberEdit->text().rightJustified(6, '0');
                qDebug() << "[DEBUG] Numer zajęty, nowy numer:" << client["client_number"].toString();
            }
            // WYMUSZAMY ZAPIS OCZYSZCZONEGO NIP
            client["nip"] = cleanNip;
            if (db.addClientWithAddresses(client, addresses)) {
                db.markClientNumberUsed(client["client_number"].toString().rightJustified(6, '0').toInt());
                added = true;
                emit clientAdded();
                break;
            } else {
                QSqlError err = db.lastError();
                if (err.nativeErrorCode() == "23505") {
                    qDebug() << "[DEBUG] Błąd 23505 (duplikat numeru), generuję nowy numer.";
                    loadNextClientNumber();
                    client["client_number"] = clientNumberEdit->text().rightJustified(6, '0');
                    continue;
                } else {
                    qDebug() << "[DEBUG] Inny błąd SQL przy dodawaniu klienta:" << err.text();
                    break;
                }
            }
        }
        if (!added) {
            QMessageBox::critical(this, "Błąd zapisu", "Nie udało się wygenerować unikalnego numeru klienta. Spróbuj ponownie.");
            return;
        }
        accept();
    } else if (dialogMode == Edit) {
        // Przy edycji: nie traktuj własnego NIP jako duplikatu
        int existingId = db.findClientByNip(cleanNip); // użyj oczyszczonego NIP
        if (existingId != -1 && existingId != clientId) {
            QMessageBox::warning(this, "Duplikat klienta", "Inny klient o podanym NIP już istnieje w bazie.");
            return;
        }
        client["nip"] = cleanNip; // wymuś zapis oczyszczonego NIP
        if (db.updateClientWithAddresses(clientId, client, addresses)) {
            accept();
        } else {
            QMessageBox::critical(this, "Błąd zapisu", "Nie udało się zaktualizować klienta i adresów dostawy.");
        }
    }
}

ClientFullDialog::~ClientFullDialog() {
    // Zapisz szerokości kolumn tabeli adresów dostaw
    QSettings settings("EtykietyManager", "ClientFullDialog");
    settings.setValue("addressesTableState", addressesTable->horizontalHeader()->saveState());
}

void ClientFullDialog::setDeliveryAddresses(const QList<QMap<QString, QVariant>> &addresses) {
    addressesTable->setRowCount(0);
    for (const auto &addr : addresses) {
        addAddressToTable(addr);
    }
}

void ClientFullDialog::addAddressToTable(const QMap<QString, QVariant> &address) {
    int row = addressesTable->rowCount();
    addressesTable->insertRow(row);
    addressesTable->setItem(row, 0, new QTableWidgetItem(address.value("name").toString()));
    addressesTable->setItem(row, 1, new QTableWidgetItem(address.value("company").toString()));
    addressesTable->setItem(row, 2, new QTableWidgetItem(address.value("street").toString()));
    addressesTable->setItem(row, 3, new QTableWidgetItem(address.value("postal_code").toString()));
    addressesTable->setItem(row, 4, new QTableWidgetItem(address.value("city").toString()));
    addressesTable->setItem(row, 5, new QTableWidgetItem(address.value("contact_person").toString()));
    addressesTable->setItem(row, 6, new QTableWidgetItem(address.value("phone").toString()));
    addressesTable->setItem(row, 7, new QTableWidgetItem(address.value("client_id").toString()));
}

void ClientFullDialog::editSelectedAddressRow() {
    int row = addressesTable->currentRow();
    if (row < 0) return;
    QMap<QString, QVariant> address;
    address["name"] = addressesTable->item(row, 0) ? addressesTable->item(row, 0)->text() : "";
    address["company"] = addressesTable->item(row, 1) ? addressesTable->item(row, 1)->text() : "";
    address["street"] = addressesTable->item(row, 2) ? addressesTable->item(row, 2)->text() : "";
    address["postal_code"] = addressesTable->item(row, 3) ? addressesTable->item(row, 3)->text() : "";
    address["city"] = addressesTable->item(row, 4) ? addressesTable->item(row, 4)->text() : "";
    address["contact_person"] = addressesTable->item(row, 5) ? addressesTable->item(row, 5)->text() : "";
    address["phone"] = addressesTable->item(row, 6) ? addressesTable->item(row, 6)->text() : "";
    DeliveryAddressDialog dlg(this);
    dlg.setAddressData(address);
    if (dlg.exec() == QDialog::Accepted) {
        QMap<QString, QVariant> edited = dlg.addressData();
        addressesTable->setItem(row, 0, new QTableWidgetItem(edited.value("name").toString()));
        addressesTable->setItem(row, 1, new QTableWidgetItem(edited.value("company").toString()));
        addressesTable->setItem(row, 2, new QTableWidgetItem(edited.value("street").toString()));
        addressesTable->setItem(row, 3, new QTableWidgetItem(edited.value("postal_code").toString()));
        addressesTable->setItem(row, 4, new QTableWidgetItem(edited.value("city").toString()));
        addressesTable->setItem(row, 5, new QTableWidgetItem(edited.value("contact_person").toString()));
        addressesTable->setItem(row, 6, new QTableWidgetItem(edited.value("phone").toString()));
    }
}

void ClientFullDialog::addDefaultDeliveryAddress() {
    QMap<QString, QVariant> addr;
    addr["company"] = shortNameEdit->text();
    addr["street"] = streetEdit->text();
    addr["postal_code"] = postalCodeEdit->text();
    addr["city"] = cityEdit->text();
    addr["contact_person"] = contactPersonEdit->text();
    addr["phone"] = phoneEdit->text();
    addAddressToTable(addr);
}

void ClientFullDialog::setMode(Mode mode) {
    dialogMode = mode;
    if (dialogMode == Add) {
        setWindowTitle("Dodawanie nowego klienta");
        loadNextClientNumber();
        clientNumberEdit->setReadOnly(true);
    } else {
        setWindowTitle("Edycja danych klienta");
    }
}

void ClientFullDialog::onGusClicked() {
    if (!nipEdit || !btnGus) return;
    QString nip = nipEdit->text().trimmed();
    if (nip.isEmpty()) {
        QMessageBox::warning(this, "Brak NIP", "Wpisz NIP przed pobraniem danych z GUS.");
        return;
    }
    auto *gusClient = new GusClient(this);
    connect(gusClient, &GusClient::companyDataReceived, this, [this, gusClient](const QMap<QString, QString> &data) {
        if (!nameEdit || !shortNameEdit || !streetEdit || !postalCodeEdit || !cityEdit) return;
        // Mapowanie pól z GUS na formularz
        nameEdit->setText(data.value("company_name", ""));
        shortNameEdit->setText(data.value("company_name", "")); // lub inny klucz jeśli dostępny
        streetEdit->setText(data.value("street", ""));
        postalCodeEdit->setText(data.value("postal_code", ""));
        cityEdit->setText(data.value("city", ""));
        nipEdit->setText(data.value("nip", ""));
        // Możesz dodać więcej pól jeśli chcesz
        gusClient->deleteLater();
    });
    connect(gusClient, &GusClient::errorOccurred, this, [this, gusClient](const QString &errorMsg) {
        QMessageBox::warning(this, "Błąd GUS", errorMsg);
        gusClient->deleteLater();
    });
    gusClient->fetchCompanyData(nip);
}
