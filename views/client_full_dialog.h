#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMap>
#include <QVariant>
#include <QSqlError>
#include "views/delivery_address_dialog.h"

class ClientFullDialog : public QDialog {
    Q_OBJECT
signals:
    void clientAdded(); // Sygnał do odświeżania widoku klientów po dodaniu
public:
    enum Mode { Add, Edit };
    explicit ClientFullDialog(QWidget *parent = nullptr, Mode mode = Add);
    ~ClientFullDialog();
    void setClientData(const QMap<QString, QVariant> &data);
    QMap<QString, QVariant> clientData() const;
    QList<QMap<QString, QVariant>> deliveryAddresses() const;
    void setDeliveryAddresses(const QList<QMap<QString, QVariant>> &addresses);
    void setMode(Mode mode);

    // Utility: oczyszcza NIP z myślników, spacji i innych znaków, zostawia tylko cyfry
    static QString cleanNip(const QString &nip);

private slots:
    void addDeliveryAddressRow();
    void removeSelectedAddressRow();
    void generateClientNumber();
    void validateAndAccept();
    void editSelectedAddressRow(); // Slot do edycji adresu
    void addDefaultDeliveryAddress(); // Dodanie domyślnego adresu
    void onGusClicked();

private:
    int clientId = 0; // Przechowuj id klienta do edycji
    QLineEdit *clientNumberEdit;
    QLineEdit *nameEdit;
    QLineEdit *shortNameEdit;
    QLineEdit *contactPersonEdit;
    QLineEdit *phoneEdit;
    QLineEdit *emailEdit;
    QLineEdit *streetEdit;
    QLineEdit *postalCodeEdit;
    QLineEdit *cityEdit;
    QLineEdit *nipEdit;
    QPushButton *btnGus;
    QComboBox *countryCombo;
    QDialogButtonBox *buttonBox;
    QLabel *errorLabel;
    QTableWidget *addressesTable;
    QPushButton *btnAddAddress;
    QPushButton *btnRemoveAddress;
    QPushButton *btnEditAddress; // Dodano przycisk Edytuj
    void setupUI();
    bool validate();
    void loadNextClientNumber(); // teraz korzysta z getNextUniqueClientNumber
    void addAddressToTable(const QMap<QString, QVariant> &address);
    Mode dialogMode = Add;
};
