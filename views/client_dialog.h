#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>

class ClientDialog : public QDialog {
    Q_OBJECT
public:
    explicit ClientDialog(QWidget *parent = nullptr);
    void setClientData(const QMap<QString, QVariant> &data);
    QMap<QString, QVariant> clientData() const;
    void setEditMode(int clientId); // Ustawia tryb edycji dla istniejącego klienta

private:
    QLineEdit *nameEdit;
    QLineEdit *nipEdit;
    QLineEdit *cityEdit;
    QLineEdit *phoneEdit;
    QLineEdit *emailEdit;
    QDialogButtonBox *buttonBox;
    QLabel *errorLabel;
    int m_clientId; // ID klienta w trybie edycji, -1 dla nowego klienta
    void setupUI();
    bool validate();
};
