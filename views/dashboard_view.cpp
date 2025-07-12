#include "dashboard_view.h"
#include "order_card.h"
#include "db/dbmanager.h"
#include "models/order.h"
#include "models/client.h"
#include "views/order_dialog.h"
#include <QScrollArea>
#include <QDate>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QMap>
#include <QVariant>
#include <QMetaObject>
#include <QTimer>
#include <QGroupBox>
#include <QSqlQuery>

class DayBox : public QFrame {
public:
    DayBox(const QString &dayName, const QDate &date, const QColor &headerColor, QWidget *parent = nullptr)
        : QFrame(parent), m_date(date) {
        setAcceptDrops(true);
        setFrameShape(QFrame::StyledPanel);
        setStyleSheet("QFrame { background: #fff; border-radius: 10px; border: 1px solid #d1d5db; }");
        QVBoxLayout *layout = new QVBoxLayout(this);
        QLabel *header = new QLabel(dayName + "\n" + date.toString("dd.MM.yyyy"), this);
        header->setAlignment(Qt::AlignCenter);
        header->setStyleSheet(QString("background:%1; font-weight: bold; font-size: 15px; padding: 4px 0; border-top-left-radius:10px; border-top-right-radius:10px;").arg(headerColor.name()));
        layout->addWidget(header);
        m_cardsLayout = new QVBoxLayout();
        layout->addLayout(m_cardsLayout);
        layout->addStretch();
        setMinimumWidth(120);
    }
    void addOrderCard(OrderCard* card) {
        m_cardsLayout->addWidget(card);
    }
    QDate date() const { return m_date; }
protected:
    void dragEnterEvent(QDragEnterEvent* event) override {
        if (event->mimeData()->hasFormat("application/x-order-id"))
            event->acceptProposedAction();
    }
    void dropEvent(QDropEvent* event) override {
        if (event->mimeData()->hasFormat("application/x-order-id")) {
            int orderId = event->mimeData()->data("application/x-order-id").toInt();
            QDate newDate = m_date;
            auto& dbm = DbManager::instance();
            if (!dbm.updateOrderDeliveryDate(orderId, newDate)) {
                QMessageBox::critical(this, "Błąd", "Nie udało się zaktualizować daty zamówienia w bazie.");
            }
            // Odśwież dashboard
            QWidget* w = this;
            while (w && !w->inherits("DashboardView")) w = w->parentWidget();
            if (w) w->update();
            QMetaObject::invokeMethod(w, "refreshDashboard", Qt::QueuedConnection);
            event->acceptProposedAction();
        }
    }
private:
    QDate m_date;
    QVBoxLayout* m_cardsLayout;
};

QWidget* createDashboardGrid(QWidget *parent = nullptr, QMap<QDate, DayBox*>* dayBoxMap = nullptr) {
    QWidget *container = new QWidget(parent);
    QVBoxLayout *mainLayout = new QVBoxLayout(container);
    QDate today = QDate::currentDate();
    QDate monday = today.addDays(-(today.dayOfWeek()-1));
    QList<QDate> weekStarts;
    weekStarts << monday.addDays(-7) << monday << monday.addDays(7) << monday.addDays(14);
    QStringList dayNames = {"Pon", "Wt", "Śr", "Czw", "Pt"};
    QList<QColor> weekColors = { QColor("#f0f9ff"), QColor("#fef9c3"), QColor("#f3e8ff"), QColor("#f0fdf4") };
    for (int row = 0; row < 4; ++row) {
        QHBoxLayout *rowLayout = new QHBoxLayout;
        for (int col = 0; col < 5; ++col) {
            QDate day = weekStarts[row].addDays(col);
            DayBox *box = new DayBox(dayNames[col], day, weekColors[row]);
            if (dayBoxMap) (*dayBoxMap)[day] = box;
            rowLayout->addWidget(box, 1);
        }
        mainLayout->addLayout(rowLayout);
    }
    mainLayout->addStretch();
    return container;
}

// Przeciążenie dla kompatybilności
QWidget* createDashboardGrid(QWidget *parent) {
    return createDashboardGrid(parent, nullptr);
}

DashboardView::DashboardView(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    QMap<QDate, DayBox*> dayBoxMap;
    QWidget* grid = createDashboardGrid(nullptr, &dayBoxMap);
    // DEBUG: wypisz wszystkie klucze dayBoxMap
    qDebug() << "[DASHBOARD] Klucze dayBoxMap (dni na tablicy):";
    QList<QDate> dayBoxDates = dayBoxMap.keys();
    for (const QDate& d : dayBoxDates) {
        qDebug() << "  " << d.toString("yyyy-MM-dd");
    }
    // Pobierz zamówienia i klientów
    auto& dbm = DbManager::instance();
    QVector<QMap<QString, QVariant>> orders = dbm.getOrders();
    QVector<QMap<QString, QVariant>> clients = dbm.getClients();
    QMap<int, QMap<QString, QVariant>> clientMap;
    for (const auto& c : clients) clientMap[c["id"].toInt()] = c;
    // DEBUG: wypisz daty deliveryDate wszystkich zamówień
    qDebug() << "[DASHBOARD] Daty deliveryDate zamówień:";
    for (const auto& o : orders) {
        QDate d = o["delivery_date"].toDate();
        qDebug() << "  ID:" << o["id"].toInt() << ", nr:" << o["order_number"].toString() << ", delivery:" << d.toString("yyyy-MM-dd");
    }
    // Dla każdego zamówienia utwórz OrderCard i wstaw do odpowiedniego DayBox
    for (const auto& o : orders) {
        Order order;
        order.id = o["id"].toInt();
        order.orderNumber = o["order_number"].toString();
        order.orderDate = o["order_date"].toDate();
        order.deliveryDate = o["delivery_date"].toDate();
        order.clientId = o["client_id"].toInt();
        order.notes = o["notes"].toString();
        order.paymentTerm = o["payment_term"].toString();
        order.status = o.contains("status") ? static_cast<Order::Status>(o["status"].toInt()) : Order::Przyjete;
        
        // Pomiń zamówienia ze statusem "Zrealizowane" - nie pokazuj ich na dashboard
        if (order.status == Order::Zrealizowane) {
            continue;
        }
        
        if (clientMap.contains(order.clientId)) {
            const auto& c = clientMap[order.clientId];
            order.client.name = c["name"].toString();
            order.client.shortName = c["short_name"].toString();
        }
        // DEBUG: sprawdź czy data jest w dayBoxMap
        if (dayBoxMap.contains(order.deliveryDate)) {
            qDebug() << "[DASHBOARD][MATCH] Zamówienie ID:" << order.id << ", nr:" << order.orderNumber << ", delivery:" << order.deliveryDate.toString("yyyy-MM-dd") << "-> DayBox OK";
            OrderCard* card = new OrderCard(order, this);
            // Połącz sygnał zmiany statusu
            connect(card, &OrderCard::orderStatusChanged, this, &DashboardView::onOrderStatusChanged);
            // Połącz sygnał dwukliku z podglądem zamówienia
            connect(card, &OrderCard::orderDoubleClicked, this, &DashboardView::previewOrder);
            dayBoxMap[order.deliveryDate]->addOrderCard(card);
        } else {
            // Wypisz różnicę z każdą datą DayBoxa
            qDebug() << "[DASHBOARD][NOMATCH] Zamówienie ID:" << order.id << ", nr:" << order.orderNumber << ", delivery:" << order.deliveryDate.toString("yyyy-MM-dd") << "nie pasuje do żadnego DayBoxa!";
            for (const QDate& d : dayBoxDates) {
                int diff = d.daysTo(order.deliveryDate);
                qDebug() << "    DayBox:" << d.toString("yyyy-MM-dd") << ", różnica dni:" << diff;
            }
        }
    }
    scroll->setWidget(grid);
    layout->addWidget(scroll);
    setLayout(layout);
}

void DashboardView::refreshDashboard() {
    // Usuń obecny widget i stwórz nowy
    QLayout* oldLayout = layout();
    if (oldLayout) {
        QLayoutItem* item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete oldLayout;
    }
    
    // Odtwórz całą strukturę dashboard
    QVBoxLayout *layout = new QVBoxLayout(this);
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    QMap<QDate, DayBox*> dayBoxMap;
    QWidget* grid = createDashboardGrid(nullptr, &dayBoxMap);
    
    // Pobierz zamówienia i klientów
    auto& dbm = DbManager::instance();
    QVector<QMap<QString, QVariant>> orders = dbm.getOrders();
    QVector<QMap<QString, QVariant>> clients = dbm.getClients();
    QMap<int, QMap<QString, QVariant>> clientMap;
    for (const auto& c : clients) clientMap[c["id"].toInt()] = c;
    
    // Dla każdego zamówienia utwórz OrderCard i wstaw do odpowiedniego DayBox
    for (const auto& o : orders) {
        Order order;
        order.id = o["id"].toInt();
        order.orderNumber = o["order_number"].toString();
        order.orderDate = o["order_date"].toDate();
        order.deliveryDate = o["delivery_date"].toDate();
        order.clientId = o["client_id"].toInt();
        order.notes = o["notes"].toString();
        order.paymentTerm = o["payment_term"].toString();
        order.status = o.contains("status") ? static_cast<Order::Status>(o["status"].toInt()) : Order::Przyjete;
        
        // Pomiń zamówienia ze statusem "Zrealizowane" - nie pokazuj ich na dashboard
        if (order.status == Order::Zrealizowane) {
            continue;
        }
        
        if (clientMap.contains(order.clientId)) {
            const auto& c = clientMap[order.clientId];
            order.client.name = c["name"].toString();
            order.client.shortName = c["short_name"].toString();
        }
        
        if (dayBoxMap.contains(order.deliveryDate)) {
            OrderCard* card = new OrderCard(order, this);
            // Połącz sygnał zmiany statusu
            connect(card, &OrderCard::orderStatusChanged, this, &DashboardView::onOrderStatusChanged);
            // Połącz sygnał dwukliku z podglądem zamówienia
            connect(card, &OrderCard::orderDoubleClicked, this, &DashboardView::previewOrder);
            dayBoxMap[order.deliveryDate]->addOrderCard(card);
        }
    }
    scroll->setWidget(grid);
    layout->addWidget(scroll);
    setLayout(layout);
}

void DashboardView::onOrderStatusChanged(int orderId, Order::Status newStatus) {
    qDebug() << "[DashboardView] onOrderStatusChanged: orderId=" << orderId << ", newStatus=" << newStatus;
    
    try {
        // Tylko emituj sygnał dalej dla innych komponentów (np. tabeli zamówień)
        // Karta sama się ukrywa i usuwa, więc nie ma potrzeby refresh'u dashboard
        emit orderStatusChanged(orderId, newStatus);
        qDebug() << "[DashboardView] onOrderStatusChanged zakończony pomyślnie";
    } catch (const std::exception& e) {
        qDebug() << "[DashboardView] Wyjątek w onOrderStatusChanged:" << e.what();
    } catch (...) {
        qDebug() << "[DashboardView] Nieznany wyjątek w onOrderStatusChanged";
    }
}

void DashboardView::previewOrder(int orderId) {
    auto& db = DbManager::instance();
    auto orders = db.getOrders();
    QMap<QString, QVariant> order;
    for (const auto& o : orders) {
        if (o["id"].toInt() == orderId) {
            order = o;
            break;
        }
    }
    if (order.isEmpty()) {
        QMessageBox::warning(this, "Podgląd zamówienia", "Nie znaleziono zamówienia o podanym ID.");
        return;
    }
    int clientId = order["client_id"].toInt();
    QVector<QMap<QString, QVariant>> clients = db.getClients();
    QMap<QString, QVariant> client;
    for (const auto &c : clients) {
        if (c["id"].toInt() == clientId) {
            client = c;
            break;
        }
    }
    QDialog dlg(this);
    dlg.setWindowTitle("Podgląd zamówienia " + order["order_number"].toString());
    dlg.resize(900, 700);
    QVBoxLayout *mainLayout = new QVBoxLayout(&dlg);
    // --- Sekcja: Dane zamawiającego ---
    QGroupBox *groupboxOrdering = new QGroupBox("Dane zamawiającego");
    QGridLayout *grid = new QGridLayout(groupboxOrdering);
    QFont labelFont("Segoe UI", 12, QFont::Bold);
    QFont valueFont("Segoe UI", 12);
    QStringList labels = {"Firma:", "Nr klienta:", "Osoba kontaktowa:", "Telefon:", "E-mail:", "Ulica i nr:", "Kod pocztowy:", "Miasto:", "NIP:"};
    QStringList values = {
        client.value("name").toString(),
        client.value("client_number").toString(),
        client.value("contact_person").toString(),
        client.value("phone").toString(),
        client.value("email").toString(),
        client.value("street").toString(),
        client.value("postal_code").toString(),
        client.value("city").toString(),
        client.value("nip").toString()
    };
    for (int i = 0; i < labels.size(); ++i) {
        QLabel *label = new QLabel(labels[i]);
        label->setFont(labelFont);
        grid->addWidget(label, i, 0);
        QLabel *value = new QLabel(values[i]);
        value->setFont(valueFont);
        grid->addWidget(value, i, 1);
    }
    mainLayout->addWidget(groupboxOrdering);
    // --- Sekcja: Dane produkcji ---
    QGroupBox *groupboxProduction = new QGroupBox("Dane produkcji");
    QVBoxLayout *productionLayout = new QVBoxLayout(groupboxProduction);
    QSqlQuery q(db.database());
    q.prepare("SELECT material, width, height, ordered_quantity, quantity_type, roll_length, core, price, price_type FROM order_items WHERE order_id=?");
    q.addBindValue(order["id"].toInt());
    bool anyRow = false;
    if (q.exec()) {
        int idx = 1;
        while (q.next()) {
            QString width = q.value(1).toString().trimmed();
            if (width.isEmpty()) continue;
            QString prod = QString("%1. Szer: %2 mm, Wys: %3 mm, Materiał: %4, Ilość: %5 %6, Nawój: %7, Rdzeń: %8, cena: %9 %10")
                .arg(idx++)
                .arg(width)
                .arg(q.value(2).toString())
                .arg(q.value(0).toString())
                .arg(q.value(3).toString())
                .arg(q.value(4).toString())
                .arg(q.value(5).toString())
                .arg(q.value(6).toString())
                .arg(q.value(7).toString())
                .arg(q.value(8).toString());
            QLabel *prodLabel = new QLabel(prod);
            prodLabel->setFont(valueFont);
            productionLayout->addWidget(prodLabel);
            anyRow = true;
        }
    }
    if (!anyRow) {
        QLabel *noProd = new QLabel("Brak pozycji z wypełnioną szerokością");
        noProd->setFont(valueFont);
        productionLayout->addWidget(noProd);
    }
    mainLayout->addWidget(groupboxProduction);
    // --- Sekcja: Adres dostawy ---
    QGroupBox *groupboxAddress = new QGroupBox("Adres dostawy");
    QGridLayout *gridAddress = new QGridLayout(groupboxAddress);
    QStringList addressLabels = {"Nazwa firmy:", "Ulica i nr:", "Kod pocztowy:", "Miejscowość:", "Osoba kontaktowa:", "Telefon:"};
    QStringList addressValues = {
        order.value("delivery_company").toString(),
        order.value("delivery_street").toString(),
        order.value("delivery_postal_code").toString(),
        order.value("delivery_city").toString(),
        order.value("delivery_contact_person").toString(),
        order.value("delivery_phone").toString()
    };
    for (int i = 0; i < addressLabels.size(); ++i) {
        QLabel *label = new QLabel(addressLabels[i]);
        label->setFont(labelFont);
        gridAddress->addWidget(label, i, 0);
        QLabel *value = new QLabel(addressValues[i]);
        value->setFont(valueFont);
        gridAddress->addWidget(value, i, 1);
    }
    mainLayout->addWidget(groupboxAddress);
    // --- Sekcja: Uwagi ---
    QGroupBox *groupboxNotes = new QGroupBox("Uwagi do zamówienia");
    QVBoxLayout *notesLayout = new QVBoxLayout(groupboxNotes);
    QLabel *notes = new QLabel(order["notes"].toString());
    notes->setFont(valueFont);
    notesLayout->addWidget(notes);
    mainLayout->addWidget(groupboxNotes);
    // Przycisk zamknięcia
    QPushButton *btnClose = new QPushButton("Zamknij");
    btnClose->setFont(labelFont);
    connect(btnClose, &QPushButton::clicked, &dlg, &QDialog::accept);
    mainLayout->addWidget(btnClose);
    dlg.exec();
}
