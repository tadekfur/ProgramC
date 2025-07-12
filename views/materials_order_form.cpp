#include "materials_order_form.h"
#include "db/dbmanager.h"
#include "utils/email_sender.h"

#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QPaintDevice>
#include <QPainter>
#include <QPdfWriter>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTextDocument>

MaterialsOrderForm::MaterialsOrderForm(QWidget *parent)
    : QWidget(parent)
{
    qDebug() << "[DEBUG] MaterialsOrderForm konstruktor START";
    try {
        materialModel = new QStringListModel(this);
        materialCompleter = nullptr;
        widthModel = new QStringListModel(this);
        lengthModel = new QStringListModel(this);
        qtyModel = new QStringListModel(this);
        widthTypes = {};
        lengthTypes = {};
        qtyTypes = {};
        setupUi();
        refreshMaterialTypes();
        // --- Pobierz podpowiedzi szerokości, długości i ilości rolek z bazy ---
        QVector<QVariant> widthVals = DbManager::instance().getUniqueMaterialWidths();
        QVector<QVariant> lengthVals = DbManager::instance().getUniqueMaterialLengths();
        QVector<QVariant> qtyVals = DbManager::instance().getUniqueMaterialRolls();
        widthTypes.clear();
        for (const auto &v : widthVals) widthTypes << v.toString();
        lengthTypes.clear();
        for (const auto &v : lengthVals) lengthTypes << v.toString();
        qtyTypes.clear();
        for (const auto &v : qtyVals) qtyTypes << v.toString();
        widthModel->setStringList(widthTypes);
        lengthModel->setStringList(lengthTypes);
        qtyModel->setStringList(qtyTypes);
        qDebug() << "[DEBUG] MaterialsOrderForm konstruktor END";
    } catch (const std::exception &e) {
        qCritical() << "[ERROR] Wyjątek w konstruktorze MaterialsOrderForm:" << e.what();
    } catch (...) {
        qCritical() << "[ERROR] Nieznany wyjątek w konstruktorze MaterialsOrderForm";
    }
}

MaterialsOrderForm::~MaterialsOrderForm() {
    qDebug() << "[DEBUG] MaterialsOrderForm destruktor START";
    // ...existing code...
    qDebug() << "[DEBUG] MaterialsOrderForm destruktor END";
}

void MaterialsOrderForm::setupUi() {
    qDebug() << "[DEBUG] setupUi() START";
    try {
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        // --- Sekcja: Numer zamówienia ---
        QHBoxLayout *nrLayout = new QHBoxLayout;
        nrLayout->addWidget(new QLabel("Nr zamówienia:"));
        orderNumberEdit = new QLineEdit;
        orderNumberEdit->setReadOnly(true);
        orderNumberEdit->setMaximumWidth(220);
        orderNumberEdit->setText(generateOrderNumber());
        nrLayout->addWidget(orderNumberEdit);
        nrLayout->addStretch(1);
        mainLayout->addLayout(nrLayout);
        // --- Sekcja: Dostawca + Adres dostawy w jednym wierszu ---
        QHBoxLayout *topDataLayout = new QHBoxLayout;
        qDebug() << "[DEBUG] Tworzenie sekcji dostawcy";
        QGroupBox *supplierGroup = new QGroupBox("Dane dostawcy");
        QGridLayout *supplierGrid = new QGridLayout(supplierGroup);
        supplierGrid->addWidget(new QLabel("Nazwa:"), 0, 0);
        supplierNameEdit = new QLineEdit;
        supplierGrid->addWidget(supplierNameEdit, 0, 1);
        supplierGrid->addWidget(new QLabel("Ulica i nr:"), 1, 0);
        supplierStreetEdit = new QLineEdit;
        supplierGrid->addWidget(supplierStreetEdit, 1, 1);
        supplierGrid->addWidget(new QLabel("Miasto:"), 2, 0);
        supplierCityEdit = new QLineEdit;
        supplierGrid->addWidget(supplierCityEdit, 2, 1);
        supplierGrid->addWidget(new QLabel("Kod pocztowy:"), 3, 0);
        supplierPostalEdit = new QLineEdit;
        supplierGrid->addWidget(supplierPostalEdit, 3, 1);
        supplierGrid->addWidget(new QLabel("Kraj:"), 4, 0);
        supplierCountryEdit = new QLineEdit;
        supplierGrid->addWidget(supplierCountryEdit, 4, 1);
        supplierGrid->addWidget(new QLabel("Osoba kontaktowa:"), 5, 0);
        supplierContactEdit = new QLineEdit;
        supplierGrid->addWidget(supplierContactEdit, 5, 1);
        supplierGrid->addWidget(new QLabel("Telefon:"), 6, 0);
        supplierPhoneEdit = new QLineEdit;
        supplierGrid->addWidget(supplierPhoneEdit, 6, 1);
        supplierGrid->addWidget(new QLabel("E-mail:"), 7, 0);
        supplierEmailEdit = new QLineEdit;
        supplierGrid->addWidget(supplierEmailEdit, 7, 1);
        btnSelectSupplier = new QPushButton("Wstaw z bazy");
        btnSaveSupplier = new QPushButton("Zapisz dostawcę");
        btnEditSupplier = new QPushButton("Edytuj");
        QHBoxLayout *supBtnLayout = new QHBoxLayout;
        supBtnLayout->addWidget(btnSelectSupplier);
        supBtnLayout->addWidget(btnSaveSupplier);
        supBtnLayout->addWidget(btnEditSupplier);
        supplierGrid->addLayout(supBtnLayout, 8, 0, 1, 2);
        topDataLayout->addWidget(supplierGroup, 1);
        qDebug() << "[DEBUG] Tworzenie sekcji adresu dostawy";
        QGroupBox *deliveryGroup = new QGroupBox("Adres dostawy");
        QGridLayout *deliveryGrid = new QGridLayout(deliveryGroup);
        deliveryGrid->addWidget(new QLabel("Firma:"), 0, 0);
        deliveryCompanyEdit = new QLineEdit;
        deliveryCompanyEdit->setText("TERMEDIA Magdalena Żemła");
        deliveryGrid->addWidget(deliveryCompanyEdit, 0, 1);
        deliveryGrid->addWidget(new QLabel("Ulica i nr:"), 1, 0);
        deliveryStreetEdit2 = new QLineEdit;
        deliveryStreetEdit2->setText("ul. Przemysłowa 60");
        deliveryGrid->addWidget(deliveryStreetEdit2, 1, 1);
        deliveryGrid->addWidget(new QLabel("Miasto:"), 2, 0);
        deliveryCityEdit2 = new QLineEdit;
        deliveryCityEdit2->setText("Tychy");
        deliveryGrid->addWidget(deliveryCityEdit2, 2, 1);
        deliveryGrid->addWidget(new QLabel("Kod pocztowy:"), 3, 0);
        deliveryPostalEdit2 = new QLineEdit;
        deliveryPostalEdit2->setText("43-110");
        deliveryGrid->addWidget(deliveryPostalEdit2, 3, 1);
        deliveryGrid->addWidget(new QLabel("Kraj:"), 4, 0);
        deliveryCountryEdit2 = new QLineEdit;
        deliveryCountryEdit2->setText("Polska");
        deliveryGrid->addWidget(deliveryCountryEdit2, 4, 1);
        deliveryGrid->addWidget(new QLabel("NIP:"), 5, 0);
        deliveryNipEdit = new QLineEdit;
        deliveryNipEdit->setText("PL6381016402");
        deliveryGrid->addWidget(deliveryNipEdit, 5, 1);
        btnSelectDelivery = new QPushButton("Wstaw z bazy");
        btnSaveDelivery = new QPushButton("Zapisz adres");
        btnEditDelivery = new QPushButton("Edytuj");
        QHBoxLayout *delBtnLayout = new QHBoxLayout;
        delBtnLayout->addWidget(btnSelectDelivery);
        delBtnLayout->addWidget(btnSaveDelivery);
        delBtnLayout->addWidget(btnEditDelivery);
        deliveryGrid->addLayout(delBtnLayout, 6, 0, 1, 2);
        topDataLayout->addWidget(deliveryGroup, 1);
        qDebug() << "[DEBUG] Sekcje dostawcy i adresu dostawy utworzone";
        mainLayout->addLayout(topDataLayout);
        // --- Sekcja: Materiały ---
        QGroupBox *materialsGroup = new QGroupBox("Materiały");
        QVBoxLayout *materialsLayout = new QVBoxLayout(materialsGroup);
        materialsTable = new QTableWidget(0, 5, this);
        QStringList headers = {"Nr", "Rodzaj materiału", "Szerokość [mm]", "Długość [mb]", "Ilość rolek"};
        materialsTable->setHorizontalHeaderLabels(headers);
        materialsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        materialsTable->setSelectionMode(QAbstractItemView::SingleSelection);
        materialsTable->setEditTriggers(QAbstractItemView::AllEditTriggers);
        materialsLayout->addWidget(materialsTable);
        QHBoxLayout *materialsBtnLayout = new QHBoxLayout;
        materialsBtnLayout->setContentsMargins(0,0,0,0);
        materialsBtnLayout->setSpacing(4);
        btnAddMaterial = new QPushButton("Dodaj pozycję");
        btnDeleteMaterial = new QPushButton("Usuń pozycję");
        materialsBtnLayout->addWidget(btnAddMaterial);
        materialsBtnLayout->addWidget(btnDeleteMaterial);
        materialsBtnLayout->addStretch(1);
        materialsLayout->addLayout(materialsBtnLayout, 0);
        materialsLayout->setAlignment(materialsBtnLayout, Qt::AlignLeft);
        mainLayout->addWidget(materialsGroup);
        // --- Sekcja: Termin dostawy i uwagi ---
        QHBoxLayout *bottomLayout = new QHBoxLayout;
        bottomLayout->addWidget(new QLabel("Termin dostawy:"));
        deliveryDateEdit = new QDateEdit(QDate::currentDate());
        deliveryDateEdit->setCalendarPopup(true);
        deliveryDateEdit->setMaximumWidth(150);
        bottomLayout->addWidget(deliveryDateEdit);
        bottomLayout->addSpacing(30);
        bottomLayout->addWidget(new QLabel("Uwagi:"));
        notesEdit = new QTextEdit;
        notesEdit->setMaximumHeight(60);
        bottomLayout->addWidget(notesEdit);
        mainLayout->addLayout(bottomLayout);
        // --- Przycisk zapisu ---
        btnSaveOrder = new QPushButton(" Zapisz zamówienie");
        btnSaveOrder->setMinimumWidth(220);
        btnSaveOrder->setMinimumHeight(44);
        mainLayout->addWidget(btnSaveOrder, 0, Qt::AlignCenter);
        // --- Przycisk wczytywania zamówienia ---
        btnLoadOrder = new QPushButton("Wczytaj zamówienie...");
        mainLayout->addWidget(btnLoadOrder, 0, Qt::AlignLeft);
        // --- Połączenia ---
        qDebug() << "[DEBUG] setupUi() - podpinanie sygnałów";
        connect(btnAddMaterial, &QPushButton::clicked, this, &MaterialsOrderForm::addMaterialRow);
        connect(btnDeleteMaterial, &QPushButton::clicked, this, &MaterialsOrderForm::handleDeleteMaterial);
        connect(btnSelectSupplier, &QPushButton::clicked, this, &MaterialsOrderForm::handleSelectSupplier);
        connect(btnSaveSupplier, &QPushButton::clicked, this, &MaterialsOrderForm::handleSaveSupplier);
        connect(btnSelectDelivery, &QPushButton::clicked, this, &MaterialsOrderForm::handleSelectDelivery);
        connect(btnSaveDelivery, &QPushButton::clicked, this, &MaterialsOrderForm::handleSaveDelivery);
        connect(btnEditSupplier, &QPushButton::clicked, this, &MaterialsOrderForm::handleEditSupplier);
        connect(btnEditDelivery, &QPushButton::clicked, this, &MaterialsOrderForm::handleEditDelivery);
        connect(btnSaveOrder, &QPushButton::clicked, this, &MaterialsOrderForm::handleSaveOrder);
        connect(btnLoadOrder, &QPushButton::clicked, this, [this]() { this->showOrderSelectDialog(); });
        qDebug() << "[DEBUG] setupUi() END";
    } catch (const std::exception &e) {
        qCritical() << "[ERROR] Wyjątek w setupUi MaterialsOrderForm:" << e.what();
    } catch (...) {
        qCritical() << "[ERROR] Nieznany wyjątek w setupUi MaterialsOrderForm";
    }
}

void MaterialsOrderForm::addMaterialRow() {
    qDebug() << "[DEBUG] addMaterialRow() start";
    int row = materialsTable ? materialsTable->rowCount() : -1;
    if (!materialsTable) {
        qCritical() << "[ERROR] materialsTable == nullptr w addMaterialRow";
        return;
    }
    materialsTable->insertRow(row);
    QTableWidgetItem *nrItem = new QTableWidgetItem(QString::number(row + 1));
    nrItem->setFlags(nrItem->flags() & ~Qt::ItemIsEditable);
    materialsTable->setItem(row, 0, nrItem);
    // --- QComboBox bez completera, tylko wybór lub wpisanie nowego ---
    QComboBox *materialCombo = new QComboBox;
    materialCombo->setEditable(true); // Pozostaw możliwość wpisania nowego
    materialCombo->setModel(materialModel);
    materialCombo->setCurrentText("");
    // Dodaj do listy TYLKO po zatwierdzeniu wpisu (Enter lub opuszczenie pola)
    QLineEdit *editor = materialCombo->lineEdit();
    connect(editor, &QLineEdit::editingFinished, this, [this, materialCombo]() {
        QString text = materialCombo->currentText().trimmed();
        if (text.length() >= 2 && !materialTypes.contains(text, Qt::CaseInsensitive)) {
            materialTypes << text;
            materialModel->setStringList(materialTypes);
            qDebug() << "[DEBUG] Dodano nowy rodzaj materiału (zatwierdzony):" << text;
        }
        // Ustaw wpisany tekst jako wybrany, nawet jeśli nie ma go na liście
        materialCombo->setCurrentText(text);
    });
    materialsTable->setCellWidget(row, 1, materialCombo);

    QComboBox *widthCombo = new QComboBox;
    widthCombo->setEditable(true);
    widthCombo->setModel(widthModel);
    widthCombo->setCurrentText("");
    QLineEdit *widthEditor = widthCombo->lineEdit();
    connect(widthEditor, &QLineEdit::editingFinished, this, [this, widthCombo]() {
        QString text = widthCombo->currentText().trimmed();
        if (text.length() >= 1 && !widthTypes.contains(text, Qt::CaseInsensitive)) {
            widthTypes << text;
            widthModel->setStringList(widthTypes);
            qDebug() << "[DEBUG] Dodano nową szerokość (zatwierdzona):" << text;
        }
        widthCombo->setCurrentText(text);
    });
    materialsTable->setCellWidget(row, 2, widthCombo);

    QComboBox *lengthCombo = new QComboBox;
    lengthCombo->setEditable(true);
    lengthCombo->setModel(lengthModel);
    lengthCombo->setCurrentText("");
    QLineEdit *lengthEditor = lengthCombo->lineEdit();
    connect(lengthEditor, &QLineEdit::editingFinished, this, [this, lengthCombo]() {
        QString text = lengthCombo->currentText().trimmed();
        if (text.length() >= 1 && !lengthTypes.contains(text, Qt::CaseInsensitive)) {
            lengthTypes << text;
            lengthModel->setStringList(lengthTypes);
            qDebug() << "[DEBUG] Dodano nową długość (zatwierdzona):" << text;
        }
        lengthCombo->setCurrentText(text);
    });
    materialsTable->setCellWidget(row, 3, lengthCombo);

    QComboBox *qtyCombo = new QComboBox;
    qtyCombo->setEditable(true);
    qtyCombo->setModel(qtyModel);
    qtyCombo->setCurrentText("");
    QLineEdit *qtyEditor = qtyCombo->lineEdit();
    connect(qtyEditor, &QLineEdit::editingFinished, this, [this, qtyCombo]() {
        QString text = qtyCombo->currentText().trimmed();
        if (text.length() >= 1 && !qtyTypes.contains(text, Qt::CaseInsensitive)) {
            qtyTypes << text;
            qtyModel->setStringList(qtyTypes);
            qDebug() << "[DEBUG] Dodano nową ilość rolek (zatwierdzona):" << text;
        }
        qtyCombo->setCurrentText(text);
    });
    materialsTable->setCellWidget(row, 4, qtyCombo);
    qDebug() << "[DEBUG] addMaterialRow() end";
}

void MaterialsOrderForm::refreshMaterialTypes() {
    qDebug() << "[DEBUG] refreshMaterialTypes() start";
    try {
        materialTypes.clear();
        QVector<QMap<QString, QVariant>> materials = DbManager::instance().getMaterialsCatalog();
        for (const auto &mat : materials) {
            materialTypes << mat["name"].toString();
        }
        if (materialTypes.isEmpty()) {
            materialTypes = {"Folia PET", "Folia PE", "Papier", "Inny"};
        }
        if (!materialModel) {
            materialModel = new QStringListModel(this);
        }
        materialModel->setStringList(materialTypes);
        if (!materialCompleter) {
            materialCompleter = new QCompleter(materialModel, this);
        } else {
            materialCompleter->setModel(materialModel);
        }
        materialCompleter->setCaseSensitivity(Qt::CaseInsensitive);
        // Odśwież podpowiedzi szerokości, długości i ilości rolek
        QVector<QVariant> widthVals = DbManager::instance().getUniqueMaterialWidths();
        QVector<QVariant> lengthVals = DbManager::instance().getUniqueMaterialLengths();
        QVector<QVariant> qtyVals = DbManager::instance().getUniqueMaterialRolls();
        widthTypes.clear();
        for (const auto &v : widthVals) widthTypes << v.toString();
        lengthTypes.clear();
        for (const auto &v : lengthVals) lengthTypes << v.toString();
        qtyTypes.clear();
        for (const auto &v : qtyVals) qtyTypes << v.toString();
        widthModel->setStringList(widthTypes);
        lengthModel->setStringList(lengthTypes);
        qtyModel->setStringList(qtyTypes);
        qDebug() << "[DEBUG] refreshMaterialTypes() end";
    } catch (const std::exception &e) {
        qCritical() << "[ERROR] Wyjątek w refreshMaterialTypes:" << e.what();
    } catch (...) {
        qCritical() << "[ERROR] Nieznany wyjątek w refreshMaterialTypes";
    }
}

QString MaterialsOrderForm::generateOrderNumber() {
    qDebug() << "[DEBUG] generateOrderNumber() start";
    // Nowy numer z sekwencji w bazie
    QString result = DbManager::instance().getNextMaterialsOrderNumber();
    qDebug() << "[DEBUG] generateOrderNumber() result:" << result;
    return result;
}

void MaterialsOrderForm::removeMaterialRow(int row) {
    qDebug() << "[DEBUG] removeMaterialRow(" << row << ") start";
    if (row < 0 || row >= materialsTable->rowCount()) return;
    materialsTable->removeRow(row);
    // Przebuduj numerację pozycji
    for (int i = 0; i < materialsTable->rowCount(); ++i) {
        QTableWidgetItem *nrItem = materialsTable->item(i, 0);
        if (nrItem) nrItem->setText(QString::number(i + 1));
    }
    qDebug() << "[DEBUG] removeMaterialRow(" << row << ") end";
}

void MaterialsOrderForm::handleDeleteMaterial() {
    qDebug() << "[DEBUG] handleDeleteMaterial()";
    if (!materialsTable) return;
    int row = materialsTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Brak zaznaczenia", "Wybierz wiersz do usunięcia.");
        return;
    }
    removeMaterialRow(row);
}

void MaterialsOrderForm::handleSelectSupplier() {
    qDebug() << "[DEBUG] handleSelectSupplier()";
    QVector<QMap<QString, QVariant>> suppliers = DbManager::instance().getSuppliers();
    QStringList supplierList;
    for (const auto &s : suppliers) {
        supplierList << s["name"].toString() + " | " + s["city"].toString();
    }
    bool ok = false;
    QString selected = QInputDialog::getItem(this, "Wybierz dostawcę", "Dostawcy:", supplierList, 0, false, &ok);
    if (!ok || selected.isEmpty()) return;
    int idx = supplierList.indexOf(selected);
    if (idx < 0) return;
    const auto &s = suppliers[idx];
    supplierNameEdit->setText(s["name"].toString());
    supplierStreetEdit->setText(s["street"].toString());
    supplierCityEdit->setText(s["city"].toString());
    supplierPostalEdit->setText(s["postal_code"].toString());
    supplierCountryEdit->setText(s["country"].toString());
    supplierContactEdit->setText(s["contact_person"].toString());
    supplierPhoneEdit->setText(s["phone"].toString());
    supplierEmailEdit->setText(s["email"].toString());
}

void MaterialsOrderForm::handleSaveSupplier() {
    qDebug() << "[DEBUG] handleSaveSupplier()";
    QMap<QString, QVariant> data;
    data["name"] = supplierNameEdit->text();
    data["street"] = supplierStreetEdit->text();
    data["city"] = supplierCityEdit->text();
    data["postal_code"] = supplierPostalEdit->text();
    data["country"] = supplierCountryEdit->text();
    data["contact_person"] = supplierContactEdit->text();
    data["phone"] = supplierPhoneEdit->text();
    data["email"] = supplierEmailEdit->text();
    if (data["name"].toString().isEmpty()) {
        QMessageBox::warning(this, "Błąd", "Nazwa dostawcy nie może być pusta!");
        return;
    }
    
    bool ok = DbManager::instance().addSupplier(data);
    if (ok) {
        QMessageBox::information(this, "Sukces", "Dostawca został zapisany do bazy.");
    } else {
        QMessageBox::critical(this, "Błąd", "Nie udało się zapisać dostawcy do bazy!");
    }
    refreshMaterialTypes(); // odśwież listę materiałów, jeśli powiązane
}

void MaterialsOrderForm::handleSelectDelivery() {
    qDebug() << "[DEBUG] handleSelectDelivery()";
    
    // Pobierz wszystkie adresy dostawy
    QVector<QMap<QString, QVariant>> addresses = DbManager::instance().getDeliveryAddresses();
    if (addresses.isEmpty()) {
        QMessageBox::information(this, "Brak adresów", "Brak zapisanych adresów dostawy.");
        return;
    }

    // Przygotuj listę do wyświetlenia w oknie dialogowym
    QStringList addressList;
    for (const auto &addr : addresses) {
        QString company = addr["company"].toString();
        QString city = addr["city"].toString();
        addressList << QString("%1 | %2").arg(company, city);
    }

    // Wyświetl okno wyboru
    bool ok;
    QString selected = QInputDialog::getItem(
        this,
        "Wybierz adres dostawy",
        "Dostępne adresy:",
        addressList,
        0, // domyślny indeks
        false, // nie edytowalne
        &ok
    );

    if (!ok || selected.isEmpty()) {
        return; // użytkownik anulował
    }

    // Znajdź wybrany adres
    int idx = addressList.indexOf(selected);
    if (idx < 0 || idx >= addresses.size()) {
        return;
    }

    const auto &addr = addresses[idx];
    
    // Zapisz ID wybranego adresu
    currentDeliveryAddressId = addr["id"].toInt();
    qDebug() << "[DEBUG] Wybrano adres dostawy ID:" << currentDeliveryAddressId;
    
    // Uzupełnij pola formularza
    deliveryCompanyEdit->setText(addr["company"].toString());
    deliveryStreetEdit2->setText(addr["street"].toString());
    deliveryCityEdit2->setText(addr["city"].toString());
    deliveryPostalEdit2->setText(addr["postal_code"].toString());
    deliveryCountryEdit2->setText(addr["country"].toString());
    deliveryNipEdit->setText(addr["nip"].toString());
}

void MaterialsOrderForm::handleSaveDelivery() {
    qDebug() << "[DEBUG] handleSaveDelivery()";
    
    // Przygotuj dane do zapisania
    QMap<QString, QVariant> data;
    data["company"] = deliveryCompanyEdit->text().trimmed();
    data["street"] = deliveryStreetEdit2->text().trimmed();
    data["city"] = deliveryCityEdit2->text().trimmed();
    data["postal_code"] = deliveryPostalEdit2->text().trimmed();
    data["country"] = deliveryCountryEdit2->text().trimmed();
    data["nip"] = deliveryNipEdit->text().trimmed();
    data["name"] = data["company"]; // nazwa firmy jako domyślna nazwa adresu
    data["contact_person"] = ""; // opcjonalne
    data["phone"] = ""; // opcjonalne

    // Walidacja
    if (data["company"].toString().isEmpty()) {
        QMessageBox::warning(this, "Błąd", "Nazwa firmy nie może być pusta!");
        return;
    }

    if (data["street"].toString().isEmpty() || data["city"].toString().isEmpty()) {
        QMessageBox::warning(this, "Błąd", "Ulica i miasto są wymagane!");
        return;
    }

    // Zapisz do bazy
    bool ok = DbManager::instance().addDeliveryAddress(data);
    if (ok) {
        QMessageBox::information(this, "Sukces", "Adres dostawy został zapisany.");
    } else {
        QMessageBox::critical(this, "Błąd", "Nie udało się zapisać adresu dostawy!");
    }
}

bool MaterialsOrderForm::showEditSupplierDialog(QMap<QString, QVariant> &data) {
    QDialog dialog(this);
    dialog.setWindowTitle("Edytuj dane dostawcy");
    dialog.setMinimumWidth(600);  // Zwiększona minimalna szerokość okna
    
    QFormLayout form(&dialog);
    form.setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form.setLabelAlignment(Qt::AlignRight);
    
    // Pola formularza z ustawioną minimalną szerokością
    QLineEdit *nameEdit = new QLineEdit(data["name"].toString(), &dialog);
    QLineEdit *streetEdit = new QLineEdit(data["street"].toString(), &dialog);
    QLineEdit *cityEdit = new QLineEdit(data["city"].toString(), &dialog);
    QLineEdit *postalEdit = new QLineEdit(data["postal_code"].toString(), &dialog);
    QLineEdit *countryEdit = new QLineEdit(data["country"].toString(), &dialog);
    QLineEdit *contactEdit = new QLineEdit(data["contact_person"].toString(), &dialog);
    QLineEdit *phoneEdit = new QLineEdit(data["phone"].toString(), &dialog);
    QLineEdit *emailEdit = new QLineEdit(data["email"].toString(), &dialog);
    
    // Ustawienie minimalnej szerokości pól tekstowych
    const int fieldWidth = 350;  // Zwiększona szerokość pól
    nameEdit->setMinimumWidth(fieldWidth);
    streetEdit->setMinimumWidth(fieldWidth);
    cityEdit->setMinimumWidth(fieldWidth);
    postalEdit->setMinimumWidth(100);  // Mniejsza szerokość dla kodu pocztowego
    countryEdit->setMinimumWidth(fieldWidth);
    contactEdit->setMinimumWidth(fieldWidth);
    phoneEdit->setMinimumWidth(200);   // Średnia szerokość dla telefonu
    emailEdit->setMinimumWidth(fieldWidth);
    
    // Dodanie pól do formularza
    form.addRow("Nazwa:", nameEdit);
    form.addRow("Ulica i nr:", streetEdit);
    form.addRow("Miasto:", cityEdit);
    form.addRow("Kod pocztowy:", postalEdit);
    form.addRow("Kraj:", countryEdit);
    form.addRow("Osoba kontaktowa:", contactEdit);
    form.addRow("Telefon:", phoneEdit);
    form.addRow("E-mail:", emailEdit);
    
    // Przyciski
    QDialogButtonBox buttonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel,
                             Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    
    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    if (dialog.exec() == QDialog::Accepted) {
        data["name"] = nameEdit->text().trimmed();
        data["street"] = streetEdit->text().trimmed();
        data["city"] = cityEdit->text().trimmed();
        data["postal_code"] = postalEdit->text().trimmed();
        data["country"] = countryEdit->text().trimmed();
        data["contact_person"] = contactEdit->text().trimmed();
        data["phone"] = phoneEdit->text().trimmed();
        data["email"] = emailEdit->text().trimmed();
        return true;
    }
    return false;
}

bool MaterialsOrderForm::showEditDeliveryDialog(QMap<QString, QVariant> &data) {
    QDialog dialog(this);
    dialog.setWindowTitle("Edytuj adres dostawy");
    dialog.setMinimumWidth(600);  // Zwiększona minimalna szerokość okna
    
    QFormLayout form(&dialog);
    form.setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form.setLabelAlignment(Qt::AlignRight);
    
    // Pola formularza z wartościami domyślnymi, jeśli puste
    QString company = data["company"].toString();
    QString street = data["street"].toString();
    QString city = data["city"].toString();
    QString postal = data["postal_code"].toString();
    QString country = data["country"].toString();
    if (country.isEmpty()) country = "Polska";  // Domyślny kraj
    QString nip = data["nip"].toString();
    if (nip.isEmpty()) nip = "PL";  // Domyślny prefiks NIP
    
    QLineEdit *companyEdit = new QLineEdit(company, &dialog);
    QLineEdit *streetEdit = new QLineEdit(street, &dialog);
    QLineEdit *cityEdit = new QLineEdit(city, &dialog);
    QLineEdit *postalEdit = new QLineEdit(postal, &dialog);
    QLineEdit *countryEdit = new QLineEdit(country, &dialog);
    QLineEdit *nipEdit = new QLineEdit(nip, &dialog);
    
    // Ustawienie minimalnej szerokości pól tekstowych
    const int fieldWidth = 350;  // Zwiększona szerokość pól
    companyEdit->setMinimumWidth(fieldWidth);
    streetEdit->setMinimumWidth(fieldWidth);
    cityEdit->setMinimumWidth(fieldWidth);
    postalEdit->setMinimumWidth(100);  // Mniejsza szerokość dla kodu pocztowego
    countryEdit->setMinimumWidth(fieldWidth);
    nipEdit->setMinimumWidth(200);     // Średnia szerokość dla NIP
    
    // Dodanie pól do formularza
    form.addRow("Firma:", companyEdit);
    form.addRow("Ulica i nr:", streetEdit);
    form.addRow("Miasto:", cityEdit);
    form.addRow("Kod pocztowy:", postalEdit);
    form.addRow("Kraj:", countryEdit);
    form.addRow("NIP:", nipEdit);
    
    // Przyciski
    QDialogButtonBox buttonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel,
                             Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    
    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    if (dialog.exec() == QDialog::Accepted) {
        data["company"] = companyEdit->text().trimmed();
        data["street"] = streetEdit->text().trimmed();
        data["city"] = cityEdit->text().trimmed();
        data["postal_code"] = postalEdit->text().trimmed();
        data["country"] = countryEdit->text().trimmed();
        data["nip"] = nipEdit->text().trimmed();
        return true;
    }
    return false;
}

void MaterialsOrderForm::handleEditSupplier() {
    QMap<QString, QVariant> data;
    data["name"] = supplierNameEdit->text();
    data["street"] = supplierStreetEdit->text();
    data["city"] = supplierCityEdit->text();
    data["postal_code"] = supplierPostalEdit->text();
    data["country"] = supplierCountryEdit->text();
    data["contact_person"] = supplierContactEdit->text();
    data["phone"] = supplierPhoneEdit->text();
    data["email"] = supplierEmailEdit->text();
    
    if (showEditSupplierDialog(data)) {
        // Zastosuj zmiany w formularzu
        supplierNameEdit->setText(data["name"].toString());
        supplierStreetEdit->setText(data["street"].toString());
        supplierCityEdit->setText(data["city"].toString());
        supplierPostalEdit->setText(data["postal_code"].toString());
        supplierCountryEdit->setText(data["country"].toString());
        supplierContactEdit->setText(data["contact_person"].toString());
        supplierPhoneEdit->setText(data["phone"].toString());
        supplierEmailEdit->setText(data["email"].toString());
        
        // Zapisz zmiany w bazie
        // Pobierz ID dostawcy na podstawie nazwy i ulicy (jak w handleSaveOrder)
        int supplierId = -1;
        QVector<QMap<QString, QVariant>> suppliers = DbManager::instance().getSuppliers();
        for (const auto &s : suppliers) {
            if (s["name"].toString() == data["name"].toString() && s["street"].toString() == data["street"].toString()) {
                supplierId = s["id"].toInt();
                break;
            }
        }
        if (supplierId > 0) {
            bool ok = DbManager::instance().updateSupplier(supplierId, data);
            if (ok) {
                QMessageBox::information(this, "Edycja dostawcy", "Dane dostawcy zostały zaktualizowane w bazie.");
            } else {
                QMessageBox::critical(this, "Błąd edycji dostawcy", "Nie udało się zaktualizować danych dostawcy w bazie.");
            }
        } else {
            QMessageBox::warning(this, "Błąd", "Nie znaleziono dostawcy w bazie po edycji. Dane nie zostały zapisane.");
        }
    }
}

void MaterialsOrderForm::handleEditDelivery() {
    qDebug() << "[DEBUG] Rozpoczęcie edycji adresu dostawy";
    
    // Pobierz aktualne dane z formularza
    QMap<QString, QVariant> data;
    data["id"] = currentDeliveryAddressId; // Dodaj ID aktualnego adresu
    data["company"] = deliveryCompanyEdit->text();
    data["street"] = deliveryStreetEdit2->text();
    data["city"] = deliveryCityEdit2->text();
    data["postal_code"] = deliveryPostalEdit2->text();
    data["country"] = deliveryCountryEdit2->text();
    data["nip"] = deliveryNipEdit->text();
    data["contact_person"] = ""; // Wypełnij domyślnymi wartościami, jeśli są wymagane
    data["phone"] = "";
    
    qDebug() << "[DEBUG] Dane przed edycją:" << data;
    
    // Otwórz okno dialogowe edycji
    if (showEditDeliveryDialog(data)) {
        qDebug() << "[DEBUG] Zatwierdzono zmiany w oknie dialogowym";
        qDebug() << "[DEBUG] Dane po edycji:" << data;
        
        // Zastosuj zmiany w formularzu
        deliveryCompanyEdit->setText(data["company"].toString());
        deliveryStreetEdit2->setText(data["street"].toString());
        deliveryCityEdit2->setText(data["city"].toString());
        deliveryPostalEdit2->setText(data["postal_code"].toString());
        deliveryCountryEdit2->setText(data["country"].toString());
        deliveryNipEdit->setText(data["nip"].toString());
        
        // Zapisz zmiany w bazie danych
        qDebug() << "[DEBUG] Próba zapisu zmian w bazie danych...";
        if (DbManager::instance().updateDeliveryAddress(data)) {
            qDebug() << "[DEBUG] Pomyślnie zaktualizowano adres dostawy w bazie danych";
            QMessageBox::information(this, "Sukces", "Zmiany w adresie dostawy zostały zapisane.");
        } else {
            qWarning() << "[WARNING] Nie udało się zaktualizować adresu dostawy w bazie danych";
            QMessageBox::warning(this, "Błąd", "Nie udało się zapisać zmian w bazie danych.");
        }
    }
}

void MaterialsOrderForm::handleSaveOrder() {
    qDebug() << "[DEBUG] handleSaveOrder() start";

    QString orderNumber;
    if (loadedOrderId == -1) {
        // Nowe zamówienie
        orderNumber = generateOrderNumber();
        orderNumberEdit->setText(orderNumber);
    } else {
        // Edycja istniejącego zamówienia
        orderNumber = orderNumberEdit->text();
    }
    qDebug() << "[DEBUG] Numer zamówienia przed zapisem:" << orderNumber;
    QString supplierName = supplierNameEdit->text();
    QString supplierStreet = supplierStreetEdit->text();
    QString supplierCity = supplierCityEdit->text();
    QString supplierPostal = supplierPostalEdit->text();
    QString supplierCountry = supplierCountryEdit->text();
    QString contact = supplierContactEdit->text();
    QString phone = supplierPhoneEdit->text();
    QString email = supplierEmailEdit->text();
    QString deliveryCompany = deliveryCompanyEdit->text();
    QString deliveryStreet = deliveryStreetEdit2->text();
    QString deliveryPostal = deliveryPostalEdit2->text();
    QString deliveryCity = deliveryCityEdit2->text();
    QString deliveryCountry = deliveryCountryEdit2->text();
    QString deliveryDate = deliveryDateEdit->date().toString("yyyy-MM-dd");
    QString notes = notesEdit->toPlainText();
    QString orderDate = QDate::currentDate().toString("yyyy-MM-dd");

    // --- ZAPISZ/POBIERZ dostawcę ---
    int supplierId = -1;
    QVector<QMap<QString, QVariant>> suppliers = DbManager::instance().getSuppliers();
    for (const auto &s : suppliers) {
        if (s["name"].toString() == supplierName && s["street"].toString() == supplierStreet) {
            supplierId = s["id"].toInt();
            break;
        }
    }
    if (supplierId == -1 && !supplierName.isEmpty()) {
        QMap<QString, QVariant> data;
        data["name"] = supplierName;
        data["street"] = supplierStreet;
        data["city"] = supplierCity;
        data["postal_code"] = supplierPostal;
        data["country"] = supplierCountry;
        data["contact_person"] = contact;
        data["phone"] = phone;
        data["email"] = email;
        DbManager::instance().addSupplier(data);
        suppliers = DbManager::instance().getSuppliers();
        for (const auto &s : suppliers) {
            if (s["name"].toString() == supplierName && s["street"].toString() == supplierStreet) {
                supplierId = s["id"].toInt();
                break;
            }
        }
    }

    // --- Materiały ---
    QVector<QMap<QString, QVariant>> items;
    for (int i = 0; i < materialsTable->rowCount(); ++i) {
        QMap<QString, QVariant> item;
        QComboBox *mat = qobject_cast<QComboBox*>(materialsTable->cellWidget(i, 1));
        QComboBox *width = qobject_cast<QComboBox*>(materialsTable->cellWidget(i, 2));
        QComboBox *length = qobject_cast<QComboBox*>(materialsTable->cellWidget(i, 3));
        QComboBox *qty = qobject_cast<QComboBox*>(materialsTable->cellWidget(i, 4));
        QString matName = mat ? mat->currentText() : "";
        int materialId = -1;
        QVector<QMap<QString, QVariant>> mats = DbManager::instance().getMaterialsCatalog();
        for (const auto &m : mats) {
            if (m["name"].toString() == matName) {
                materialId = m["id"].toInt();
                break;
            }
        }
        if (materialId > 0) {
            item["material_id"] = materialId;
        } else if (!matName.isEmpty()) {
            // Dodaj materiał do katalogu i pobierz nowe ID
            QMap<QString, QVariant> matData;
            matData["name"] = matName;
            matData["width"] = width ? width->currentText() : "";
            matData["length"] = length ? length->currentText() : "";
            matData["unit"] = "mb";
            DbManager::instance().addMaterial(matData);
            QVector<QMap<QString, QVariant>> mats2 = DbManager::instance().getMaterialsCatalog();
            for (const auto &m2 : mats2) {
                if (m2["name"].toString() == matName) {
                    item["material_id"] = m2["id"].toInt();
                    break;
                }
            }
        } else {
            item["material_id"] = QVariant(); // fallback, nie powinno się zdarzyć
        }
        item["material_name"] = matName;
        item["width"] = width ? width->currentText() : "";
        item["length"] = length ? length->currentText() : "";
        item["quantity"] = qty ? qty->currentText() : "";
        items.append(item);
    }

    QMap<QString, QVariant> orderData;
    orderData["order_number"] = orderNumber;
    orderData["order_date"] = orderDate;
    orderData["delivery_date"] = deliveryDate;
    orderData["notes"] = notes;
    orderData["supplier_id"] = supplierId > 0 ? supplierId : QVariant();
    orderData["delivery_company"] = deliveryCompany;
    orderData["delivery_street"] = deliveryStreet;
    orderData["delivery_postal_code"] = deliveryPostal;
    orderData["delivery_city"] = deliveryCity;
    orderData["delivery_country"] = deliveryCountry;

    bool dbOk = false;
    if (loadedOrderId == -1) {
        dbOk = DbManager::instance().addMaterialsOrder(orderData, items);
    } else {
        dbOk = DbManager::instance().updateMaterialsOrder(loadedOrderId, orderData, items);
    }
    if (!dbOk) {
        QMessageBox::critical(this, "Błąd bazy danych", "Nie udało się zapisać zamówienia do bazy danych!");
        return;
    }
    qDebug() << "[DEBUG] Zamówienie materiałów zapisane do bazy.";

    // --- PDF ---
    qDebug() << "[DEBUG] Generowanie HTML do PDF";
    QString materialsRows;
    for (int i = 0; i < materialsTable->rowCount(); ++i) {
        QString nr = materialsTable->item(i, 0) ? materialsTable->item(i, 0)->text() : "";
        QComboBox *mat = qobject_cast<QComboBox*>(materialsTable->cellWidget(i, 1));
        QComboBox *width = qobject_cast<QComboBox*>(materialsTable->cellWidget(i, 2));
        QComboBox *length = qobject_cast<QComboBox*>(materialsTable->cellWidget(i, 3));
        QComboBox *qty = qobject_cast<QComboBox*>(materialsTable->cellWidget(i, 4));
        qDebug() << QString("[DEBUG] PDF row %1: mat='%2', width='%3', length='%4', qty='%5'")
                    .arg(i)
                    .arg(mat ? mat->currentText() : "NULL")
                    .arg(width ? width->currentText() : "NULL")
                    .arg(length ? length->currentText() : "NULL")
                    .arg(qty ? qty->currentText() : "NULL");
        materialsRows += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>")
            .arg(nr)
            .arg(mat ? mat->currentText() : "")
            .arg(width ? width->currentText() : "")
            .arg(length ? length->currentText() : "")
            .arg(qty ? qty->currentText() : "");
    }
    // Sprawdzenie kluczowych danych przed generowaniem PDF
    if (orderNumber.isEmpty() || supplierName.isEmpty() || materialsRows.isEmpty()) {
        qCritical() << "[ERROR] Brak wymaganych danych do wygenerowania PDF!";
        QMessageBox::critical(this, "Błąd PDF", "Brak wymaganych danych do wygenerowania PDF!");
        return;
    }
    QString html = QString(R"(
        <h2 style='text-align:center;'>Zamówienie materiałów nr %1<br><span style='font-size:14px;'>Materials Order No. %1</span></h2>
        <p><b>Data zamówienia/Order date:</b> %2</p>
        <h3>Dostawca / Supplier</h3>
        <table>
        <tr><td><b>Nazwa/Name:</b></td><td>%3</td></tr>
        <tr><td><b>Ulica i nr/Street and No.:</b></td><td>%4</td></tr>
        <tr><td><b>Miasto/City:</b></td><td>%5</td></tr>
        <tr><td><b>Kod pocztowy/Postal code:</b></td><td>%6</td></tr>
        <tr><td><b>Kraj/Country:</b></td><td>%7</td></tr>
        <tr><td><b>Osoba kontaktowa/Contact person:</b></td><td>%8</td></tr>
        <tr><td><b>Telefon/Phone:</b></td><td>%9</td></tr>
        <tr><td><b>E-mail:</b></td><td>%10</td></tr>
        </table>
        <h3>Adres dostawy / Delivery address</h3>
        <table>
        <tr><td><b>Firma/Company:</b></td><td>%11</td></tr>
        <tr><td><b>Ulica i nr/Street and No.:</b></td><td>%12</td></tr>
        <tr><td><b>Miasto/City:</b></td><td>%13</td></tr>
        <tr><td><b>Kod pocztowy/Postal code:</b></td><td>%14</td></tr>
        <tr><td><b>Kraj/Country:</b></td><td>%15</td></tr>
        <tr><td><b>NIP:</b></td><td>%16</td></tr>
        </table>
        <h3>Materiały / Materials</h3>
        <table border='1' cellspacing='0' cellpadding='3' style='width:100%;'>
        <tr>
            <th style='width:4%;'>Nr/No.</th>
            <th style='width:32%;'>Materiał/Material</th>
            <th style='width:18%;'>Szerokość [mm]<br>Width [mm]</th>
            <th style='width:18%;'>Długość [mb]<br>Length [m]</th>
            <th style='width:18%;'>Ilość rolek<br>Quantity (rolls)</th>
        </tr>
        %17
        </table>
        <p><b>Termin dostawy/Delivery date:</b> %18</p>
        <p><b>Uwagi/Notes:</b> %19</p>
    )")
        .arg(orderNumber)
        .arg(orderDate)
        .arg(supplierName)
        .arg(supplierStreet)
        .arg(supplierCity)
        .arg(supplierPostal)
        .arg(supplierCountry)
        .arg(contact)
        .arg(phone)
        .arg(email)
        .arg(deliveryCompany)
        .arg(deliveryStreet)
        .arg(deliveryCity)
        .arg(deliveryPostal)
        .arg(deliveryCountry)
        .arg(deliveryNipEdit->text())
        .arg(materialsRows)
        .arg(deliveryDate)
        .arg(notes);
    qDebug() << "[DEBUG] HTML do PDF, długość:" << html.length();
    qDebug() << "[DEBUG] Fragment HTML do PDF:" << html.left(300);

    // --- WALIDACJA ŚCIEŻKI I NAZWY PLIKU ---
    // Pobierz katalog z ustawień aplikacji
    QSettings settings;
    QString pdfDir = settings.value("pdf/materialsOrderDir", "").toString();
    
    // Jeśli katalog nie jest ustawiony, użyj katalogu tymczasowego systemu
    if (pdfDir.isEmpty()) {
        pdfDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        qWarning() << "[WARNING] Nie ustawiono katalogu dla zamówień materiałów PDF. Używam katalogu tymczasowego:" << pdfDir;
        QMessageBox::warning(this, "Uwaga", "Nie ustawiono katalogu dla zapisywania zamówień materiałów PDF.\nPlik zostanie zapisany w katalogu tymczasowym.\n\nAby to zmienić, przejdź do Ustawienia -> Drukarki i katalogi -> Katalog zamówień materiałów PDF.");
    }
    
    QDir dir(pdfDir);
    if (!dir.exists()) {
        bool mkOk = dir.mkpath(".");
        qDebug() << "[DEBUG] Wynik mkpath (" << pdfDir << "):" << mkOk;
        if (!mkOk) {
            qCritical() << "[ERROR] Nie udało się utworzyć katalogu docelowego PDF:" << pdfDir;
            QMessageBox::critical(this, "Błąd zapisu PDF", QString("Nie udało się utworzyć katalogu docelowego: %1").arg(pdfDir));
            return;
        }
    }
    
    QString pdfPath = QDir::toNativeSeparators(pdfDir + "/" + orderNumber + ".pdf");
    qDebug() << "[DEBUG] Ścieżka do PDF:" << pdfPath;
    QFileInfo pdfInfo(pdfPath);
    if (pdfInfo.fileName().contains(QRegularExpression("[\\/:*?\"<>|]"))) {
        qCritical() << "[ERROR] Nazwa pliku PDF zawiera niedozwolone znaki:" << pdfInfo.fileName();
        QMessageBox::critical(this, "Błąd nazwy pliku", "Nazwa pliku PDF zawiera niedozwolone znaki: " + pdfInfo.fileName());
        return;
    }
    QFileInfo dirInfo(pdfDir);
    if (!dirInfo.isReadable() || !dirInfo.isWritable()) {
        qCritical() << "[ERROR] Brak uprawnień do katalogu C:/Temp";
        QMessageBox::critical(this, "Błąd uprawnień", "Brak uprawnień do katalogu C:/Temp");
        return;
    }
    QFile testFile(pdfPath);
    bool openOk = testFile.open(QIODevice::WriteOnly);
    qDebug() << "[DEBUG] Wynik testFile.open(QIODevice::WriteOnly):" << openOk;
    if (!openOk) {
        qCritical() << "[ERROR] Nie można utworzyć pliku PDF do zapisu:" << pdfPath << testFile.errorString();
        QMessageBox::critical(this, "Błąd zapisu PDF", "Nie można utworzyć pliku PDF do zapisu:\n" + pdfPath + "\nSzczegóły: " + testFile.errorString());
        return;
    }
    testFile.close();
    qDebug() << "[DEBUG] Test zapisu pliku OK";

    // --- ZABEZPIECZENIE: obsługa nieoczekiwanych wyjątków ---
    bool success = true;
    try {
        qDebug() << "[DEBUG] Rozpoczynanie generowania PDF";
        QTextDocument doc;
        qDebug() << "[DEBUG] QTextDocument utworzony";
        doc.setHtml(html); // Pełny PDF na podstawie wygenerowanego HTML
        qDebug() << "[DEBUG] Ustawiono pełny HTML";
        QSizeF pageSize(595, 842); // A4 w punktach
        doc.setPageSize(pageSize);
        qDebug() << "[DEBUG] Rozmiar strony ustawiony";
        QPdfWriter writer(pdfPath);
        qDebug() << "[DEBUG] QPdfWriter utworzony";
        writer.setResolution(600); // Zwiększona rozdzielczość PDF (600 DPI)
        writer.setPageSize(QPageSize(QPageSize::A4));
        // Ustaw marginesy 5 mm z każdej strony
        writer.setPageMargins(QMarginsF(5, 5, 5, 5), QPageLayout::Millimeter);
        qDebug() << "[DEBUG] Ustawiono marginesy 5mm";
        qDebug() << "[DEBUG] Rozpoczynanie drukowania do PDF";
        QElapsedTimer timer;
        timer.start();
        doc.print(&writer);
        qint64 elapsed = timer.elapsed();
        qDebug() << "[DEBUG] doc.print() OK, czas generowania (ms):" << elapsed;
    } catch (const std::exception &e) {
        qCritical() << "[ERROR] Wyjątek podczas zapisu PDF:" << e.what();
        QMessageBox::critical(this, "Błąd PDF", "Wystąpił błąd podczas zapisu PDF: " + QString::fromUtf8(e.what()));
        success = false;
    } catch (...) {
        qCritical() << "[ERROR] Nieznany wyjątek podczas zapisu PDF";
        QMessageBox::critical(this, "Błąd PDF", "Wystąpił nieznany błąd podczas zapisu PDF.");
        success = false;
    }

    if (success) {
        QMessageBox::information(this, "Sukces", "Zamówienie zostało zapisane do pliku PDF:\n" + pdfPath);
        qDebug() << "[DEBUG] PDF zapisany do" << pdfPath;
        
        // Pobierz adres e-mail dostawcy z pola edycyjnego
        QString supplierEmail = supplierEmailEdit->text().trimmed();
        
        // Pokaż okno dialogowe z opcjami wysyłki e-mail
        if (QMessageBox::question(this, "Wysyłka e-mail", 
            QString("Czy chcesz wysłać zamówienie na adres e-mail dostawcy?\n\n"
            "Adres e-mail: %1\n\n"
            "Wyślij e-mail z załączonym PDF?").arg(supplierEmail.isEmpty() ? "(brak adresu e-mail)" : supplierEmail),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            
            if (supplierEmail.isEmpty()) {
                QMessageBox::warning(this, "Błąd", "Brak adresu e-mail dostawcy. Nie można wysłać zamówienia.");
                return;
            }
            
            // Pobierz nazwę dostawcy z pola edycyjnego
            QString supplierName = supplierNameEdit->text().trimmed();
            
            // Wyślij e-mail z załączonym PDF
            sendOrderByEmail(pdfPath, supplierEmail, supplierName, orderNumber);
        }
    } else {
        QMessageBox::critical(this, "Błąd", "Nie udało się zapisać zamówienia do PDF!");
    }
    
    qDebug() << "[DEBUG] handleSaveOrder() end";
}

void MaterialsOrderForm::handleEmailSent(bool success, const QString &message) {
    qDebug() << "[EmailSender] EMIT emailSent(" << success << "," << message << ")";
    if (success) {
        QMessageBox::information(this, "Sukces", "Wiadomość e-mail została wysłana pomyślnie.");
    } else {
        QMessageBox::warning(this, "Błąd wysyłki", "Nie udało się wysłać wiadomości e-mail:\n" + message);
    }
    
    // Znajdź i usuń obiekt emailSender
    EmailSender *emailSender = qobject_cast<EmailSender*>(sender());
    if (emailSender) {
        emailSender->deleteLater();
    }
}

void MaterialsOrderForm::sendOrderByEmail(const QString &pdfPath, const QString &email, const QString &supplierName, const QString &orderNumber) {
    qDebug() << "[EmailSender] === ROZPOCZYNANIE WYSYŁANIA E-MAILA ===";
    qDebug() << "[EmailSender] Do:" << email << "Nazwa:" << supplierName;
    qDebug() << "[EmailSender] Temat: \"Zamówienie materiałów nr" << orderNumber << "\"";
    qDebug() << "[EmailSender] Załącznik:" << QFileInfo(pdfPath).fileName();
    
    // Sprawdź czy plik PDF istnieje
    QFileInfo fileInfo(pdfPath);
    if (!fileInfo.exists() || !fileInfo.isReadable()) {
        QString errorMsg = QString("Nie można odczytać pliku PDF do wysyłania.\nŚcieżka: %1").arg(pdfPath);
        qCritical() << "[EmailSender] BŁĄD:" << errorMsg;
        QMessageBox::critical(this, "Błąd", errorMsg);
        return;
    }
    
    // Pobierz ustawienia e-mail z konfiguracji
    QSettings settings;
    EmailSender::EmailData emailData;
    
    // Pobierz ustawienia SMTP z tych samych kluczy co SettingsDialog
    QString smtpServer = settings.value("email/server", "termedialabels.home.pl").toString();
    int smtpPort = settings.value("email/port", 587).toInt();
    QString smtpUser = settings.value("email/user", "potwierdzenia@termedialabels.pl").toString();
    QString smtpPassword = settings.value("email/password", "").toString();
    
    // Walidacja konfiguracji
    if (smtpServer.isEmpty() || smtpPort == 0 || smtpUser.isEmpty() || smtpPassword.isEmpty()) {
        QString errorMsg = "Brak poprawnie skonfigurowanych ustawień serwera SMTP.\n\n"
                          "Upewnij się, że w ustawieniach e-mail podano:\n"
                          "- Adres serwera SMTP\n"
                          "- Numer portu\n"
                          "- Nazwę użytkownika\n"
                          "- Hasło\n\n"
                          "Przejdź do Ustawienia -> Ustawienia serwera e-mail, aby skonfigurować połączenie.";
        
        qCritical() << "[EmailSender] BŁĄD KONFIGURACJI:" << errorMsg;
        QMessageBox::warning(this, "Błąd konfiguracji", errorMsg);
        return;
    }
    
    // Konfiguracja danych e-mail
    emailData.toEmail = email;
    emailData.toName = supplierName;
    
    // Ustawienia nadawcy - upewnij się, że senderEmail nie jest pusty
    emailData.senderEmail = !smtpUser.isEmpty() ? smtpUser : "noreply@termedia.pl";
    emailData.senderName = settings.value("company/name", "Termedia").toString();
    
    // Ustawienia serwera SMTP
    emailData.smtpServer = smtpServer;
    emailData.smtpPort = smtpPort;
    emailData.smtpUser = smtpUser;
    emailData.smtpPassword = smtpPassword;
    emailData.smtpEncryption = settings.value("smtp/encryption", "TLS").toString();
    
    // Dane wiadomości
    emailData.subject = QString("Zamówienie materiałów nr %1 / Material order no. %1 / Medžiagų užsakymas nr. %1").arg(orderNumber);
    emailData.body = QString(
        "Dzień dobry,\n\n"
        "W załączniku przesyłamy zamówienie materiałów nr %1.\n\n"
        "Pozdrawiamy,\n%2\n\n"
        "---\n"
        "Hello,\n\n"
        "Please find attached the material order no. %1.\n\n"
        "Best regards,\n%2\n\n"
        "---\n"
        "Sveiki,\n\n"
        "Priedo rasite medžiagų užsakymą nr. %1.\n\n"
        "Pagarbiai,\n%2"
    ).arg(orderNumber, emailData.senderName);
    emailData.attachmentPath = pdfPath;
    emailData.attachmentName = QString("Zamowienie_%1.pdf").arg(orderNumber);
    
    // Logowanie konfiguracji
    qDebug() << "[EmailSender] SMTP Server:" << emailData.smtpServer;
    qDebug() << "[EmailSender] SMTP Port:" << emailData.smtpPort;
    qDebug() << "[EmailSender] SMTP User:" << emailData.smtpUser;
    qDebug() << "[EmailSender] Sender:" << emailData.senderName << "<" << emailData.senderEmail << ">";
    
    // Utwórz i skonfiguruj wysyłarkę e-mail
    EmailSender *emailSender = new EmailSender();
    
    // Połącz sygnał zakończenia wysyłki z obsługą odpowiedzi
    connect(emailSender, &EmailSender::emailSent, this, &MaterialsOrderForm::handleEmailSent);
    
    // Wyślij e-mail
    qDebug() << "[EmailSender] Rozpoczynanie wysyłki e-mail...";
    emailSender->sendEmail(emailData);
    
    // Wyświetl komunikat o rozpoczęciu wysyłki
    QMessageBox::information(this, "Wysyłanie e-mail", "Rozpoczęto wysyłanie wiadomości e-mail. Proszę czekać na potwierdzenie.");
}

void MaterialsOrderForm::showOrderSelectDialog() {
    QVector<QMap<QString, QVariant>> orders = DbManager::instance().getMaterialsOrdersWithDetails();
    
    // Sortowanie zamówień od najnowszego do najstarszego
    std::sort(orders.begin(), orders.end(), 
        [](const QMap<QString, QVariant> &a, const QMap<QString, QVariant> &b) {
            return a["order_date"].toDateTime() > b["order_date"].toDateTime();
        });
    
    // Tworzenie okna dialogowego
    QDialog dialog(this);
    dialog.setWindowTitle("Wybierz zamówienie do załadowania");
    dialog.setMinimumSize(1000, 600);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    
    // Tabela z zamówieniami
    QTableWidget *table = new QTableWidget(&dialog);
    table->setColumnCount(8);
    table->setHorizontalHeaderLabels({"Data", "Numer", "Dostawca", "Status", "Wartość", "Uwagi", "Dostawa", "Materiały"});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);
    
    // Wypełnianie tabeli danymi
    table->setRowCount(orders.size());
    for (int i = 0; i < orders.size(); ++i) {
        const auto &order = orders[i];
        
        QTableWidgetItem *dateItem = new QTableWidgetItem(
            order["order_date"].toDateTime().toString("dd.MM.yyyy hh:mm"));
        QTableWidgetItem *numberItem = new QTableWidgetItem(order["order_number"].toString());
        QTableWidgetItem *supplierItem = new QTableWidgetItem(order["supplier_name"].toString());
        
        QString status = order["is_done"].toBool() ? "Zrealizowane" : "W trakcie";
        QTableWidgetItem *statusItem = new QTableWidgetItem(status);
        
        QTableWidgetItem *valueItem = new QTableWidgetItem(
            QString::number(order["total_value"].toDouble(), 'f', 2) + " zł");
        QTableWidgetItem *notesItem = new QTableWidgetItem(order["notes"].toString());
        QTableWidgetItem *deliveryItem = new QTableWidgetItem(
            order["delivery_date"].toDate().toString("dd.MM.yyyy"));
            
        // Pobierz listę materiałów dla zamówienia
        QVector<QMap<QString, QVariant>> items = DbManager::instance().getMaterialsOrderItemsForOrder(order["id"].toInt());
        QStringList materialsList;
        for (const auto &item : items) {
            QString material = QString("%1 %2x%3 x%4")
                .arg(item["material_name"].toString())
                .arg(item["width"].toDouble())
                .arg(item["length"].toDouble())
                .arg(item["quantity"].toInt());
            materialsList << material;
        }
        QTableWidgetItem *materialsItem = new QTableWidgetItem(materialsList.join("\n"));
        materialsItem->setToolTip(materialsList.join("\n"));
        
        // Ustawienie wyrównania dla kolumn liczbowych
        valueItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        
        table->setItem(i, 0, dateItem);
        table->setItem(i, 1, numberItem);
        table->setItem(i, 2, supplierItem);
        table->setItem(i, 3, statusItem);
        table->setItem(i, 4, valueItem);
        table->setItem(i, 5, notesItem);
        table->setItem(i, 6, deliveryItem);
        table->setItem(i, 7, materialsItem);
        
        // Ustawienie wysokości wiersza, aby pomieścić wszystkie materiały
        table->setRowHeight(i, qMax(30, (materialsList.size() + 1) * 20));
        
        // Przechowywanie ID zamówienia w danych użytkownika
        table->item(i, 0)->setData(Qt::UserRole, order["id"]);
    }
    
    // Dopasowanie szerokości kolumn do zawartości
    table->resizeColumnsToContents();
    table->horizontalHeader()->setStretchLastSection(true);
    
    // Przyciski
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, 
        Qt::Horizontal, &dialog);
    
    mainLayout->addWidget(table);
    mainLayout->addWidget(buttonBox);
    
    // Połączenie przycisków
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    // Podwójne kliknięcie wiersza akceptuje wybór
    connect(table, &QTableWidget::itemDoubleClicked, [&dialog](QTableWidgetItem *item) {
        dialog.accept();
    });
    
    // Wyświetlenie okna dialogowego
    if (dialog.exec() == QDialog::Accepted && table->currentRow() >= 0) {
        int orderId = table->item(table->currentRow(), 0)->data(Qt::UserRole).toInt();
        // Pobranie pełnych danych zamówienia i jego pozycji
        QMap<QString, QVariant> order = DbManager::instance().getMaterialsOrderById(orderId);
        QVector<QMap<QString, QVariant>> items = DbManager::instance().getMaterialsOrderItemsForOrder(orderId);
        
        if (!order.isEmpty()) {
            // Wczytanie nowego zamówienia z danymi z bazy
            loadNewOrder();
            
            // Ustawienie danych zamówienia
            orderNumberEdit->setText(order["order_number"].toString());
            deliveryDateEdit->setDate(order["delivery_date"].toDate());
            notesEdit->setPlainText(order["notes"].toString());
            
            // Ustawienie danych dostawcy
            int supplierId = order["supplier_id"].toInt();
            if (supplierId > 0) {
                QMap<QString, QVariant> supplier = DbManager::instance().getSupplierById(supplierId);
                if (!supplier.isEmpty()) {
                    supplierNameEdit->setText(supplier["name"].toString());
                    supplierStreetEdit->setText(supplier["street"].toString());
                    supplierCityEdit->setText(supplier["city"].toString());
                    supplierPostalEdit->setText(supplier["postal_code"].toString());
                    supplierCountryEdit->setText(supplier["country"].toString());
                    supplierContactEdit->setText(supplier["contact_person"].toString());
                    supplierPhoneEdit->setText(supplier["phone"].toString());
                    supplierEmailEdit->setText(supplier["email"].toString());
                }
            }
            
            // Ustawienie adresu dostawy
            deliveryCompanyEdit->setText(order["delivery_company"].toString());
            deliveryStreetEdit2->setText(order["delivery_street"].toString());
            deliveryPostalEdit2->setText(order["delivery_postal_code"].toString());
            deliveryCityEdit2->setText(order["delivery_city"].toString());
            deliveryCountryEdit2->setText(order["delivery_country"].toString());
            QString nip = order["delivery_nip"].toString();
            if (nip.isEmpty()) nip = "PL6381016402";
            deliveryNipEdit->setText(nip);
            
            // Dodanie pozycji zamówienia do tabeli
            materialsTable->setRowCount(0); // Wyczyść istniejące wiersze
            for (const auto &item : items) {
                addMaterialRow();
                int row = materialsTable->rowCount() - 1;
                
                // Pobierz wskaźniki do komórek w nowym wierszu
                QComboBox *materialCombo = qobject_cast<QComboBox*>(materialsTable->cellWidget(row, 1));
                QComboBox *widthCombo = qobject_cast<QComboBox*>(materialsTable->cellWidget(row, 2));
                QComboBox *lengthCombo = qobject_cast<QComboBox*>(materialsTable->cellWidget(row, 3));
                QComboBox *qtyCombo = qobject_cast<QComboBox*>(materialsTable->cellWidget(row, 4));
                
                // Ustaw wartości w komórkach
                if (materialCombo) materialCombo->setCurrentText(item["material_name"].toString());
                if (widthCombo) widthCombo->setCurrentText(item["width"].toString());
                if (lengthCombo) lengthCombo->setCurrentText(item["length"].toString());
                if (qtyCombo) qtyCombo->setCurrentText(item["quantity"].toString());
            }
        }
    }
}


void MaterialsOrderForm::setViewOnly(bool viewOnly) {
    m_viewOnly = viewOnly;
    // Pola dostawcy
    supplierNameEdit->setReadOnly(viewOnly);
    supplierContactEdit->setReadOnly(viewOnly);
    supplierPhoneEdit->setReadOnly(viewOnly);
    supplierEmailEdit->setReadOnly(viewOnly);
    supplierStreetEdit->setReadOnly(viewOnly);
    supplierCityEdit->setReadOnly(viewOnly);
    supplierPostalEdit->setReadOnly(viewOnly);
    supplierCountryEdit->setReadOnly(viewOnly);
    btnSelectSupplier->setEnabled(!viewOnly);
    btnSaveSupplier->setEnabled(!viewOnly);
    // Pola adresu dostawy
    deliveryCompanyEdit->setReadOnly(viewOnly);
    deliveryStreetEdit2->setReadOnly(viewOnly);
    deliveryCityEdit2->setReadOnly(viewOnly);
    deliveryPostalEdit2->setReadOnly(viewOnly);
    deliveryCountryEdit2->setReadOnly(viewOnly);
    deliveryNipEdit->setReadOnly(viewOnly);
    btnSelectDelivery->setEnabled(!viewOnly);
    btnSaveDelivery->setEnabled(!viewOnly);
    // Materiały - tabela
    materialsTable->setEditTriggers(viewOnly ? QAbstractItemView::NoEditTriggers : QAbstractItemView::AllEditTriggers);
    btnAddMaterial->setEnabled(!viewOnly);
    // Data i uwagi
    deliveryDateEdit->setReadOnly(viewOnly);
    notesEdit->setReadOnly(viewOnly);
    // Przycisk zapisu i ładowania
    btnSaveOrder->setEnabled(!viewOnly);
    btnLoadOrder->setEnabled(!viewOnly);
    // Dodatkowo: wyłącz wszystkie comboboxy w tabeli jeśli viewOnly
    for (int row = 0; row < materialsTable->rowCount(); ++row) {
        for (int col = 1; col <= 4; ++col) { // kolumny z QComboBox
            QWidget *w = materialsTable->cellWidget(row, col);
            if (auto cb = qobject_cast<QComboBox*>(w)) {
                cb->setEnabled(!viewOnly);
            }
        }
    }
}

void MaterialsOrderForm::loadNewOrder() {
    clearForm();
}

void MaterialsOrderForm::loadOrderFromDb(int orderId) {
    if (orderId <= 0) {
        qWarning() << "Nieprawidłowe ID zamówienia:" << orderId;
        return;
    }
    
    // Pobierz dane zamówienia z bazy
    QMap<QString, QVariant> order = DbManager::instance().getMaterialsOrderById(orderId);
    if (order.isEmpty()) {
        qWarning() << "Nie znaleziono zamówienia o ID:" << orderId;
        return;
    }
    
    // Pobierz pozycje zamówienia
    QVector<QMap<QString, QVariant>> items = DbManager::instance().getMaterialsOrderItemsForOrder(orderId);
    
    // Wyczyść formularz przed załadowaniem nowych danych
    clearForm();
    
    // Ustaw podstawowe dane zamówienia
    orderNumberEdit->setText(order["order_number"].toString());
    deliveryDateEdit->setDate(order["delivery_date"].toDate());
    notesEdit->setPlainText(order["notes"].toString());
    
    // Ustaw dane dostawcy
    int supplierId = order["supplier_id"].toInt();
    if (supplierId > 0) {
        QMap<QString, QVariant> supplier = DbManager::instance().getSupplierById(supplierId);
        if (!supplier.isEmpty()) {
            supplierNameEdit->setText(supplier["name"].toString());
            supplierStreetEdit->setText(supplier["street"].toString());
            supplierCityEdit->setText(supplier["city"].toString());
            supplierPostalEdit->setText(supplier["postal_code"].toString());
            supplierCountryEdit->setText(supplier["country"].toString());
            supplierContactEdit->setText(supplier["contact_person"].toString());
            supplierPhoneEdit->setText(supplier["phone"].toString());
            supplierEmailEdit->setText(supplier["email"].toString());
        }
    }
    
    // Ustaw adres dostawy
    deliveryCompanyEdit->setText(order["delivery_company"].toString());
    deliveryStreetEdit2->setText(order["delivery_street"].toString());
    deliveryPostalEdit2->setText(order["delivery_postal_code"].toString());
    deliveryCityEdit2->setText(order["delivery_city"].toString());
    deliveryCountryEdit2->setText(order["delivery_country"].toString());
    deliveryNipEdit->setText(order["delivery_nip"].toString());
    
    // Dodaj pozycje zamówienia do tabeli
    for (const auto &item : items) {
        addMaterialRow();
        int row = materialsTable->rowCount() - 1;
        
        // Pobierz wskaźniki do komórek w nowym wierszu
        QComboBox *materialCombo = qobject_cast<QComboBox*>(materialsTable->cellWidget(row, 1));
        QComboBox *widthCombo = qobject_cast<QComboBox*>(materialsTable->cellWidget(row, 2));
        QComboBox *lengthCombo = qobject_cast<QComboBox*>(materialsTable->cellWidget(row, 3));
        QComboBox *qtyCombo = qobject_cast<QComboBox*>(materialsTable->cellWidget(row, 4));
        
        // Ustaw wartości w komórkach
        if (materialCombo) materialCombo->setCurrentText(item["material_name"].toString());
        if (widthCombo) widthCombo->setCurrentText(item["width"].toString());
        if (lengthCombo) lengthCombo->setCurrentText(item["length"].toString());
        if (qtyCombo) qtyCombo->setCurrentText(item["quantity"].toString());
    }
    
    // Zaktualizuj ID załadowanego zamówienia
    loadedOrderId = orderId;
    
    // Zaktualizuj tytuł formularza
    setWindowTitle(QString("Edycja zamówienia %1").arg(orderNumberEdit->text()));
}

void MaterialsOrderForm::updateMaterialCompleter() {
    // Pobierz unikalne nazwy materiałów z bazy danych
    QVector<QMap<QString, QVariant>> materials = DbManager::instance().getMaterialsCatalog();
    QStringList materialNames;
    
    for (const auto &material : materials) {
        materialNames << material["name"].toString();
    }
    
    // Zaktualizuj model z nową listą materiałów
    materialModel->setStringList(materialNames);
    
    // Jeśli to pierwsze wywołanie, utwórz nowy QCompleter
    if (!materialCompleter) {
        materialCompleter = new QCompleter(materialModel, this);
        materialCompleter->setCaseSensitivity(Qt::CaseInsensitive);
        materialCompleter->setFilterMode(Qt::MatchContains);
    }
    
    // Zaktualizuj QCompleter dla każdego wiersza w tabeli materiałów
    for (int row = 0; row < materialsTable->rowCount(); ++row) {
        QComboBox *materialCombo = qobject_cast<QComboBox*>(materialsTable->cellWidget(row, 1));
        if (materialCombo) {
            // Ustaw nowy model dla istniejącego QCompleter
            materialCombo->setCompleter(materialCompleter);
            
            // Ustaw model z listą materiałów
            materialCombo->setModel(materialModel);
            materialCombo->setEditable(true);
            materialCombo->setInsertPolicy(QComboBox::NoInsert);
        }
    }
}

void MaterialsOrderForm::clearForm() {
    // Pola dostawcy
    supplierNameEdit->clear();
    supplierStreetEdit->clear();
    supplierCityEdit->clear();
    supplierPostalEdit->clear();
    supplierCountryEdit->clear();
    supplierContactEdit->clear();
    supplierPhoneEdit->clear();
    supplierEmailEdit->clear();
    // Pola adresu dostawy
    deliveryCompanyEdit->setText("TERMEDIA Magdalena Żemła");
    deliveryStreetEdit2->setText("ul. Przemysłowa 60");
    deliveryCityEdit2->setText("Tychy");
    deliveryPostalEdit2->setText("43-110");
    deliveryCountryEdit2->setText("Polska");
    deliveryNipEdit->setText("PL6381016402");
    // Tabela materiałów
    materialsTable->setRowCount(0);
    // Data i uwagi
    deliveryDateEdit->setDate(QDate::currentDate());
    notesEdit->clear();
    // Numer zamówienia
    orderNumberEdit->setText(generateOrderNumber());
    // Reset loadedOrderId
    loadedOrderId = -1;
}
