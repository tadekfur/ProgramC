#pragma once

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QItemSelection>
#include <QLineEdit>
#include <QComboBox>
#include <QMap>
#include <QStandardItemModel>
#include "models/user.h"

class OrdersDbView : public QWidget {
    Q_OBJECT
public:
    explicit OrdersDbView(const User& user, QWidget *parent = nullptr);
    void setCurrentUser(const User& user) { currentUser = user; }

public slots:
    void refreshOrders();
    void addOrder();
    void editOrder();
    void deleteOrder();
    void duplicateOrder();
    void previewOrder(); // Dodano deklarację metody previewOrder()
    void openPrintDialog();
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private:
    QTableView *tableView;
    QPushButton *btnAdd;
    QPushButton *btnEdit;
    QPushButton *btnDelete;
    QPushButton *btnDuplicate;
    QPushButton *btnPreview;
    QPushButton *btnPrint;
    QVBoxLayout *mainLayout;
    QHBoxLayout *btnLayout;
    QLineEdit *searchEdit;
    QComboBox *searchTypeCombo; // Dodano wskaźnik do QComboBox dla wyboru typu wyszukiwania
    int selectedOrderId = -1;
    QMap<int, QMap<QString, QVariant>> orderIdToData;
    QList<int> highlightedRows; // Lista podświetlonych wierszy
    QMetaObject::Connection highlightConnection; // Połączenie dla obsługi wyczyszczenia podświetleń
    void setupUI();
    void loadOrders();
    void highlightSearchResults(QStandardItemModel* model, const QString& filter, int searchType); // Nowa metoda do podświetlania wyników
    User currentUser;

signals:
    void requestShowNewOrder();
    void requestEditOrder(const QMap<QString, QVariant>& orderData);
    void requestDuplicateOrder(const QMap<QString, QVariant>& orderData);
};
