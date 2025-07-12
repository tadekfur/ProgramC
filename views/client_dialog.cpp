#include "client_dialog.h"
#include "../db/dbmanager.h"
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QMessageBox>

ClientDialog::ClientDialog(QWidget *parent) : QDialog(parent), m_clientId(-1) {
    setupUI();
}

void ClientDialog::setupUI() {
    setWindowTitle("Dane klienta");
    nameEdit = new QLineEdit(this);
    nipEdit = new QLineEdit(this);
    cityEdit = new QLineEdit(this);
    phoneEdit = new QLineEdit(this);
    emailEdit = new QLineEdit(this);
    errorLabel = new QLabel(this);
    errorLabel->setStyleSheet("color: red");
    errorLabel->setVisible(false);
    QFormLayout *form = new QFormLayout;
    form->addRow("Nazwa:", nameEdit);
    form->addRow("NIP:", nipEdit);
    form->addRow("Miasto:", cityEdit);
    form->addRow("Telefon:", phoneEdit);
    form->addRow("E-mail:", emailEdit);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(form);
    mainLayout->addWidget(errorLabel);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        if (validate()) accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ClientDialog::setClientData(const QMap<QString, QVariant> &data) {
    nameEdit->setText(data.value("name").toString());
    nipEdit->setText(data.value("nip").toString());
    cityEdit->setText(data.value("city").toString());
    phoneEdit->setText(data.value("phone").toString());
    emailEdit->setText(data.value("email").toString());
}

QMap<QString, QVariant> ClientDialog::clientData() const {
    QMap<QString, QVariant> data;
    data["name"] = nameEdit->text();
    data["nip"] = nipEdit->text();
    data["city"] = cityEdit->text();
    data["phone"] = phoneEdit->text();
    data["email"] = emailEdit->text();
    return data;
}

void ClientDialog::setEditMode(int clientId) {
    m_clientId = clientId;
}

bool ClientDialog::validate() {
    if (nameEdit->text().isEmpty() || nipEdit->text().isEmpty()) {
        errorLabel->setText("Nazwa i NIP są wymagane.");
        errorLabel->setVisible(true);
        return false;
    }
    
    // Prosta walidacja NIP
    QRegularExpression reNip("^[0-9]{10}$");
    if (!reNip.match(nipEdit->text()).hasMatch()) {
        errorLabel->setText("Nieprawidłowy NIP (10 cyfr).");
        errorLabel->setVisible(true);
        return false;
    }
    
    // Sprawdzenie duplikatów NIP (tylko dla nowych klientów lub gdy NIP został zmieniony)
    DbManager& dbManager = DbManager::instance();
    int existingClientId = dbManager.findClientByNip(nipEdit->text());
    
    if (existingClientId != -1 && existingClientId != m_clientId) {
        // Znaleziono klienta z tym NIP i to nie jest ten sam klient (w przypadku edycji)
        int reply = QMessageBox::question(
            this,
            "Duplikat NIP",
            QString("Klient z numerem NIP %1 już istnieje w bazie danych.\n\n"
                   "Czy chcesz kontynuować mimo to?").arg(nipEdit->text()),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (reply != QMessageBox::Yes) {
            errorLabel->setText("Klient z tym numerem NIP już istnieje.");
            errorLabel->setVisible(true);
            return false;
        }
    }
    
    errorLabel->setVisible(false);
    return true;
}
