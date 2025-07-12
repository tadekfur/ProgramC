#include "delivery_address_dialog.h"
#include <QVBoxLayout>
#include <QRegularExpression>

DeliveryAddressDialog::DeliveryAddressDialog(QWidget *parent) : QDialog(parent) {
    setupUI();
}

void DeliveryAddressDialog::setupUI() {
    setWindowTitle("Dodaj adres dostawy");
    resize(500, 350);
    nameEdit = new QLineEdit(this);
    companyEdit = new QLineEdit(this);
    streetEdit = new QLineEdit(this);
    postalCodeEdit = new QLineEdit(this);
    cityEdit = new QLineEdit(this);
    contactPersonEdit = new QLineEdit(this);
    phoneEdit = new QLineEdit(this);
    errorLabel = new QLabel(this);
    errorLabel->setStyleSheet("color: red");
    errorLabel->setVisible(false);
    QFormLayout *form = new QFormLayout;
    form->addRow("Nazwa adresu:", nameEdit);
    form->addRow("Firma:", companyEdit);
    form->addRow("Ulica:", streetEdit);
    form->addRow("Kod pocztowy:", postalCodeEdit);
    form->addRow("Miasto:", cityEdit);
    form->addRow("Osoba kontaktowa:", contactPersonEdit);
    form->addRow("Telefon:", phoneEdit);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(form);
    mainLayout->addWidget(errorLabel);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DeliveryAddressDialog::validateAndAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

bool DeliveryAddressDialog::validate() {
    if (nameEdit->text().trimmed().isEmpty()) {
        errorLabel->setText("Nazwa adresu jest wymagana.");
        errorLabel->setVisible(true);
        return false;
    }
    if (companyEdit->text().trimmed().isEmpty()) {
        errorLabel->setText("Pole Firma jest wymagane.");
        errorLabel->setVisible(true);
        return false;
    }
    if (streetEdit->text().trimmed().isEmpty()) {
        errorLabel->setText("Ulica jest wymagana.");
        errorLabel->setVisible(true);
        return false;
    }
    if (postalCodeEdit->text().trimmed().isEmpty()) {
        errorLabel->setText("Kod pocztowy jest wymagany.");
        errorLabel->setVisible(true);
        return false;
    }
    if (!postalCodeEdit->text().trimmed().contains(QRegularExpression("^\\d{2}-\\d{3}$"))) {
        errorLabel->setText("Kod pocztowy powinien mieć format 00-000.");
        errorLabel->setVisible(true);
        return false;
    }
    if (cityEdit->text().trimmed().isEmpty()) {
        errorLabel->setText("Miasto jest wymagane.");
        errorLabel->setVisible(true);
        return false;
    }
    if (phoneEdit->text().trimmed().isEmpty()) {
        errorLabel->setText("Telefon jest wymagany.");
        errorLabel->setVisible(true);
        return false;
    }
    QString phone = phoneEdit->text().trimmed();
    int digitCount = phone.count(QRegularExpression("[0-9]"));
    if (digitCount < 6) {
        errorLabel->setText("Telefon musi zawierać co najmniej 6 cyfr.");
        errorLabel->setVisible(true);
        return false;
    }
    if (!QRegularExpression(R"(^[0-9 +\-]+$)").match(phoneEdit->text().trimmed()).hasMatch()) {
        errorLabel->setText("Telefon może zawierać tylko cyfry, spacje, myślniki i plusy.");
        errorLabel->setVisible(true);
        return false;
    }
    errorLabel->setVisible(false);
    return true;
}

void DeliveryAddressDialog::validateAndAccept() {
    if (validate()) {
        accept();
    }
}

QMap<QString, QVariant> DeliveryAddressDialog::addressData() const {
    QMap<QString, QVariant> data;
    data["name"] = nameEdit->text().trimmed();
    data["company"] = companyEdit->text().trimmed();
    data["street"] = streetEdit->text().trimmed();
    data["postal_code"] = postalCodeEdit->text().trimmed();
    data["city"] = cityEdit->text().trimmed();
    data["contact_person"] = contactPersonEdit->text().trimmed();
    data["phone"] = phoneEdit->text().trimmed();
    return data;
}

void DeliveryAddressDialog::setAddressData(const QMap<QString, QVariant> &data) {
    nameEdit->setText(data.value("name").toString());
    companyEdit->setText(data.value("company").toString());
    streetEdit->setText(data.value("street").toString());
    postalCodeEdit->setText(data.value("postal_code").toString());
    cityEdit->setText(data.value("city").toString());
    contactPersonEdit->setText(data.value("contact_person").toString());
    phoneEdit->setText(data.value("phone").toString());
}
