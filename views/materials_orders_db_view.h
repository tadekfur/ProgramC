#pragma once
#include <QWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QItemSelection>

class MaterialsOrdersDbView : public QWidget {
    Q_OBJECT
public:
    explicit MaterialsOrdersDbView(QWidget *parent = nullptr);

signals:
    void orderSelected(int orderId); // Sygnał, że wybrano zamówienie (do podglądu)
    void requestEditOrder(int orderId);
    void requestPrintOrder(int orderId);
    void requestDuplicateOrder(int orderId); // Nowy sygnał do duplikowania zamówienia

public slots:
    void refreshOrders();

private slots:
    void handleEdit();
    void handleDelete();
    void handlePrint();
    void handleView();
    void handleDoneChanged(QStandardItem *item);
    void handleDuplicate(); // Deklaracja slotu do obsługi przycisku 'Wystaw jako nowe'
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private:
    QTableView *ordersTable;
    QStandardItemModel *ordersModel;
    int selectedOrderId = -1;
    QPushButton *btnRefresh;
    QPushButton *btnEdit;
    QPushButton *btnDelete;
    QPushButton *btnPrint;
    QPushButton *btnView;
    QPushButton *btnDuplicate; // Nowy przycisk
    QVBoxLayout *mainLayout;
    void setupUi();
    void loadOrders();
};
