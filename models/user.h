#pragma once
#include <QString>
#include <QJsonObject>

class User {
public:
    enum class Role {
        Admin,
        Operator,
        Guest
    };

    User(const QString &login = "", const QString &password = "", const QString &firstName = "", const QString &lastName = "", const QString &position = "", const QString &email = "", const QString &phone = "", Role role = Role::Operator)
        : login(login), password(password), firstName(firstName), lastName(lastName), position(position), email(email), phone(phone), role(role) {}

    QString login;
    QString password;
    QString firstName;
    QString lastName;
    QString position;
    QString email;
    QString phone;
    Role role;

    QString confirmationPrinter;
    QString productionPrinter;
    QString confirmationDir;
    QString productionDir;

    QString smtpServer;
    QString smtpPort;
    QString smtpUser;
    QString smtpPassword;
    QString dbHost;
    QString dbName;
    QString dbUser;
    QString dbPassword;

    QString style;

    QJsonObject toJson() const;
    static User fromJson(const QJsonObject& obj);

    QString getDisplayName() const { return firstName + " " + lastName; }
};
