#pragma once
#include <QString>

class DeliveryAddress {
public:
    int id;
    int clientId;
    QString name;
    QString company;
    QString street;
    QString postalCode;
    QString city;
    QString contactPerson;
    QString phone;
};
