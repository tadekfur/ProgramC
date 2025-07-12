#pragma once
#include <QString>
#include <QVector>
#include "deliveryaddress.h"

class Client {
public:
    int id;
    QString clientNumber;
    QString name;
    QString shortName;
    QString contactPerson;
    QString phone;
    QString email;
    QString street;
    QString postalCode;
    QString city;
    QString nip;
    QString deliveryCompany;
    QString deliveryStreet;
    QString deliveryPostalCode;
    QString deliveryCity;
    QVector<DeliveryAddress> deliveryAddresses;
};
