#include "page_manager.h"
#include "models/user.h"
#include "views/dashboard_view.h"
#include "views/orders_db_view.h"
#include "views/clients_db_view.h"
#include "views/production_summary_view.h"
#include "views/new_order_view.h"
#include "views/pdf_viewer.h"
#include "views/materials_pdf_page.h"
#include "views/materials_orders_db_view.h"
#include "db/dbmanager.h"
#include <QScrollArea>
#include <QMessageBox>

PageManager::PageManager(QWidget *parent)
    : QObject(parent), m_parent(parent), m_pages(nullptr), m_currentUser(nullptr) {
}

QStackedWidget* PageManager::createPages() {
    if (m_pages) return m_pages; // Nie twórz ponownie!
    m_pages = new QStackedWidget(m_parent);
    m_pages->setStyleSheet("QStackedWidget { background: #fff; border-radius: 18px; }");

    setupDashboardPage();     // index 0 - Dashboard
    setupNewOrderPage();      // index 1 - NewOrder  
    setupOrdersDbPage();      // index 2 - OrdersDb
    setupClientsDbPage();     // index 3 - ClientsDb
    setupProductionSummaryPage(); // index 4 - ProductionSummary
    setupOrdersPdfPage();     // index 5 - OrdersPdf
    setupMaterialsPdfPage();  // index 6 - MaterialsPdf
    setupMaterialsOrdersDbPage(); // index 7 - Baza zamówień materiałów
    setupConnections();

    return m_pages;
}

void PageManager::setupDashboardPage() {
    m_dashboardPage = new QWidget;
    auto *dashboardLayout = new QVBoxLayout(m_dashboardPage);
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    
    m_dashboardView = new DashboardView();
    
    // Połącz sygnał zmiany statusu z odświeżaniem tabeli zamówień
    connect(m_dashboardView, &DashboardView::orderStatusChanged, this, [this](int orderId, Order::Status newStatus) {
        // Odśwież tabelę zamówień gdy status zostanie zmieniony w OrderCard
        if (m_ordersDbView) {
            m_ordersDbView->refreshOrders();
        }
    });
    
    scroll->setWidget(m_dashboardView);
    dashboardLayout->addWidget(scroll);
    
    m_pages->addWidget(m_dashboardPage); // index 0
}

void PageManager::setupOrdersDbPage() {
    // OrdersDbView zostanie utworzony w setCurrentUser() gdy będziemy mieć usera
    m_ordersDbView = nullptr;
    
    // Dodajemy placeholder widget na razie
    auto *placeholder = new QWidget();
    m_pages->addWidget(placeholder); // index 2 - OrdersDb
}

void PageManager::setupClientsDbPage() {
    m_clientsDbView = new ClientsDbView(m_parent);
    m_pages->addWidget(m_clientsDbView); // index 3 - ClientsDb
}

void PageManager::setupProductionSummaryPage() {
    m_productionSummaryView = new ProductionSummaryView(m_parent);
    m_pages->addWidget(m_productionSummaryView); // index 4 - ProductionSummary
}

void PageManager::setupNewOrderPage() {
    m_newOrderView = new NewOrderView(m_parent);
    m_pages->addWidget(m_newOrderView); // index 1 - NewOrder
}

void PageManager::setupMaterialsOrdersDbPage() {
    m_materialsOrdersDbView = new MaterialsOrdersDbView(m_parent);
    m_pages->addWidget(m_materialsOrdersDbView); // index 7

    // Podpięcie sygnałów do obsługi edycji/podglądu/drukowania
    // Zakładamy, że MaterialsPdfPage zawiera MaterialsOrderForm jako jedyne dziecko
    auto dbView = qobject_cast<MaterialsOrdersDbView*>(m_materialsOrdersDbView);
    if (!dbView) return;
    connect(dbView, &MaterialsOrdersDbView::requestEditOrder, this, [this](int orderId) {
        auto materialsPdfPage = dynamic_cast<MaterialsPdfPage*>(m_materialsPdfView);
        if (!materialsPdfPage) return;
        auto form = materialsPdfPage->findChild<MaterialsOrderForm*>();
        if (form) {
            form->setViewOnly(false);
            form->loadOrderFromDb(orderId);
        }
        switchToPage(MaterialsPdf);
    });
    // --- Dodaj obsługę czyszczenia formularza przy przejściu do nowego zamówienia materiałów ---
    connect(dbView, &MaterialsOrdersDbView::orderSelected, this, [this]() {
        auto materialsPdfPage = dynamic_cast<MaterialsPdfPage*>(m_materialsPdfView);
        if (!materialsPdfPage) return;
        if (materialsPdfPage->form) {
            materialsPdfPage->form->clearForm();
        }
        switchToPage(MaterialsPdf);
    });
    connect(dbView, &MaterialsOrdersDbView::requestDuplicateOrder, this, [this](int orderId) {
        auto materialsPdfPage = dynamic_cast<MaterialsPdfPage*>(m_materialsPdfView);
        if (!materialsPdfPage) return;
        auto form = materialsPdfPage->findChild<MaterialsOrderForm*>();
        if (form) {
            form->setViewOnly(false);
            form->loadOrderFromDb(orderId);
            form->getOrderNumberEdit()->setText(form->generateOrderNumber()); // Nowy numer zamówienia
            form->setLoadedOrderId(-1); // Tryb nowego zamówienia
        }
        switchToPage(MaterialsPdf);
    });
}

void PageManager::setupConnections() {
    // Połączenie z sygnałem orderAdded z DbManager dla odświeżania wszystkich widoków
    auto& dbManager = DbManager::instance();
    connect(&dbManager, &DbManager::orderAdded, this, &PageManager::refreshAllViews);
    qDebug() << "PageManager: Połączono z sygnałem orderAdded";
}

void PageManager::setupOrdersDbConnections() {
    if (m_ordersDbView && m_newOrderView) {
        connect(m_ordersDbView, &OrdersDbView::requestShowNewOrder, this, [this]() {
            m_newOrderView->resetForm();
            m_newOrderView->loadOrderData(QMap<QString, QVariant>(), true);
            switchToPage(NewOrder);
        });

        connect(m_ordersDbView, &OrdersDbView::requestEditOrder, this, [this](const QMap<QString, QVariant>& data) {
            m_newOrderView->loadOrderData(data, true);
            switchToPage(NewOrder);
        });

        connect(m_ordersDbView, &OrdersDbView::requestDuplicateOrder, this, [this](const QMap<QString, QVariant>& data) {
            m_newOrderView->loadOrderData(data, false);
            switchToPage(NewOrder);
        });
        
        // Połącz sygnał orderSaved - po zapisaniu przełącz na dashboard
        connect(m_newOrderView, &NewOrderView::orderSaved, this, [this]() {
            // Odśwież listę zamówień
            if (m_ordersDbView) {
                m_ordersDbView->refreshOrders();
            }
            // Odśwież dashboard
            if (m_dashboardView) {
                m_dashboardView->refreshDashboard();
            }
            switchToPage(Dashboard);
        });
    }
}

void PageManager::setCurrentUser(const User &user) {
    m_currentUser = &user;
    
    // Utwórz OrdersDbView teraz gdy mamy usera
    if (!m_ordersDbView && m_pages) {
        try {
            m_ordersDbView = new OrdersDbView(user, m_parent);
            // Zastąp placeholder na index 2 (OrdersDb)
            QWidget *oldWidget = m_pages->widget(2);
            m_pages->removeWidget(oldWidget);
            oldWidget->deleteLater();
            m_pages->insertWidget(2, m_ordersDbView);
            
            // Połącz sygnały teraz
            setupOrdersDbConnections();
        } catch (const std::exception &e) {
            QMessageBox::critical(m_parent, "PageManager Error", 
                QString("Failed to create OrdersDbView: %1").arg(e.what()));
        } catch (...) {
            QMessageBox::critical(m_parent, "PageManager Error", 
                "Unknown error creating OrdersDbView");
        }
    }
}

void PageManager::switchToPage(PageIndex page) {
    if (m_pages) {
        m_pages->setCurrentIndex(static_cast<int>(page));
        // --- Usunięto automatyczne czyszczenie formularza materiałów ---
        emit pageChanged(page);
    }
}

void PageManager::setupOrdersPdfPage() {
    // Utwórz widok PDF dla zamówień (z podzielonym interfejsem na zamówienia i potwierdzenia)
    m_ordersPdfView = new PdfViewer(PdfViewer::OrdersMode, m_parent);
    m_pages->addWidget(m_ordersPdfView); // index 5 - OrdersPdf
}

void PageManager::setupMaterialsPdfPage() {
    // Nowy widok: formularz zamówienia materiałów produkcyjnych
    m_materialsPdfView = new MaterialsPdfPage(m_parent);
    m_pages->addWidget(m_materialsPdfView); // index 6 - MaterialsPdf
}

void PageManager::refreshAllViews() {
    qDebug() << "=== PageManager::refreshAllViews() wywołane ===";
    
    // Odśwież dashboard
    if (m_dashboardView) {
        qDebug() << "Odświeżanie Dashboard View";
        // Dashboard zazwyczaj ma metodę refresh lub update
        QMetaObject::invokeMethod(m_dashboardView, "refresh", Qt::QueuedConnection);
    }
    
    // Odśwież widok bazy zamówień
    if (m_ordersDbView) {
        qDebug() << "Odświeżanie Orders DB View";
        QMetaObject::invokeMethod(m_ordersDbView, "refreshData", Qt::QueuedConnection);
    }
    
    // Odśwież widok klientów
    if (m_clientsDbView) {
        qDebug() << "Odświeżanie Clients DB View";
        QMetaObject::invokeMethod(m_clientsDbView, "refreshClients", Qt::QueuedConnection);
    }
    
    // ProductionSummaryView już jest podłączony bezpośrednio do orderAdded,
    // ale możemy go też odświeżyć tutaj dla pewności
    if (m_productionSummaryView) {
        qDebug() << "Odświeżanie Production Summary View";
        QMetaObject::invokeMethod(m_productionSummaryView, "generateReport", Qt::QueuedConnection);
    }
    
    qDebug() << "=== PageManager: Wszystkie widoki odświeżone ===";
}
