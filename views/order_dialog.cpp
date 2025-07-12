#include "order_dialog.h"
#include "clients_db_view.h"
#include "client_select_dialog.h"
#include <QVBoxLayout>
OrderDialog::OrderDialog(QWidget *parent) : QDialog(parent) {
    setupUI();
}
void OrderDialog::setupUI() {
    setWindowTitle("Dane zamówienia");
    orderNumberEdit = new QLineEdit(this);
    orderDateEdit = new QDateEdit(QDate::currentDate(), this);
    deliveryDateEdit = new QDateEdit(QDate::currentDate(), this);
    clientIdEdit = new QLineEdit(this);
    notesEdit = new QLineEdit(this);
    deliveryCompanyEdit = new QLineEdit(this);
    deliveryStreetEdit = new QLineEdit(this);
    deliveryPostalCodeEdit = new QLineEdit(this);
    deliveryCityEdit = new QLineEdit(this);
    deliveryContactPersonEdit = new QLineEdit(this);
    deliveryPhoneEdit = new QLineEdit(this);
    paymentTermEdit = new QLineEdit(this);
    btnSelectClient = new QPushButton("Wstaw z bazy", this);
    errorLabel = new QLabel(this);
    errorLabel->setStyleSheet("color: red");
    errorLabel->setVisible(false);
    QFormLayout *form = new QFormLayout;
    form->addRow("Numer zamówienia:", orderNumberEdit);
    form->addRow("Data zamówienia:", orderDateEdit);
    form->addRow("Termin dostawy:", deliveryDateEdit);
    QHBoxLayout *clientLayout = new QHBoxLayout;
    clientLayout->addWidget(clientIdEdit);
    clientLayout->addWidget(btnSelectClient);
    form->addRow("ID klienta:", clientLayout);
    form->addRow("Notatki:", notesEdit);
    form->addRow("Firma dostawy:", deliveryCompanyEdit);
    form->addRow("Ulica dostawy:", deliveryStreetEdit);
    form->addRow("Kod pocztowy dostawy:", deliveryPostalCodeEdit);
    form->addRow("Miasto dostawy:", deliveryCityEdit);
    form->addRow("Osoba kontaktowa dostawy:", deliveryContactPersonEdit);
    form->addRow("Telefon dostawy:", deliveryPhoneEdit);
    form->addRow("Termin płatności:", paymentTermEdit);
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
    connect(btnSelectClient, &QPushButton::clicked, this, &OrderDialog::selectClientFromDb);
}

void OrderDialog::setOrderData(const QMap<QString, QVariant> &data) {
    orderNumberEdit->setText(data.value("order_number").toString());
    orderDateEdit->setDate(data.value("order_date").toDate());
    deliveryDateEdit->setDate(data.value("delivery_date").toDate());
    clientIdEdit->setText(data.value("client_id").toString());
    notesEdit->setText(data.value("notes").toString());
    deliveryCompanyEdit->setText(data.value("delivery_company").toString());
    deliveryStreetEdit->setText(data.value("delivery_street").toString());
    deliveryPostalCodeEdit->setText(data.value("delivery_postal_code").toString());
    deliveryCityEdit->setText(data.value("delivery_city").toString());
    deliveryContactPersonEdit->setText(data.value("delivery_contact_person").toString());
    deliveryPhoneEdit->setText(data.value("delivery_phone").toString());
    paymentTermEdit->setText(data.value("payment_term").toString());
}

QMap<QString, QVariant> OrderDialog::orderData() const {
    QMap<QString, QVariant> data;
    data["order_number"] = orderNumberEdit->text();
    data["order_date"] = orderDateEdit->date();
    data["delivery_date"] = deliveryDateEdit->date();
    data["client_id"] = clientIdEdit->text();
    data["notes"] = notesEdit->text();
    data["delivery_company"] = deliveryCompanyEdit->text();
    data["delivery_street"] = deliveryStreetEdit->text();
    data["delivery_postal_code"] = deliveryPostalCodeEdit->text();
    data["delivery_city"] = deliveryCityEdit->text();
    data["delivery_contact_person"] = deliveryContactPersonEdit->text();
    data["delivery_phone"] = deliveryPhoneEdit->text();
    data["payment_term"] = paymentTermEdit->text();
    return data;
}

void OrderDialog::selectClientFromDb() {
    // Otwórz dialog wyboru klienta
    ClientSelectDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QMap<QString, QVariant> clientData = dlg.selectedClient();
        clientIdEdit->setText(clientData.value("id").toString());
        // Możesz ustawić inne pola, np. nazwę, adres itp.
    }
}

bool OrderDialog::validate() {
    if (orderNumberEdit->text().isEmpty() || clientIdEdit->text().isEmpty()) {
        errorLabel->setText("Numer zamówienia i ID klienta są wymagane.");
        errorLabel->setVisible(true);
        return false;
    }
    errorLabel->setVisible(false);
    return true;
}
