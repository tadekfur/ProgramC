#pragma once
#include <QString>
#include <QDate>
#include <QVector>
#include "supplier.h"
#include "material.h"

class MaterialsOrderItem {
public:
    int id;
    int orderId;
    int materialId;
    QString materialName;
    QString width;
    QString length;
    QString quantity;
};

class MaterialsOrder {
public:
    int id;
    QString orderNumber;
    QDate orderDate;
    QDate deliveryDate;
    QString notes;
    int supplierId;
    QString deliveryCompany;
    QString deliveryStreet;
    QString deliveryPostalCode;
    QString deliveryCity;
    QString deliveryCountry;
    QVector<MaterialsOrderItem> items;
    Supplier supplier;
};
