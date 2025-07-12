#pragma once
#include <QString>
#include <QDate>
#include <QVector>
#include "orderitem.h"
#include "client.h"

class Order {
public:
    enum Status {
        Przyjete = 0,
        Produkcja = 1,
        Gotowe = 2,
        Zrealizowane = 3
    };
    int id;
    QString orderNumber;
    QDate orderDate;
    QDate deliveryDate;
    int clientId;
    QString notes;
    QString paymentTerm;
    QString deliveryCompany;
    QString deliveryStreet;
    QString deliveryPostalCode;
    QString deliveryCity;
    QString deliveryContactPerson;
    QString deliveryPhone;
    QString userLogin;
    QVector<OrderItem> items;
    Client client; // Dodane pole na dane klienta
    Status status = Przyjete;

    static QString statusToString(Status s) {
        switch (s) {
        case Przyjete: return "Przyjęte do realizacji";
        case Produkcja: return "Produkcja";
        case Gotowe: return "Gotowe do wysyłki";
        case Zrealizowane: return "Zrealizowane";
        }
        return "Nieznany";
    }
};
