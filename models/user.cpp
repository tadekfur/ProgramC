#include "user.h"
#include <QJsonObject>

QJsonObject User::toJson() const {
    QJsonObject obj;
    obj["login"] = login;
    obj["password"] = password;
    obj["firstName"] = firstName;
    obj["lastName"] = lastName;
    obj["position"] = position;
    obj["email"] = email;
    obj["phone"] = phone;
    obj["role"] = static_cast<int>(role);
    obj["confirmationPrinter"] = confirmationPrinter;
    obj["productionPrinter"] = productionPrinter;
    obj["confirmationDir"] = confirmationDir;
    obj["productionDir"] = productionDir;
    obj["smtpServer"] = smtpServer;
    obj["smtpPort"] = smtpPort;
    obj["smtpUser"] = smtpUser;
    obj["smtpPassword"] = smtpPassword;
    obj["dbHost"] = dbHost;
    obj["dbName"] = dbName;
    obj["dbUser"] = dbUser;
    obj["dbPassword"] = dbPassword;
    obj["style"] = style;
    return obj;
}

User User::fromJson(const QJsonObject& obj) {
    User user;
    user.login = obj["login"].toString();
    user.password = obj["password"].toString();
    user.firstName = obj["firstName"].toString();
    user.lastName = obj["lastName"].toString();
    user.position = obj["position"].toString();
    user.email = obj["email"].toString();
    user.phone = obj["phone"].toString();
    user.role = static_cast<Role>(obj["role"].toInt());
    user.confirmationPrinter = obj["confirmationPrinter"].toString();
    user.productionPrinter = obj["productionPrinter"].toString();
    user.confirmationDir = obj["confirmationDir"].toString();
    user.productionDir = obj["productionDir"].toString();
    user.smtpServer = obj["smtpServer"].toString();
    user.smtpPort = obj["smtpPort"].toString();
    user.smtpUser = obj["smtpUser"].toString();
    user.smtpPassword = obj["smtpPassword"].toString();
    user.dbHost = obj["dbHost"].toString();
    user.dbName = obj["dbName"].toString();
    user.dbUser = obj["dbUser"].toString();
    user.dbPassword = obj["dbPassword"].toString();
    user.style = obj["style"].toString();
    return user;
}
