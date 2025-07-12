#pragma once
#include <QDialog>
#include "models/user.h"

class QLineEdit;
class QComboBox;
class QPushButton;

class UserDialog : public QDialog {
    Q_OBJECT
public:
    explicit UserDialog(QWidget *parent = nullptr);
    void setUser(const User &user);
    User getUser() const;

protected:
    void accept() override;
    void showEvent(QShowEvent *event) override;

private:
    QLineEdit *loginEdit;
    QLineEdit *passwordEdit;
    QLineEdit *firstNameEdit;
    QLineEdit *lastNameEdit;
    QLineEdit *positionEdit;
    QLineEdit *emailEdit;
    QLineEdit *phoneEdit;
    QComboBox *roleCombo;
    QComboBox *styleCombo;
    QPushButton *okBtn;
    QPushButton *cancelBtn;

    void setupUi();
    void applyStyle();
};
