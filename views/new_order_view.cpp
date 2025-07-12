#include "new_order_view.h"
#include "models/client.h"
#include "views/clients_db_view.h"
#include "views/delivery_addresses_view.h"
#include "views/client_select_dialog.h"
#include "network/gusclient.h"
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
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QXmlStreamReader>
#include <QFile>
#include <QMessageBox>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

NewOrderView::NewOrderView(QWidget *parent) : QWidget(parent) {
    setupUi();
}

// --- Pola do trybu edycji/duplikacji ---
bool editMode = false;
int currentOrderId = -1;

void NewOrderView::setupUi() {
    // Styl jak w SettingsDialog: tło, border-radius, bez własnych kolorów czcionek, domyślne Qt
    this->setStyleSheet("QWidget#NewOrderView, QWidget[objectName=\"NewOrderView\"] { background: #f6f8fa; border-radius: 12px; }");
    this->setObjectName("NewOrderView");
    setMinimumWidth(1200);
    setMaximumWidth(1800);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    // Tytuł
    titleLabel = new QLabel("WPROWADŹ DANE ZAMÓWIENIA");
    QFont titleFont;
    titleFont.setPointSize(11);
    titleFont.setBold(false);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignLeft);
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
    clientGroup = new QGroupBox("Dane zamawiającego");
    clientGroup->setStyleSheet("QGroupBox { background: #f6f8fa; border-radius: 0px; border: 1.5px solid #bcd; margin-top: 8px; margin-bottom: 8px; padding: 12px; } QGroupBox::title { font-weight: normal; color: #222; }");
    QGridLayout *clientGrid = new QGridLayout(clientGroup);
    clientGrid->setAlignment(Qt::AlignLeft);
    clientGrid->setHorizontalSpacing(6);
    clientGrid->setVerticalSpacing(8);
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
    clientGrid->addWidget(btnSelectClient, 0, 4);
    btnAddClient = new QPushButton("zapisz");
    clientGrid->addWidget(btnAddClient, 0, 5);
    connect(btnAddClient, &QPushButton::clicked, this, &NewOrderView::handleAddNewClient);
    QStringList clientLabels = {"Firma", "Nazwa skróc.", "Osoba kontaktowa", "Nr telefonu", "E-mail", "Ulica i nr", "Kod pocztowy", "Miasto", "NIP"};
    QVector<int> clientFieldWidths = {260, 180, 180, 140, 260, 200, 90, 160, 120};
    for (int i = 0; i < clientLabels.size(); ++i) {
        QLabel *lbl = new QLabel(clientLabels[i] + ":");
        lbl->setFixedWidth(100);
        lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        clientGrid->addWidget(lbl, i+1, 0, Qt::AlignLeft);
        QLineEdit *edit = new QLineEdit;
        edit->setMinimumWidth(clientFieldWidths[i]);
        edit->setMaximumWidth(clientFieldWidths[i] + 40);
        if (clientLabels[i] == "NIP") {
            QHBoxLayout *nipLayout = new QHBoxLayout;
            nipLayout->setSpacing(4);
            nipLayout->addWidget(edit);
            QPushButton *btnGUS = new QPushButton;
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
            clientGrid->addWidget(edit, i+1, 1, 1, 5, Qt::AlignLeft);
        }
        clientFields.append(edit);
    }
    clientsAndAddressLayout->addWidget(clientGroup, 2);
    // Adres dostawy
    deliveryAddrGroup = new QGroupBox("Adres dostawy");
    deliveryAddrGroup->setStyleSheet("QGroupBox { background: #f6f8fa; border-radius: 0px; border: 1.5px solid #bcd; margin-top: 8px; margin-bottom: 8px; padding: 12px; } QGroupBox::title { font-weight: normal; color: #222; }");
    QGridLayout *addrGrid = new QGridLayout(deliveryAddrGroup);
    addrGrid->setAlignment(Qt::AlignLeft);
    addrGrid->setHorizontalSpacing(6);
    addrGrid->setVerticalSpacing(8);
    btnAddrFromDb = new QPushButton("Wstaw z bazy");
    QPushButton *btnSaveDeliveryAddr = new QPushButton("Zapisz adres");
    addrGrid->addWidget(btnAddrFromDb, 0, 0, 1, 1, Qt::AlignLeft);
    addrGrid->addWidget(btnSaveDeliveryAddr, 0, 1, 1, 1, Qt::AlignLeft);
    QStringList addrLabels = {"Nazwa firmy:", "Ulica i nr:", "Kod pocztowy:", "Miejscowość:", "Osoba kont.:", "Tel.:", };
    QVector<int> addrWidths = {250, 200, 80, 150, 180, 120};
    for (int i = 0; i < addrLabels.size(); ++i) {
        QLabel *lbl = new QLabel(addrLabels[i]);
        lbl->setFixedWidth(100);
        lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        addrGrid->addWidget(lbl, i+1, 0, Qt::AlignLeft);
        QLineEdit *edit = new QLineEdit;
        edit->setFixedWidth(addrWidths[i]);
        edit->setAlignment(Qt::AlignLeft);
        addrGrid->addWidget(edit, i+1, 1, Qt::AlignLeft);
        addrFields.append(edit);
    }
    // Domyślnie kopiuj dane klienta do adresu dostawy
    auto fillDeliveryFromClient = [this]() {
        addrFields[0]->setText(clientFields[1]->text()); // Nazwa firmy: skrócona nazwa
        addrFields[1]->setText(clientFields[5]->text()); // Ulica i nr
        addrFields[2]->setText(clientFields[6]->text()); // Kod pocztowy
        addrFields[3]->setText(clientFields[7]->text()); // Miasto
        addrFields[4]->setText(clientFields[2]->text()); // Osoba kontaktowa
        addrFields[5]->setText(clientFields[3]->text()); // Tel.
    };
    fillDeliveryFromClient();
    // Aktualizuj adres dostawy przy zmianie danych klienta
    for (int i : {0, 2, 3, 5, 6, 7}) {
        connect(clientFields[i], &QLineEdit::textChanged, this, fillDeliveryFromClient);
    }
    clientsAndAddressLayout->addWidget(deliveryAddrGroup, 2);
    formLayout->addLayout(clientsAndAddressLayout);
    // --- Sekcja: Pozycje zamówienia ---
    prodContainer = new QGroupBox("Dane produkcji");
    prodContainer->setStyleSheet("QGroupBox { background: #f6f8fa; border-radius: 0px; border: 1.5px solid #bcd; margin-top: 8px; margin-bottom: 8px; padding: 12px; } QGroupBox::title { font-weight: normal; color: #222; }");
    prodMainLayout = new QVBoxLayout(prodContainer);
    prodGrid = new QGridLayout;
    prodMainLayout->addLayout(prodGrid);
    addProductBlock(1);
    addProductBlock(2);
    btnAddPosition = new QPushButton("Dodaj pozycję");
    btnAddPosition->setMinimumWidth(120);
    btnAddPosition->setMaximumWidth(220);
    btnAddPosition->setMinimumHeight(36);
    btnAddPosition->setMaximumHeight(36);
    btnAddPosition->setStyleSheet("QPushButton:hover { background: #e0edff; }");
    prodMainLayout->addWidget(btnAddPosition, 0, Qt::AlignLeft);
    formLayout->addWidget(prodContainer);
    // --- Sekcja: Uwagi ---
    QGroupBox *notesGroup = new QGroupBox("Uwagi");
    notesGroup->setStyleSheet("QGroupBox { background: #f6f8fa; border-radius: 0px; border: 1.5px solid #bcd; margin-top: 8px; margin-bottom: 8px; padding: 12px; } QGroupBox::title { font-weight: normal; color: #222; }");
    QVBoxLayout *notesLayout = new QVBoxLayout(notesGroup);
    notesEdit = new QTextEdit;
    notesEdit->setPlaceholderText("Wpisz dodatkowe uwagi do zamówienia...");
    notesLayout->addWidget(notesEdit);
    formLayout->addWidget(notesGroup);
    // --- Przycisk zapisu ---
    btnSave = new QPushButton("💾 Zapisz zamówienie");
    btnSave->setMinimumWidth(120);
    btnSave->setMaximumWidth(220);
    btnSave->setMinimumHeight(36);
    btnSave->setMaximumHeight(36);
    btnSave->setStyleSheet("QPushButton:hover { background: #e0edff; } QPushButton:focus { background: #e0edff; }");
    formLayout->addWidget(btnSave, 0, Qt::AlignCenter);
    formArea->setWidget(formWidget);
    mainLayout->addWidget(formArea);
    // Połączenia sygnałów
    connect(btnSelectClient, &QPushButton::clicked, this, &NewOrderView::handleSelectClientFromDb);
    connect(btnAddrFromDb, &QPushButton::clicked, this, &NewOrderView::handleSelectDeliveryAddress);
    connect(btnSaveDeliveryAddr, &QPushButton::clicked, this, [this]() {
        // Dialog z polem 'nazwa' do zapisu adresu
        QDialog dlg(this);
        dlg.setWindowTitle("Zapisz nowy adres dostawy");
        QVBoxLayout *layout = new QVBoxLayout(&dlg);
        QLineEdit *nameEdit = new QLineEdit;
        nameEdit->setPlaceholderText("Nazwa adresu (np. Magazyn, Biuro)");
        layout->addWidget(new QLabel("Podaj nazwę adresu:"));
        layout->addWidget(nameEdit);
        QPushButton *okBtn = new QPushButton("Zapisz");
        QPushButton *cancelBtn = new QPushButton("Anuluj");
        QHBoxLayout *btnLayout = new QHBoxLayout;
        btnLayout->addWidget(okBtn);
        btnLayout->addWidget(cancelBtn);
        layout->addLayout(btnLayout);
        QObject::connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
        QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
        if (dlg.exec() == QDialog::Accepted) {
            QString nazwa = nameEdit->text().trimmed();
            if (nazwa.isEmpty()) {
                QMessageBox::warning(this, "Błąd", "Nazwa adresu nie może być pusta.");
                return;
            }
            // Pobierz numer klienta
            QString clientNumber = clientNumberEdit->text().trimmed();
            if (clientNumber.isEmpty()) {
                QMessageBox::warning(this, "Brak klienta", "Najpierw wybierz klienta, aby zapisać adres.");
                return;
            }
            int clientId = -1;
            QSqlQuery q(DbManager::instance().database());
            q.prepare("SELECT id FROM clients WHERE client_number = ?");
            q.addBindValue(clientNumber);
            if (q.exec() && q.next()) {
                clientId = q.value(0).toInt();
            }
            if (clientId <= 0) {
                QMessageBox::warning(this, "Brak klienta", "Nie znaleziono klienta o podanym numerze. Wybierz klienta z bazy.");
                return;
            }
            // Zapisz adres do bazy
            QSqlQuery q2(DbManager::instance().database());
            q2.prepare("INSERT INTO delivery_addresses (client_id, name, company, street, postal_code, city, contact_person, phone) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
            q2.addBindValue(clientId);
            q2.addBindValue(nazwa);
            for (int i = 0; i < 6; ++i) q2.addBindValue(addrFields[i]->text());
            if (!q2.exec()) {
                QMessageBox::warning(this, "Błąd zapisu", "Nie udało się zapisać adresu: " + q2.lastError().text());
            } else {
                QMessageBox::information(this, "Sukces", "Adres dostawy został zapisany.");
            }
        }
    });
    connect(btnAddPosition, &QPushButton::clicked, this, [this](){ addProductBlock(); });
    connect(btnSave, &QPushButton::clicked, this, &NewOrderView::saveOrder);
}

void NewOrderView::addProductBlock(int number) {
    int idx = prodBlocks.size();
    QGroupBox *prodBlock = new QGroupBox(QString("Pozycja %1").arg(idx+1));
    prodBlock->setStyleSheet("QGroupBox { background: #f6f8fa; border-radius: 0px; border: 1.5px solid #bcd; margin: 2px; padding: 10px; } QGroupBox::title { font-weight: normal; color: #222; }");
    prodBlock->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QGridLayout *prodLayout = new QGridLayout(prodBlock);
    prodLayout->setContentsMargins(8, 8, 8, 8);
    prodLayout->setHorizontalSpacing(18);
    prodLayout->setVerticalSpacing(12);
    // --- Pasek tytułu i przycisk Usuń ---
    QHBoxLayout *titleLayout = new QHBoxLayout;
    // Usunięto powielony label Pozycja X
    QPushButton *btnRemove = new QPushButton("Usuń");
    btnRemove->setMinimumWidth(90);
    btnRemove->setMaximumWidth(90);
    btnRemove->setMinimumHeight(28);
    btnRemove->setMaximumHeight(28);
    btnRemove->setStyleSheet("QPushButton:hover { background: #e0edff; } QPushButton:focus { background: #e0edff; }");
    connect(btnRemove, &QPushButton::clicked, this, [this, prodBlock](){ removeProductBlock(prodBlock); });
    titleLayout->addStretch(1);
    titleLayout->addWidget(btnRemove);
    prodLayout->addLayout(titleLayout, 0, 0, 1, 4);
    // --- Cena ---
    QLabel *cenaLabel = new QLabel("Cena:");
    QLineEdit *cenaInput = new QLineEdit;
    cenaInput->setPlaceholderText("Cena");
    cenaInput->setMaximumWidth(120);
    cenaInput->setMinimumWidth(80);
    QComboBox *cenaTypCombo = new QComboBox;
    cenaTypCombo->addItems({"za 1 tyś", "za 1 rolkę"});
    cenaTypCombo->setFixedWidth(80);
    QHBoxLayout *cenaRow = new QHBoxLayout;
    cenaRow->setSpacing(18);
    cenaRow->addWidget(cenaLabel);
    cenaRow->addWidget(cenaInput);
    cenaRow->addWidget(cenaTypCombo);
    cenaRow->addStretch(1);
    prodLayout->addLayout(cenaRow, 1, 0, 1, 4);
    // --- Szerokość/Wysokość ---
    QLineEdit *szer = new QLineEdit; szer->setPlaceholderText("Szerokość [mm]"); szer->setMaximumWidth(120); szer->setMinimumWidth(80);
    QLineEdit *wys = new QLineEdit; wys->setPlaceholderText("Wysokość [mm]"); wys->setMaximumWidth(120); wys->setMinimumWidth(80);
    QHBoxLayout *sizeRow = new QHBoxLayout;
    sizeRow->setSpacing(18);
    sizeRow->addWidget(new QLabel("Szerokość:"));
    sizeRow->addWidget(szer);
    sizeRow->addSpacing(18);
    sizeRow->addWidget(new QLabel("Wysokość:"));
    sizeRow->addWidget(wys);
    sizeRow->addStretch(1);
    prodLayout->addLayout(sizeRow, 2, 0, 1, 4);
    // --- Materiał ---
    QComboBox *mat = new QComboBox;
    QStringList materialOptions = {"Termiczny", "Termotransferowy", "Termiczny TOP", "Termiczny RF20", "Termotransferowy RF20", "Folia PP", "Folia PP RF20", "PET Matt Silver", "Inny (dopisz ręcznie)"};
    mat->addItems(materialOptions);
    mat->setMinimumWidth(120);
    mat->setMaximumWidth(140);
    mat->setEditable(false);
    connect(mat, QOverload<int>::of(&QComboBox::currentIndexChanged), [mat](int idx){
        if (mat->itemText(idx) == "Inny (dopisz ręcznie)") {
            mat->setEditable(true);
            mat->setCurrentText("");
            if (mat->lineEdit()) mat->lineEdit()->setPlaceholderText("Wpisz własny rodzaj materiału");
        } else {
            mat->setEditable(false);
            mat->setCurrentText(mat->itemText(idx));
        }
    });
    QHBoxLayout *rowMat = new QHBoxLayout;
    rowMat->setSpacing(18);
    rowMat->addWidget(new QLabel("Rodzaj materiału:"));
    rowMat->addWidget(mat);
    rowMat->addStretch(1);
    prodLayout->addLayout(rowMat, 3, 0, 1, 4);
    // --- Ilość ---
    QLineEdit *ilosc = new QLineEdit; ilosc->setPlaceholderText("zam. ilość"); ilosc->setFixedWidth(100);
    QComboBox *typIlosci = new QComboBox; typIlosci->addItems({"tyś.", "rolek"}); typIlosci->setFixedWidth(60); typIlosci->setCurrentText("rolek");
    QHBoxLayout *rowIlosc = new QHBoxLayout;
    rowIlosc->setSpacing(18);
    rowIlosc->addWidget(new QLabel("zam. ilość:"));
    rowIlosc->addWidget(ilosc);
    rowIlosc->addSpacing(8);
    rowIlosc->addWidget(typIlosci);
    rowIlosc->addStretch(1);
    prodLayout->addLayout(rowIlosc, 4, 0, 1, 4);
    // --- Nawój/długość, Rdzeń, Rdzeń inny ---
    QLineEdit *nawoj = new QLineEdit; nawoj->setPlaceholderText("nawój/długość"); nawoj->setFixedWidth(100);
    QComboBox *rdzen = new QComboBox; rdzen->addItems({"25", "40", "76", "inny"}); rdzen->setFixedWidth(60); rdzen->setEditable(false);
    QLineEdit *rdzenInny = new QLineEdit; rdzenInny->setPlaceholderText("Wpisz rdzeń"); rdzenInny->setVisible(false); rdzenInny->setFixedWidth(60);
    connect(rdzen, QOverload<int>::of(&QComboBox::currentIndexChanged), [rdzen, rdzenInny](int idx){
        if (rdzen->itemText(idx) == "inny") {
            rdzenInny->setVisible(true);
            rdzenInny->setFocus();
        } else {
            rdzenInny->setVisible(false);
        }
    });
    QHBoxLayout *rowNawoj = new QHBoxLayout;
    rowNawoj->setSpacing(18);
    rowNawoj->addWidget(new QLabel("nawój/długość:"));
    rowNawoj->addWidget(nawoj);
    rowNawoj->addSpacing(8);
    rowNawoj->addWidget(new QLabel("Rdzeń:"));
    rowNawoj->addWidget(rdzen);
    rowNawoj->addWidget(rdzenInny);
    rowNawoj->addStretch(1);
    prodLayout->addLayout(rowNawoj, 5, 0, 1, 4);
    // --- Zam. rolki ---
    QLineEdit *zamRolki = new QLineEdit; zamRolki->setReadOnly(true); zamRolki->setFixedWidth(100);
    QHBoxLayout *rowZamRolki = new QHBoxLayout;
    rowZamRolki->setSpacing(18);
    rowZamRolki->addWidget(new QLabel("zam. rolki:"));
    rowZamRolki->addWidget(zamRolki);
    rowZamRolki->addStretch(1);
    prodLayout->addLayout(rowZamRolki, 6, 0, 1, 4);
    // --- Formuła zam. rolki ---
    auto updateZamRolki = [ilosc, nawoj, typIlosci, zamRolki]() {
        bool ok1, ok2;
        double valIlosc = ilosc->text().replace(",", ".").toDouble(&ok1);
        double valNawoj = nawoj->text().replace(",", ".").toDouble(&ok2);
        QString typ = typIlosci->currentText();
        if (typ == "tyś.") {
            if (ok1 && ok2 && valNawoj > 0 && valIlosc > 0) {
                int wynik = std::ceil((valIlosc * 1000.0) / valNawoj);
                zamRolki->setText(QString::number(wynik));
            } else {
                zamRolki->setText("0");
            }
        } else {
            zamRolki->setText(ilosc->text());
        }
    };
    connect(ilosc, &QLineEdit::textChanged, this, updateZamRolki);
    connect(nawoj, &QLineEdit::textChanged, this, updateZamRolki);
    connect(typIlosci, &QComboBox::currentTextChanged, this, updateZamRolki);
    // --- Zbierz pola do mapy ---
    QMap<QString, QWidget*> prodFields;
    prodFields["block_widget"] = prodBlock;
    prodFields["Cena"] = cenaInput;
    prodFields["CenaTyp"] = cenaTypCombo;
    prodFields["Szerokość"] = szer;
    prodFields["Wysokość"] = wys;
    prodFields["Rodzaj materiału"] = mat;
    prodFields["zam. ilość"] = ilosc;
    prodFields["Typ ilości"] = typIlosci;
    prodFields["nawój/długość"] = nawoj;
    prodFields["Rdzeń"] = rdzen;
    prodFields["Rdzeń_inny"] = rdzenInny;
    prodFields["zam. rolki"] = zamRolki;
    prodFieldsList.append(prodFields);
    prodBlocks.append(prodBlock);
    int row = idx / 2;
    int col = idx % 2;
    prodGrid->addWidget(prodBlock, row, col);
}

void NewOrderView::removeProductBlock(QWidget *blockWidget) {
    int idxToRemove = -1;
    for (int i = 0; i < prodFieldsList.size(); ++i) {
        if (prodFieldsList[i]["block_widget"] == blockWidget) {
            idxToRemove = i;
            break;
        }
    }
    if (idxToRemove >= 0) {
        QWidget *block = prodBlocks.takeAt(idxToRemove);
        prodFieldsList.removeAt(idxToRemove);
        block->setParent(nullptr);
        block->deleteLater();
        relayoutProductBlocks();
    }
}

void NewOrderView::relayoutProductBlocks() {
    // Usuwa wszystkie widgety z siatki i dodaje ponownie
    QLayoutItem *item;
    while ((item = prodGrid->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->setParent(nullptr);
        delete item;
    }
    for (int idx = 0; idx < prodBlocks.size(); ++idx) {
        int row = idx / 2;
        int col = idx % 2;
        prodGrid->addWidget(prodBlocks[idx], row, col);
    }
}

void NewOrderView::saveOrder() {
    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) {
        qDebug() << "[saveOrder][ERROR] Baza danych nie jest otwarta!";
        QMessageBox::critical(this, "Błąd bazy danych", "Brak połączenia z bazą danych. Nie można zapisać zamówienia.");
        return;
    }
    QString errors;
    if (!validateOrderForm(errors)) {
        QMessageBox::warning(this, "Błąd danych", errors);
        return;
    }
    db.transaction();
    QString orderNumber;
    int orderId = -1;
    // --- Pobierz client_id na podstawie client_number ---
    int clientId = -1;
    QString clientNumber = clientNumberEdit->text().trimmed();
    QSqlQuery qClient(db);
    qClient.prepare("SELECT id FROM clients WHERE client_number = ?");
    qClient.addBindValue(clientNumber);
    if (qClient.exec() && qClient.next()) {
        clientId = qClient.value(0).toInt();
    } else {
        db.rollback();
        QMessageBox::critical(this, "Błąd bazy", "Nie znaleziono klienta o podanym numerze. Wybierz klienta z bazy.");
        return;
    }
    QDate orderDate = orderDateEdit->date();
    QDate deliveryDate = deliveryDateEdit->date();
    QString notes = notesEdit ? notesEdit->toPlainText() : "";
    QString paymentTerm = paymentTermCombo->currentText();
    bool success = false;
    if (editMode && currentOrderId > 0) {
        // --- UPDATE istniejącego zamówienia ---
        orderNumber = nrEdit->text();
        QSqlQuery q(db);
        q.prepare("UPDATE orders SET order_date=?, delivery_date=?, client_id=?, notes=?, payment_term=?, delivery_company=?, delivery_street=?, delivery_postal_code=?, delivery_city=?, delivery_contact_person=?, delivery_phone=? WHERE id=?");
        q.addBindValue(orderDate);
        q.addBindValue(deliveryDate);
        q.addBindValue(clientId);
        q.addBindValue(notes);
        q.addBindValue(paymentTerm);
        for (int i = 0; i < 6; ++i) q.addBindValue(addrFields[i]->text());
        q.addBindValue(currentOrderId);
        if (!q.exec()) {
            db.rollback();
            qDebug() << "[ERROR] UPDATE orders failed:" << q.lastError().text();
            QMessageBox::critical(this, "Błąd bazy", "Nie udało się zaktualizować zamówienia: " + q.lastError().text());
            return;
        }
        orderId = currentOrderId;
        // Usuń stare pozycje zamówienia
        QSqlQuery qDel(db);
        qDel.prepare("DELETE FROM order_items WHERE order_id = ?");
        qDel.addBindValue(orderId);
        if (!qDel.exec()) {
            db.rollback();
            qDebug() << "[ERROR] DELETE order_items failed:" << qDel.lastError().text();
            QMessageBox::critical(this, "Błąd bazy", "Nie udało się usunąć pozycji zamówienia: " + qDel.lastError().text());
            return;
        }
        success = true;
    } else {
        // --- INSERT nowe/duplikowane zamówienie ---
        orderNumber = generateOrderNumber(db);
        qDebug() << "[DEBUG] Wygenerowano numer zamówienia:" << orderNumber;
        if (orderNumber.isEmpty()) {
            db.rollback();
            QMessageBox::critical(this, "Błąd bazy", "Nie udało się wygenerować numeru zamówienia.");
            return;
        }
        QSqlQuery q(db);
        q.prepare("INSERT INTO orders (order_number, order_date, delivery_date, client_id, notes, payment_term, delivery_company, delivery_street, delivery_postal_code, delivery_city, delivery_contact_person, delivery_phone, status) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        q.addBindValue(orderNumber);
        q.addBindValue(orderDate);
        q.addBindValue(deliveryDate);
        q.addBindValue(clientId);
        q.addBindValue(notes);
        q.addBindValue(paymentTerm);
        for (int i = 0; i < 6; ++i) q.addBindValue(addrFields[i]->text());
        q.addBindValue(0); // Status: 0 = Order::Przyjete (Przyjęte do realizacji)
        if (!q.exec()) {
            qDebug() << "[ERROR] Nie udało się dodać zamówienia:" << q.lastError().text() << q.lastQuery();
            db.rollback();
            QMessageBox::critical(this, "Błąd bazy", "Nie udało się dodać zamówienia: " + q.lastError().text());
            return;
        }
        orderId = q.lastInsertId().toInt();
        if (orderId <= 0) {
            db.rollback();
            qDebug() << "[ERROR] lastInsertId nieprawidłowy!";
            QMessageBox::critical(this, "Błąd bazy", "Nie udało się uzyskać ID nowego zamówienia.");
            return;
        }
        success = true;
    }
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
        if (!q2.exec()) {
            db.rollback();
            qDebug() << "[ERROR] INSERT order_items failed:" << q2.lastError().text();
            QMessageBox::critical(this, "Błąd bazy", "Nie udało się dodać pozycji zamówienia: " + q2.lastError().text());
            return;
        }
    }
    if (success) {
        if (!db.commit()) {
            qDebug() << "[ERROR] Commit transakcji nie powiódł się:" << db.lastError().text();
            QMessageBox::critical(this, "Błąd bazy", "Nie udało się zatwierdzić transakcji: " + db.lastError().text());
            return;
        }
        qDebug() << "[INFO] Zamówienie zapisane. Numer:" << orderNumber << ", ID:" << orderId;
        QMessageBox::information(this, "Sukces", (editMode ? "Zamówienie zostało zaktualizowane." : "Zamówienie zostało zapisane.\nNumer: " + orderNumber));
        emit orderSaved();
        resetForm(); // Po zapisaniu zamówienia wyczyść formularz
    } else {
        db.rollback();
        qDebug() << "[ERROR] Niepowodzenie zapisu zamówienia!";
        QMessageBox::critical(this, "Błąd bazy", "Nie udało się zapisać zamówienia.");
    }
    // Możesz dodać reset formularza lub inne akcje
}

// --- Pomocnicza funkcja: czy data to polskie święto? ---
bool NewOrderView::isPolishHoliday(const QDate &date) const {
    // Najważniejsze święta stałe
    static const QList<QPair<int, int>> fixedHolidays = {
        {1, 1},   // Nowy Rok
        {1, 6},   // Trzech Króli
        {5, 1},   // Święto Pracy
        {5, 3},   // Konstytucja 3 Maja
        {8, 15},  // Wniebowzięcie NMP
        {11, 1},  // Wszystkich Świętych
        {11, 11}, // Święto Niepodległości
        {12, 25}, // Boże Narodzenie
        {12, 26}  // Drugi dzień BN
    };
    for (const auto &h : fixedHolidays) {
        if (date.month() == h.first && date.day() == h.second)
            return true;
    }
    // Wielkanoc i Boże Ciało (ruchome) - uproszczone, bez dokładnych obliczeń
    // Możesz dodać dokładne wyliczanie, jeśli potrzebujesz
    return false;
}

bool NewOrderView::validateOrderForm(QString &errors) {
    errors.clear();
    // Walidacja klienta
    if (clientFields[0]->text().trimmed().isEmpty())
        errors += "- Podaj nazwę firmy (zamawiający).\n";
    if (clientFields[3]->text().trimmed().isEmpty())
        errors += "- Podaj numer telefonu klienta.\n";
    // Walidacja NIP (tylko 10 cyfr, bez myślników)
    QString nip = clientFields[8]->text().trimmed();
    QString cleanNip = nip;
    cleanNip.remove(QRegularExpression("[^0-9]"));
    if (!cleanNip.isEmpty() && !QRegularExpression(R"(^\d{10}$)").match(cleanNip).hasMatch())
        errors += "- Nieprawidłowy format NIP (wymagane 10 cyfr).\n";
    // --- USUNIĘTO WALIDACJĘ UNIKALNOŚCI NIP ---
    // Walidacja e-mail
    if (!clientFields[4]->text().isEmpty() && !QRegularExpression(R"(^[\w\.-]+@[\w\.-]+\.[a-zA-Z]{2,}$)").match(clientFields[4]->text()).hasMatch())
        errors += "- Nieprawidłowy e-mail klienta.\n";
    // Walidacja kodu pocztowego
    QString postal = clientFields[6]->text().trimmed();
    if (!postal.isEmpty() && !QRegularExpression(R"(^\d{2}-\d{3}$)").match(postal).hasMatch())
        errors += "- Nieprawidłowy kod pocztowy (format XX-XXX).\n";
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
    // --- OSTRZEŻENIE: weekend lub święto ---
    QDate delivery = deliveryDateEdit->date();
    if (delivery.dayOfWeek() == 6 || delivery.dayOfWeek() == 7)
        errors += "- Wybrany dzień wysyłki to weekend!\n";
    if (isPolishHoliday(delivery))
        errors += "- Wybrany dzień wysyłki to święto ustawowe!\n";
    return errors.isEmpty();
}

QString NewOrderView::generateOrderNumber(QSqlDatabase db) {
    QSqlQuery q(db);
    // Pobierz ostatni numer
    if (!q.exec("SELECT last_number FROM order_sequence WHERE id=1 FOR UPDATE")) {
        qDebug() << "[ERROR] Nie można pobrać sekwencji zamówień:" << q.lastError().text();
        return "";
    }
    int lastNumber = 0;
    if (q.next())
        lastNumber = q.value(0).toInt();
    int nextNumber = (lastNumber < 750) ? 751 : lastNumber + 1;
    // Format numeru zamówienia: ZAM-2025-0xx
    QString year = QString::number(QDate::currentDate().year());
    QString num = QString("ZAM-%1-%2").arg(year).arg(nextNumber, 3, 10, QChar('0'));
    // Zaktualizuj sekwencję tylko jeśli numer zostanie użyty
    QSqlQuery q2(db);
    q2.prepare("UPDATE order_sequence SET last_number=? WHERE id=1");
    q2.addBindValue(nextNumber);
    if (!q2.exec()) {
        qDebug() << "[ERROR] Nie można zaktualizować sekwencji zamówień:" << q2.lastError().text();
    }
    return num;
}

void NewOrderView::handleSelectClientFromDb() {
    qDebug() << "[DEBUG] Kliknięto 'Wstaw z bazy' w NewOrderView (ClientSelectDialog test)";
    // Otwórz dialog wyboru klienta
    ClientSelectDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QMap<QString, QVariant> clientData = dlg.selectedClient();
        qDebug() << "[DEBUG] Wybrano klienta z ClientSelectDialog:" << clientData;
        clientNumberEdit->setText(clientData.value("client_number").toString());
        clientFields[0]->setText(clientData.value("name").toString());
        clientFields[1]->setText(clientData.value("short_name").toString());
        clientFields[2]->setText(clientData.value("contact_person").toString());
        clientFields[3]->setText(clientData.value("phone").toString());
        clientFields[4]->setText(clientData.value("email").toString());
        clientFields[5]->setText(clientData.value("street").toString());
        clientFields[6]->setText(clientData.value("postal_code").toString());
        clientFields[7]->setText(clientData.value("city").toString());
        // Ustaw NIP jako 10 cyfr bez myślników
        QString nip = clientData.value("nip").toString();
        nip.remove(QRegularExpression("[^0-9]"));
        clientFields[8]->setText(nip);
    }
}

void NewOrderView::handleSelectDeliveryAddress() {
    // Pobierz numer klienta z pola
    QString clientNumber = clientNumberEdit->text().trimmed();
    if (clientNumber.isEmpty()) {
        QMessageBox::warning(this, "Brak klienta", "Najpierw wybierz klienta, aby pobrać adresy dostaw.");
        return;
    }
    // Pobierz ID klienta na podstawie numeru klienta
    int clientId = -1;
    QSqlQuery q(DbManager::instance().database());
    q.prepare("SELECT id FROM clients WHERE client_number = ?");
    q.addBindValue(clientNumber);
    if (q.exec() && q.next()) {
        clientId = q.value(0).toInt();
    }
    if (clientId <= 0) {
        QMessageBox::warning(this, "Brak klienta", "Nie znaleziono klienta o podanym numerze. Wybierz klienta z bazy.");
        return;
    }
    DeliveryAddressesView *dlg = new DeliveryAddressesView(clientId);
    dlg->setWindowTitle("Wybierz adres dostawy");
    dlg->setMinimumWidth(900);
    dlg->setMinimumHeight(400);
    dlg->setWindowModality(Qt::ApplicationModal);
    // Dodaj kolumnę 'nazwa' jeśli nie istnieje (w DeliveryAddressesView)
    // (Zakładamy, że DeliveryAddressesView obsługuje kolumnę 'nazwa' jeśli jest w danych)
    connect(dlg, &DeliveryAddressesView::addressSelected, this, [this, dlg](const QMap<QString, QVariant> &addressData) {
        if (!addressData.isEmpty()) {
            addrFields[0]->setText(addressData.value("Firma").toString());
            addrFields[1]->setText(addressData.value("Ulica").toString());
            addrFields[2]->setText(addressData.value("Kod pocztowy").toString());
            addrFields[3]->setText(addressData.value("Miasto").toString());
            addrFields[4]->setText(addressData.value("Osoba kontaktowa").toString());
            addrFields[5]->setText(addressData.value("Telefon").toString());
            // Można dodać obsługę nazwy jeśli jest potrzebna
            dlg->close();
        }
    });
    dlg->show();
}

void NewOrderView::handleAddNewClient() {
    // Pobierz dane z pól
    QMap<QString, QVariant> data;
    // Wygeneruj unikalny numer klienta
    int nextNr = DbManager::instance().getNextUniqueClientNumber();
    QString clientNumber = QString::number(nextNr).rightJustified(6, '0');
    data["client_number"] = clientNumber;
    data["name"] = clientFields[0]->text();
    data["short_name"] = clientFields[1]->text();
    data["contact_person"] = clientFields[2]->text();
    data["phone"] = clientFields[3]->text();
    data["email"] = clientFields[4]->text();
    data["street"] = clientFields[5]->text();
    data["postal_code"] = clientFields[6]->text();
    data["city"] = clientFields[7]->text();
    data["nip"] = clientFields[8]->text();
    qDebug() << "[handleAddNewClient] Dane do zapisu:" << data;
    if (DbManager::instance().addClient(data)) {
        qDebug() << "[handleAddNewClient] Dodano klienta z numerem:" << clientNumber;
        clientNumberEdit->setText(clientNumber);
        QMessageBox::information(this, "Dodano klienta", "Nowy numer klienta: " + clientNumber);
        // Odśwież widok klientów jeśli jest dostępny (emit sygnał)
        emit clientAdded();
    } else {
        qDebug() << "[handleAddNewClient] Błąd przy dodawaniu klienta.";
        QMessageBox::warning(this, "Błąd", "Nie udało się dodać klienta do bazy.");
    }
}

void NewOrderView::fillFromClient() {
    // TODO: Uzupełnij pola klienta i adresu na podstawie obiektu Client
}

void NewOrderView::fetchGusData(const QString& nip) {
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
        
        // NIP - formatujemy do formatu XXX-XXX-XX-XX
        if (data.contains("nip") || data.contains("Nip")) {
            QString nipValue = data.value("nip", data.value("Nip", ""));
            nipValue.remove(QRegularExpression("[^0-9]")); // zostaw tylko cyfry
            
            if (nipValue.length() == 10) {
                QString formattedNip = nipValue.mid(0, 3) + "-" + 
                                       nipValue.mid(3, 3) + "-" +
                                       nipValue.mid(6, 2) + "-" +
                                       nipValue.mid(8, 2);
                clientFields[8]->setText(formattedNip);
                qDebug() << "[GUS] Ustawiono NIP (sformatowany):" << formattedNip;
            } else {
                clientFields[8]->setText(nipValue);
                qDebug() << "[GUS] Ustawiono NIP (bez formatowania):" << nipValue;
            }
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

void NewOrderView::loadOrderData(const QMap<QString, QVariant> &orderData, bool editModeParam) {
    qDebug() << "[loadOrderData] START: orderData keys:" << orderData.keys() << ", editMode:" << editModeParam;
    resetForm(); // zawsze czyść formularz na początku!
    qDebug() << "[loadOrderData] after resetForm: prodBlocks.size()=" << prodBlocks.size() << ", prodFieldsList.size()=" << prodFieldsList.size();
    editMode = editModeParam;
    currentOrderId = orderData.value("id", -1).toInt();
    if (orderData.isEmpty() && editMode) {
        qDebug() << "[loadOrderData] orderData.isEmpty() && editMode, return early";
        return;
    }
    // Wyczyść wszystkie pozycje produktów
    for (QWidget* block : prodBlocks) {
        block->setParent(nullptr);
        block->deleteLater();
    }
    prodBlocks.clear();
    prodFieldsList.clear();
    // Wyczyść pola formularza
    nrEdit->clear();
    orderDateEdit->setDate(QDate::currentDate());
    deliveryDateEdit->setDate(QDate::currentDate());
    clientNumberEdit->clear();
    paymentTermCombo->setCurrentIndex(0);
    notesEdit->clear();
    for (QLineEdit* f : clientFields) f->clear();
    for (QLineEdit* f : addrFields) f->clear();
    // Załaduj dane zamówienia jeśli są
    qDebug() << "[loadOrderData] orderData keys:" << orderData.keys() << ", editMode:" << editModeParam;
    if (!orderData.isEmpty()) {
        // --- SPRAWDZENIE POŁĄCZENIA Z BAZĄ ---
        QSqlDatabase db = DbManager::instance().database();
        if (!db.isOpen()) {
            qDebug() << "[loadOrderData][ERROR] Baza danych nie jest otwarta!";
            QMessageBox::critical(this, "Błąd bazy danych", "Brak połączenia z bazą danych. Nie można załadować danych zamówienia.");
            return;
        }
        // Usunięto powtórzone przypisanie editMode i currentOrderId
        if (editMode)
            nrEdit->setText(orderData.value("order_number").toString());
        qDebug() << "[loadOrderData] order_number:" << orderData.value("order_number").toString();
        orderDateEdit->setDate(orderData.value("order_date").toDate());
        deliveryDateEdit->setDate(orderData.value("delivery_date").toDate());
        int clientId = orderData.value("client_id").toInt();
        qDebug() << "[loadOrderData] client_id:" << clientId;
        // --- Pobierz dane klienta bezpośrednio z bazy ---
        QSqlQuery qClient(db);
        qClient.prepare("SELECT client_number, name, short_name, contact_person, phone, email, street, postal_code, city, nip FROM clients WHERE id = ?");
        qClient.addBindValue(clientId);
        if (qClient.exec() && qClient.next()) {
            clientNumberEdit->setText(qClient.value(0).toString());
            clientFields[0]->setText(qClient.value(1).toString());
            clientFields[1]->setText(qClient.value(2).toString());
            clientFields[2]->setText(qClient.value(3).toString());
            clientFields[3]->setText(qClient.value(4).toString());
            clientFields[4]->setText(qClient.value(5).toString());
            clientFields[5]->setText(qClient.value(6).toString());
            clientFields[6]->setText(qClient.value(7).toString());
            clientFields[7]->setText(qClient.value(8).toString());
            // Ustaw NIP jako 10 cyfr bez myślników
            QString nip = qClient.value(9).toString();
            nip.remove(QRegularExpression("[^0-9]"));
            clientFields[8]->setText(nip);
            qDebug() << "[loadOrderData] Found client by id:" << clientId;
        } else {
            qDebug() << "[loadOrderData] Client not found for id:" << clientId;
        }
        paymentTermCombo->setCurrentText(orderData.value("payment_term").toString());
        notesEdit->setText(orderData.value("notes").toString());
        // --- Pobierz pozycje zamówienia i wypełnij bloki ---
        QVector<QMap<QString, QVariant>> items;
        QSqlQuery qItems(db); // Użyj tej samej otwartej bazy
        qItems.prepare("SELECT width, height, material, ordered_quantity, quantity_type, roll_length, core, price, price_type, zam_rolki FROM order_items WHERE order_id = ?");
        qItems.addBindValue(orderData.value("id"));
        if (qItems.exec()) {
            while (qItems.next()) {
                QMap<QString, QVariant> item;
                item["width"] = qItems.value(0);
                item["height"] = qItems.value(1);
                item["material"] = qItems.value(2);
                item["ordered_quantity"] = qItems.value(3);
                item["quantity_type"] = qItems.value(4);
                item["roll_length"] = qItems.value(5);
                item["core"] = qItems.value(6);
                item["price"] = qItems.value(7);
                item["price_type"] = qItems.value(8);
                item["zam_rolki"] = qItems.value(9);
                items.append(item);
            }
        } else {
            qDebug() << "[loadOrderData] order_items query error:" << qItems.lastError().text();
        }
        qDebug() << "[loadOrderData] loaded items count:" << items.size();
        for (QWidget* block : prodBlocks) {
            qDebug() << "[loadOrderData] deleting block:" << block;
            block->setParent(nullptr);
            block->deleteLater();
        }
        prodBlocks.clear();
        prodFieldsList.clear();
        for (int i = 0; i < items.size(); ++i) {
            addProductBlock(i+1);
            qDebug() << "[loadOrderData] addProductBlock(" << (i+1) << ")";
            auto& p = prodFieldsList.last();
            static_cast<QLineEdit*>(p["Szerokość"])->setText(items[i]["width"].toString());
            static_cast<QLineEdit*>(p["Wysokość"])->setText(items[i]["height"].toString());
            static_cast<QComboBox*>(p["Rodzaj materiału"])->setCurrentText(items[i]["material"].toString());
            static_cast<QLineEdit*>(p["zam. ilość"])->setText(items[i]["ordered_quantity"].toString());
            static_cast<QComboBox*>(p["Typ ilości"])->setCurrentText(items[i]["quantity_type"].toString());
            static_cast<QLineEdit*>(p["nawój/długość"])->setText(items[i]["roll_length"].toString());
            static_cast<QLineEdit*>(p["Cena"])->setText(items[i]["price"].toString());
            static_cast<QComboBox*>(p["CenaTyp"])->setCurrentText(items[i]["price_type"].toString());
            static_cast<QLineEdit*>(p["zam. rolki"])->setText(items[i]["zam_rolki"].toString());
            QString coreVal = items[i]["core"].toString();
            QComboBox* rdzenCombo = static_cast<QComboBox*>(p["Rdzeń"]);
            int idx = rdzenCombo->findText(coreVal);
            if (idx >= 0) {
                rdzenCombo->setCurrentIndex(idx);
            } else {
                rdzenCombo->setCurrentText("inny");
                static_cast<QLineEdit*>(p["Rdzeń_inny"])->setText(coreVal);
            }
            qDebug() << "[loadOrderData] filled block " << i << ": width=" << items[i]["width"] << ", material=" << items[i]["material"];
        }
        // --- Pobierz adres dostawy (jeśli jest) ---
        QSqlQuery qAddr(db); // Użyj tej samej otwartej bazy
        qAddr.prepare("SELECT delivery_company, delivery_street, delivery_postal_code, delivery_city, delivery_contact_person, delivery_phone FROM orders WHERE id = ?");
        qAddr.addBindValue(orderData.value("id"));
        if (qAddr.exec() && qAddr.next()) {
            for (int i = 0; i < 6; ++i) {
                addrFields[i]->setText(qAddr.value(i).toString());
            }
        } else {
            qDebug() << "[loadOrderData] delivery address query error:" << qAddr.lastError().text();
        }
    }
    // Jeśli tryb duplikacji, wyczyść numer zamówienia i daty na dziś
    if (!editMode) {
        qDebug() << "[loadOrderData] DUPLICATE MODE: clear nrEdit, set dates to today";
        nrEdit->clear();
        orderDateEdit->setDate(QDate::currentDate());
        deliveryDateEdit->setDate(QDate::currentDate());
    }
    if (prodBlocks.isEmpty()) {
        qDebug() << "[loadOrderData] prodBlocks.isEmpty() -> add 2 default blocks";
        addProductBlock(1);
        addProductBlock(2);
    }
    qDebug() << "[loadOrderData] END: prodBlocks.size()=" << prodBlocks.size() << ", prodFieldsList.size()=" << prodFieldsList.size();
}

void NewOrderView::resetForm() {
    qDebug() << "[resetForm] Czyszczenie formularza nowego zamówienia";
    // Wyczyść wszystkie pozycje produktów
    for (QWidget* block : prodBlocks) {
        block->setParent(nullptr);
        block->deleteLater();
    }
    prodBlocks.clear();
    prodFieldsList.clear();
    // Wyczyść pola formularza
    nrEdit->clear();
    orderDateEdit->setDate(QDate::currentDate());
    deliveryDateEdit->setDate(QDate::currentDate());
    clientNumberEdit->clear();
    paymentTermCombo->setCurrentIndex(0);
    notesEdit->clear();
    for (QLineEdit* f : clientFields) f->clear();
    for (QLineEdit* f : addrFields) f->clear();
    // Dodaj domyślne dwa bloki pozycji
    addProductBlock(1);
    addProductBlock(2);
    editMode = false;
    currentOrderId = -1;
}
