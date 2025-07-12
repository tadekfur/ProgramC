#include "login_dialog.h"
#include "../utils/secure_user_manager.h"
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFont>
#include <QApplication>
#include <QToolButton>
#include <QProgressBar>
#include <QTimer>

LoginDialog::LoginDialog(const QVector<User> &users, QWidget *parent)
    : QDialog(parent), m_users(users), selectedIndex(-1) {
    setWindowTitle("Logowanie");
    setMinimumSize(380, 180);
    
    QFont labelFont;
    labelFont.setPointSize(16);
    userCombo = new QComboBox(this);
    for (const User &u : m_users) {
        userCombo->addItem(u.login);
    }
    userCombo->setMinimumWidth(200);
    passwordEdit = new QLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setMinimumWidth(200);
    QToolButton *showPassBtn = new QToolButton(this);
    showPassBtn->setText("🔒");
    showPassBtn->setCheckable(true);
    connect(showPassBtn, &QToolButton::toggled, this, [this, showPassBtn](bool checked){
        passwordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
        showPassBtn->setText(checked ? "🔓" : "🔒");
    });
    roleLabel = new QLabel(this);
    QPushButton *loginBtn = new QPushButton("Zaloguj", this);
    QPushButton *cancelBtn = new QPushButton("Anuluj", this);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 18, 18, 18);
    mainLayout->setSpacing(10);
    QHBoxLayout *userLayout = new QHBoxLayout;
    userLayout->setSpacing(8);
    userLayout->addWidget(new QLabel("Użytkownik:"));
    userLayout->addWidget(userCombo);
    mainLayout->addLayout(userLayout);

    QHBoxLayout *passLayout = new QHBoxLayout;
    passLayout->setSpacing(8);
    passLayout->addWidget(new QLabel("Hasło:"));
    passLayout->addWidget(passwordEdit);
    passLayout->addWidget(showPassBtn);
    mainLayout->addLayout(passLayout);

    mainLayout->addWidget(roleLabel);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(loginBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);

    setLayout(mainLayout);
    QApplication::setStyle("Fusion");

    connect(loginBtn, &QPushButton::clicked, [this]() {
        int idx = userCombo->currentIndex();
        if (idx < 0 || idx >= m_users.size()) {
            QMessageBox::warning(this, "Błąd logowania", "Nie wybrano użytkownika.");
            return;
        }
        
        QString login = m_users[idx].login;
        QString pass = passwordEdit->text();
        
        if (pass.isEmpty()) {
            QMessageBox::warning(this, "Błąd logowania", "Hasło jest wymagane.");
            return;
        }
        
        SecureUserManager& userManager = SecureUserManager::instance();
        
        // Sprawdź czy konto nie jest zablokowane
        if (userManager.isAccountLocked(login)) {
            int attempts = userManager.getFailedLoginAttempts(login);
            QMessageBox::warning(this, "Konto zablokowane", 
                QString("Konto '%1' zostało zablokowane po %2 nieudanych próbach logowania.\n"
                       "Spróbuj ponownie później lub skontaktuj się z administratorem.")
                       .arg(login).arg(attempts));
            return;
        }
        
        // Spróbuj zalogować
        if (userManager.authenticateUser(login, pass)) {
            selectedIndex = idx;
            QString roleStr;
            switch (m_users[idx].role) {
                case User::Role::Admin: roleStr = "Administrator"; break;
                case User::Role::Operator: roleStr = "Operator"; break;
                case User::Role::Guest: roleStr = "Gość"; break;
            }
            roleLabel->setText("Rola: " + roleStr);
            
            emit loginSuccess(m_users[idx]);
            accept();
        } else {
            int attempts = userManager.getFailedLoginAttempts(login);
            int maxAttempts = 5; // SecureUserManager::MAX_LOGIN_ATTEMPTS
            
            QString message = QString("Nieprawidłowe hasło dla użytkownika '%1'.\n"
                                    "Próba %2 z %3.")
                                    .arg(login).arg(attempts).arg(maxAttempts);
            
            if (attempts >= maxAttempts) {
                message += "\n\nKonto zostało zablokowane!";
            } else {
                message += QString("\nPozostało %1 prób.").arg(maxAttempts - attempts);
            }
            
            QMessageBox::warning(this, "Błąd logowania", message);
        }
    });
    connect(cancelBtn, &QPushButton::clicked, this, &LoginDialog::reject);
}

User LoginDialog::selectedUser() const {
    if (selectedIndex >= 0 && selectedIndex < m_users.size()) {
        return m_users[selectedIndex];
    }
    // Return default user if no selection
    User defaultUser;
    defaultUser.login = "Unknown";
    defaultUser.role = User::Role::Guest;
    return defaultUser;
}

QString LoginDialog::password() const {
    return passwordEdit->text();
}

void LoginDialog::resetPasswordField() {
    // Czyści pole hasła przy wylogowaniu
    if (passwordEdit) {
        passwordEdit->clear();
    }
}

void LoginDialog::refreshUserList() {
    // Odświeź listę użytkowników z SecureUserManager
    m_users = SecureUserManager::instance().users();
    
    // Wyczyść i odbuduj combo box
    userCombo->clear();
    for (const User &u : m_users) {
        userCombo->addItem(u.login);
    }
    
    // Resetuj indeks
    selectedIndex = -1;
    
    qDebug() << "User list refreshed -" << m_users.size() << "users loaded";
}
