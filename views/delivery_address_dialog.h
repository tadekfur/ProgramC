#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>

class DeliveryAddressDialog : public QDialog {
    Q_OBJECT
public:
    explicit DeliveryAddressDialog(QWidget *parent = nullptr);
    QMap<QString, QVariant> addressData() const;
    void setAddressData(const QMap<QString, QVariant> &data);

private slots:
    void validateAndAccept();

private:
    QLineEdit *nameEdit;
    QLineEdit *companyEdit;
    QLineEdit *streetEdit;
    QLineEdit *postalCodeEdit;
    QLineEdit *cityEdit;
    QLineEdit *contactPersonEdit;
    QLineEdit *phoneEdit;
    QDialogButtonBox *buttonBox;
    QLabel *errorLabel;
    void setupUI();
    bool validate();
};
