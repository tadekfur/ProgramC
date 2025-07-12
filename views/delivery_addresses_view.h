#pragma once
#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QItemSelection>
#include <QLineEdit>

class DeliveryAddressesView : public QWidget {
    Q_OBJECT
public:
    explicit DeliveryAddressesView(int clientId, QWidget *parent = nullptr);
    ~DeliveryAddressesView();

signals:
    void addressSelected(const QMap<QString, QVariant> &addressData);

private slots:
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void emitSelectedAddress();

private:
    QTableView *tableView;
    QVBoxLayout *mainLayout;
    int selectedAddressId = -1;
    void loadAddresses(int clientId);
};
