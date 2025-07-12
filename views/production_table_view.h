#pragma once
#include <QTableView>
#include <QStandardItemModel>
#include <QWidget>
#include <QVariantMap>
#include <QList>

class ProductionTableView : public QTableView {
    Q_OBJECT
public:
    explicit ProductionTableView(QWidget* parent = nullptr);
    void setOrderItems(const QList<QVariantMap>& items);
private:
    QStandardItemModel* model;
};
