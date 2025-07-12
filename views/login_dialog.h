#pragma once
#include <QDialog>
#include <QVector>
#include "models/user.h"

class QComboBox;
class QLineEdit;
class QLabel;

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(const QVector<User> &users, QWidget *parent = nullptr);
    User selectedUser() const;
    QString password() const;
    void resetPasswordField(); // Nowa metoda do czyszczenia pola hasła
    void refreshUserList(); // Odświeź listę użytkowników

signals:
    void loginSuccess(const User &user);

private:
    QVector<User> m_users;
    QComboBox *userCombo;
    QLineEdit *passwordEdit;
    QLabel *roleLabel;
    int selectedIndex;
};
