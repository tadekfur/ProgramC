#pragma once
#include <QDialog>
#include <QMap>
#include <QVariant>
#include <QVector>

class MaterialsOrderDialog : public QDialog {
    Q_OBJECT
public:
    explicit MaterialsOrderDialog(QWidget *parent = nullptr);
    void setOrderData(const QMap<QString, QVariant> &order, const QVector<QMap<QString, QVariant>> &items);
};
