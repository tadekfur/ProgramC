// Kolejność include'ów: najpierw QLineEdit, potem user_dialog.h, potem QShowEvent
#include <QLineEdit>
#include "user_dialog.h"
#include <QShowEvent>
// Ustaw focus na pole hasła po otwarciu okna dialogowego
void UserDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    if (passwordEdit) passwordEdit->setFocus();
}
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QIcon>
#include <QFormLayout>
#include <QMessageBox>
#include <QApplication>

UserDialog::UserDialog(QWidget *parent) : QDialog(parent) {
    setObjectName("UserDialog");
    setupUi();
    setMinimumWidth(420);
}

void UserDialog::setupUi() {
    setWindowTitle("Dane użytkownika");
    loginEdit = new QLineEdit(this);
    loginEdit->setToolTip("Unikalny login użytkownika (wymagany)");
    passwordEdit = new QLineEdit(this);
    passwordEdit->setToolTip("Hasło użytkownika (min. 4 znaki)");
    passwordEdit->setEchoMode(QLineEdit::Password);
    QToolButton *showPassBtn = new QToolButton(this);
    showPassBtn->setIcon(QIcon(":/icons/eye.png"));
    showPassBtn->setCheckable(true);
    connect(showPassBtn, &QToolButton::toggled, this, [this, showPassBtn](bool checked){
        passwordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
        showPassBtn->setIcon(QIcon(":/icons/eye.png"));
    });
    firstNameEdit = new QLineEdit(this);
    firstNameEdit->setToolTip("Imię użytkownika (wymagane)");
    lastNameEdit = new QLineEdit(this);
    lastNameEdit->setToolTip("Nazwisko użytkownika (wymagane)");
    positionEdit = new QLineEdit(this);
    positionEdit->setToolTip("Stanowisko użytkownika (opcjonalnie)");
    emailEdit = new QLineEdit(this);
    emailEdit->setToolTip("Adres e-mail (opcjonalnie, poprawny format)");
    phoneEdit = new QLineEdit(this);
    phoneEdit->setToolTip("Telefon kontaktowy (opcjonalnie)");
    roleCombo = new QComboBox(this);
    roleCombo->setToolTip("Rola użytkownika w systemie");
    roleCombo->addItems({"Admin", "Operator", "Gość"});
    okBtn = new QPushButton("OK", this);
    cancelBtn = new QPushButton("Anuluj", this);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    styleCombo = new QComboBox(this);
    styleCombo->setToolTip("Styl graficzny interfejsu użytkownika");
    styleCombo->addItems({"Fusion", "Windows", "WindowsVista"});
    connect(styleCombo, &QComboBox::currentTextChanged, this, [](const QString &style){
        qApp->setStyle(style);
    });

    QFormLayout *formLayout = new QFormLayout;
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(10);
    int fieldWidth = 340;
    loginEdit->setMaximumWidth(fieldWidth);
    passwordEdit->setMaximumWidth(fieldWidth);
    firstNameEdit->setMaximumWidth(fieldWidth);
    lastNameEdit->setMaximumWidth(fieldWidth);
    positionEdit->setMaximumWidth(fieldWidth);
    emailEdit->setMaximumWidth(fieldWidth);
    phoneEdit->setMaximumWidth(fieldWidth);
    roleCombo->setMaximumWidth(fieldWidth);
    styleCombo->setMaximumWidth(fieldWidth);
    formLayout->addRow(new QLabel("Login:"), loginEdit);
    QHBoxLayout *passLayout = new QHBoxLayout;
    passLayout->addWidget(passwordEdit);
    passLayout->addWidget(showPassBtn);
    formLayout->addRow(new QLabel("Hasło:"), passLayout);
    formLayout->addRow(new QLabel("Imię:"), firstNameEdit);
    formLayout->addRow(new QLabel("Nazwisko:"), lastNameEdit);
    formLayout->addRow(new QLabel("Stanowisko:"), positionEdit);
    formLayout->addRow(new QLabel("E-mail:"), emailEdit);
    formLayout->addRow(new QLabel("Telefon:"), phoneEdit);
    formLayout->addRow(new QLabel("Rola:"), roleCombo);
    formLayout->addRow(new QLabel("Styl graficzny:"), styleCombo);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    okBtn->setMinimumWidth(100);
    okBtn->setMaximumWidth(100);
    cancelBtn->setMinimumWidth(100);
    cancelBtn->setMaximumWidth(100);
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    btnLayout->setSpacing(12);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 18, 18, 18);
    mainLayout->setSpacing(10);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(btnLayout);
    setLayout(mainLayout);

    // Usunięto lokalny styl CSS, dialog korzysta z globalnego stylu aplikacji
}

void UserDialog::setUser(const User &user) {
    loginEdit->setText(user.login);
    passwordEdit->setText(user.password);
    firstNameEdit->setText(user.firstName);
    lastNameEdit->setText(user.lastName);
    positionEdit->setText(user.position);
    emailEdit->setText(user.email);
    phoneEdit->setText(user.phone);
    roleCombo->setCurrentIndex(static_cast<int>(user.role));
    styleCombo->setCurrentText(user.style.isEmpty() ? "Fusion" : user.style);
}

User UserDialog::getUser() const {
    User::Role role = static_cast<User::Role>(roleCombo->currentIndex());
    User user(
        loginEdit->text(),
        passwordEdit->text(),
        firstNameEdit->text(),
        lastNameEdit->text(),
        positionEdit->text(),
        emailEdit->text(),
        phoneEdit->text(),
        role
    );
    user.style = styleCombo->currentText();
    return user;
}

void UserDialog::accept() {
    QString errors;
    if (loginEdit->text().trimmed().isEmpty())
        errors += "- Login jest wymagany.\n";
    if (passwordEdit->text().length() < 4)
        errors += "- Hasło musi mieć min. 4 znaki.\n";
    if (firstNameEdit->text().trimmed().isEmpty())
        errors += "- Imię jest wymagane.\n";
    if (lastNameEdit->text().trimmed().isEmpty())
        errors += "- Nazwisko jest wymagane.\n";
    if (!emailEdit->text().isEmpty() && !QRegularExpression(R"(^[\w\.-]+@[\w\.-]+\.[a-zA-Z]{2,}$)").match(emailEdit->text()).hasMatch())
        errors += "- Nieprawidłowy e-mail.\n";
    if (!errors.isEmpty()) {
        QMessageBox::warning(this, "Błąd danych", errors);
        return;
    }
    QDialog::accept();
}
