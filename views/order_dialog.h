#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QDateEdit>
#include <QPushButton>

class OrderDialog : public QDialog {
    Q_OBJECT
public:
    explicit OrderDialog(QWidget *parent = nullptr);
    void setOrderData(const QMap<QString, QVariant> &data);
    QMap<QString, QVariant> orderData() const;

private:
    QLineEdit *orderNumberEdit;
    QDateEdit *orderDateEdit;
    QDateEdit *deliveryDateEdit;
    QLineEdit *clientIdEdit;
    QLineEdit *notesEdit;
    QDialogButtonBox *buttonBox;
    QLabel *errorLabel;
    QLineEdit *deliveryCompanyEdit;
    QLineEdit *deliveryStreetEdit;
    QLineEdit *deliveryPostalCodeEdit;
    QLineEdit *deliveryCityEdit;
    QLineEdit *deliveryContactPersonEdit;
    QLineEdit *deliveryPhoneEdit;
    QLineEdit *paymentTermEdit;
    QPushButton *btnSelectClient;
    void setupUI();
    bool validate();

private slots:
    void selectClientFromDb();
};
