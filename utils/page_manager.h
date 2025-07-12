#pragma once

#include <QObject>
#include <QStackedWidget>
#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>

class User;
class DashboardView;
class OrdersDbView;
class ClientsDbView;
class ProductionSummaryView;
class NewOrderView;

class PageManager : public QObject {
    Q_OBJECT

public:
    enum PageIndex {
        Dashboard = 0,
        NewOrder = 1,
        OrdersDb = 2,
        ClientsDb = 3,
        ProductionSummary = 4,
        OrdersPdf = 5,
        MaterialsPdf = 6,
        MaterialsOrdersDb = 7 // NOWY WIDOK
    };

    explicit PageManager(QWidget *parent = nullptr);
    
    QStackedWidget* createPages();
    void setCurrentUser(const User &user);
    void switchToPage(PageIndex page);
    
    // Gettery dla widoków
    NewOrderView* newOrderView() const { return m_newOrderView; }
    OrdersDbView* ordersDbView() const { return m_ordersDbView; }
    ClientsDbView* clientsDbView() const { return m_clientsDbView; } // Dodany getter dla widoku klientów
    QWidget* materialsOrdersDbView() const { return m_materialsOrdersDbView; } // Dodany getter dla widoku bazy zamówień materiałów
    QStackedWidget* pagesWidget() const { return m_pages; }

signals:
    void pageChanged(PageIndex page);

public slots:
    void refreshAllViews(); // Slot do odświeżania wszystkich widoków

private:
    void setupDashboardPage();
    void setupNewOrderPage();
    void setupOrdersDbPage();
    void setupClientsDbPage();
    void setupProductionSummaryPage();
    void setupOrdersPdfPage();      // Nowa metoda do inicjalizacji widoku PDF z zamówieniami
    void setupMaterialsPdfPage();   // Nowa metoda do inicjalizacji widoku PDF z materiałami
    void setupMaterialsOrdersDbPage(); // Nowa metoda do inicjalizacji widoku bazy zamówień materiałów
    void setupConnections();
    void setupOrdersDbConnections();

    QWidget *m_parent;
    QStackedWidget *m_pages;
    const User *m_currentUser;  // Zmieniono na const User*

    // Strony
    QWidget *m_dashboardPage;
    NewOrderView *m_newOrderView;
    OrdersDbView *m_ordersDbView;
    ClientsDbView *m_clientsDbView;
    ProductionSummaryView *m_productionSummaryView;
    QWidget *m_ordersPdfView;     // Nowy widok dla przeglądania PDF z zamówieniami
    QWidget *m_materialsPdfView;  // Nowy widok dla przeglądania PDF z materiałami
    QWidget *m_materialsOrdersDbView; // NOWY WIDOK

    // Widoki
    DashboardView *m_dashboardView;
};
