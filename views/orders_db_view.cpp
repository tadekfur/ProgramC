#include "orders_db_view.h"
#include "db/dbmanager.h"
#include "views/order_dialog.h"
#include "views/new_order_view.h"
#include "views/print_dialog.h"
#include "models/order.h"
#include <QStandardItemModel>
#include <QMessageBox>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QDialog>
#include <QSqlQuery>
#include <QSqlError>
#include <QComboBox>
#include <QGroupBox>
#include "../utils/pdf_generator.h"
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>
#include <QTimer>
#include "models/user.h"

OrdersDbView::OrdersDbView(const User& user, QWidget *parent) : QWidget(parent), currentUser(user) {
    try {
        setupUI();
        loadOrders();
    } catch (const std::exception &e) {
        // Error in OrdersDbView construction
        QMessageBox::critical(this, "OrdersDbView Error", 
            QString("Failed to initialize OrdersDbView: %1").arg(e.what()));
    } catch (...) {
        QMessageBox::critical(this, "OrdersDbView Error", 
            "Unknown error in OrdersDbView construction");
    }
}

void OrdersDbView::setupUI() {
    searchTypeCombo = new QComboBox(this);
    searchTypeCombo->addItems({"Nr zamówienia", "Nr klienta", "Firma"});
    connect(searchTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &OrdersDbView::loadOrders);
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Szukaj...");
    connect(searchEdit, &QLineEdit::textChanged, this, &OrdersDbView::loadOrders);
    // --- Nowy układ: wszystkie przyciski i pole szukania w jednym wierszu ---
    btnAdd = new QPushButton("Dodaj zamówienie", this);
    btnEdit = new QPushButton("Edytuj", this);
    btnDelete = new QPushButton("Usuń", this);
    btnDuplicate = new QPushButton("Wystaw jako nowe", this);
    btnPreview = new QPushButton("Podgląd zamówienia", this);
    btnPrint = new QPushButton("🖨️ Drukuj", this);
    btnEdit->setEnabled(false);
    btnDelete->setEnabled(false);
    btnDuplicate->setEnabled(false);
    btnPreview->setEnabled(false);
    btnPrint->setEnabled(false);
    btnLayout = new QHBoxLayout;
    btnLayout->addWidget(btnAdd);
    btnLayout->addWidget(btnEdit);
    btnLayout->addWidget(btnDelete);
    btnLayout->addWidget(btnDuplicate);
    btnLayout->addWidget(btnPreview);
    btnLayout->addWidget(btnPrint);
    btnLayout->addSpacing(24);
    btnLayout->addWidget(new QLabel("Szukaj: ", this));
    btnLayout->addWidget(searchTypeCombo);
    btnLayout->addWidget(searchEdit);
    btnLayout->addStretch(1);
    tableView = new QTableView(this);
    mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(btnLayout);
    mainLayout->addWidget(tableView);
    setLayout(mainLayout);
    connect(btnAdd, &QPushButton::clicked, this, &OrdersDbView::addOrder);
    connect(btnEdit, &QPushButton::clicked, this, &OrdersDbView::editOrder);
    connect(btnDelete, &QPushButton::clicked, this, &OrdersDbView::deleteOrder);
    connect(btnDuplicate, &QPushButton::clicked, this, &OrdersDbView::duplicateOrder);
    connect(btnPreview, &QPushButton::clicked, this, &OrdersDbView::previewOrder);
    connect(btnPrint, &QPushButton::clicked, this, &OrdersDbView::openPrintDialog);
    // Dwuklik na wierszu tabeli otwiera podgląd zamówienia
    connect(tableView, &QTableView::doubleClicked, this, [this](const QModelIndex &index) {
        if (!index.isValid()) return;
        int row = index.row();
        QStandardItemModel *model = qobject_cast<QStandardItemModel*>(tableView->model());
        if (!model) return;
        QModelIndex idIndex = model->index(row, 0); // kolumna 0: order_number z id w UserRole+1
        int orderId = idIndex.data(Qt::UserRole + 1).toInt();
        if (orderIdToData.contains(orderId)) {
            selectedOrderId = orderId;
            previewOrder();
        }
    });
}

void OrdersDbView::loadOrders() {
    try {
        auto& db = DbManager::instance();
        
        // Check if database is available
        if (!db.isOpen()) {
            QMessageBox::warning(this, "Database Error", 
                "Database connection is not available. Some features may not work.");
            return;
        }
        
        auto orders = db.getOrders();
        QString filter = searchEdit ? searchEdit->text().trimmed() : "";
        int searchType = searchTypeCombo ? searchTypeCombo->currentIndex() : 0;
        QList<QMap<QString, QVariant>> filtered;
        orderIdToData.clear();
        QVector<QMap<QString, QVariant>> clients = db.getClients();
        QMap<int, QString> clientIdToName;
        QMap<int, QString> clientIdToNumber;
        for (const auto &c : clients) {
            clientIdToName[c["id"].toInt()] = c["name"].toString();
            clientIdToNumber[c["id"].toInt()] = c["client_number"].toString();
        }
        
        if (!filter.isEmpty()) {
            for (const auto &o : orders) {
                int cid = o["client_id"].toInt();
                bool match = false;
                
                if (searchType == 0) { // Nr zamówienia
                    match = o["order_number"].toString().contains(filter, Qt::CaseInsensitive);
                } else if (searchType == 1) { // Nr klienta
                    match = clientIdToNumber.value(cid).contains(filter, Qt::CaseInsensitive);
                } else if (searchType == 2) { // Firma
                    match = clientIdToName.value(cid).contains(filter, Qt::CaseInsensitive);
                }
                
                if (match) {
                    filtered.append(o);
                }
            }
        } else {
            filtered = orders;
        }
    // Nowy model: 9 kolumn: nr zamówienia, data zamówienia, data wysyłki, nr klienta, klient, cena, dane produkcji, uwagi, status
    auto *model = new QStandardItemModel(filtered.size(), 9, this);
    model->setHorizontalHeaderLabels({"Nr zamówienia", "Data zamówienia", "Data wysyłki", "Nr klienta", "Klient", "Cena", "Dane produkcji", "Uwagi", "Status"});
    for (int row = 0; row < filtered.size(); ++row) {
        int oid = filtered[row]["id"].toInt();
        int cid = filtered[row]["client_id"].toInt();
        // --- Numer zamówienia pogrubiony ---
        QStandardItem *orderNumItem = new QStandardItem(filtered[row]["order_number"].toString());
        QFont boldFont = orderNumItem->font();
        boldFont.setBold(true);
        orderNumItem->setFont(boldFont);
        // --- DODAJ ID jako UserRole+1 ---
        orderNumItem->setData(oid, Qt::UserRole + 1);
        model->setItem(row, 0, orderNumItem);
        // --- Data zamówienia ---
        model->setItem(row, 1, new QStandardItem(filtered[row]["order_date"].toDate().toString("yyyy-MM-dd")));
        // --- Data wysyłki ---
        model->setItem(row, 2, new QStandardItem(filtered[row]["delivery_date"].toDate().toString("yyyy-MM-dd")));
        // --- Nr klienta ---
        model->setItem(row, 3, new QStandardItem(clientIdToNumber.value(cid)));
        // --- Klient ---
        model->setItem(row, 4, new QStandardItem(clientIdToName.value(cid)));
        // --- Cena i szczegóły produkcji ---
        QString prodDetails;
        QSqlQuery q(db.database());
        q.prepare("SELECT price, price_type, material, width, height, ordered_quantity, quantity_type FROM order_items WHERE order_id=?");
        q.addBindValue(oid);
        if (q.exec()) {
            QStringList prodList;
            QString priceCellValue;
            priceCellValue = "";
            while (q.next()) {
                QString w = q.value(3).toString().trimmed();
                QString h = q.value(4).toString().trimmed();
                QString mat = q.value(2).toString().trimmed();
                QString qty = q.value(5).toString().trimmed();
                QString qtyType = q.value(6).toString();
                QString priceType = q.value(1).toString();
                QString price = q.value(0).toString();
                bool ok = false;
                int qtyInt = qty.toInt(&ok);
                // Dodajemy tylko jeśli szerokość, wysokość, materiał i ilość są niepuste i ilość > 0
                if (!w.isEmpty() && !h.isEmpty() && !mat.isEmpty() && ok && qtyInt > 0) {
                    prodList << QString("%1 %2x%3 %4 %5").arg(mat).arg(w).arg(h).arg(qty).arg(qtyType);
                        QString priceSuffix;
                    QString pt = priceType.trimmed().toLower();
                    if (pt.contains("rolk")) {
                        priceSuffix = "zł/rolkę";
                    } else {
                        priceSuffix = "zł/tyś.";
                    }
                    if (!priceCellValue.isEmpty()) {
                        priceCellValue += "\n";
                    }
                    priceCellValue += QString("%1 %2").arg(price, priceSuffix);
                }
            }
            prodDetails = prodList.join("\n"); // każda pozycja w nowej linii
            // Ustaw w kolumnie Cena tylko pierwszą cenę z sufiksem, jeśli istnieje
            if (!priceCellValue.isEmpty()) {
                model->setItem(row, 5, new QStandardItem(priceCellValue));
            } else {
                model->setItem(row, 5, new QStandardItem(""));
            }
        } else {
            model->setItem(row, 5, new QStandardItem(""));
        }
        // --- Dane produkcji (agregacja) ---
        model->setItem(row, 6, new QStandardItem(prodDetails));
        // --- Uwagi ---
        model->setItem(row, 7, new QStandardItem(filtered[row]["notes"].toString()));
        
        // --- Status zamówienia ---
        int statusValue = filtered[row].contains("status") ? filtered[row]["status"].toInt() : 0;
        Order::Status status = static_cast<Order::Status>(statusValue);
        QString statusText = QString("%1 (%2)").arg(statusValue + 1).arg(Order::statusToString(status));
        
        QStandardItem *statusItem = new QStandardItem(statusText);
        // Ustaw kolor tła w zależności od statusu
        switch (status) {
        case Order::Przyjete:
            statusItem->setBackground(QColor("#fff3cd")); // żółte tło
            statusItem->setForeground(QColor("#856404")); // ciemno-żółty tekst
            break;
        case Order::Produkcja:
            statusItem->setBackground(QColor("#d1ecf1")); // niebieskie tło
            statusItem->setForeground(QColor("#0c5460")); // ciemno-niebieski tekst
            break;
        case Order::Gotowe:
            statusItem->setBackground(QColor("#d4edda")); // zielone tło
            statusItem->setForeground(QColor("#155724")); // ciemno-zielony tekst
            break;
        case Order::Zrealizowane:
            statusItem->setBackground(QColor("#e2e3e5")); // szare tło
            statusItem->setForeground(QColor("#383d41")); // ciemno-szary tekst
            break;
        }
        model->setItem(row, 8, statusItem);
        
        orderIdToData[oid] = filtered[row];
    }
    tableView->setModel(model);
    // Sortuj po kolumnie 0 (Nr zamówienia) rosnąco
    tableView->setSortingEnabled(true);
    tableView->sortByColumn(0, Qt::AscendingOrder);
    // Kolumna 0 (Nr zamówienia) widoczna
    connect(tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &OrdersDbView::onSelectionChanged);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    tableView->horizontalHeader()->setStretchLastSection(false);
    QFont headerFont = tableView->horizontalHeader()->font();
    headerFont.setBold(true);
    tableView->horizontalHeader()->setFont(headerFont);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableView->setMinimumWidth(1100);
    tableView->setMinimumHeight(500);
    tableView->setAlternatingRowColors(true);
    tableView->setStyleSheet(
        "QTableView { font-size: 15px; } "
        "QHeaderView::section { font-size: 15px; font-weight: bold; background: #f3f6fa; color: #222; border: 1px solid #ddd; padding: 6px 8px; } "
        "QTableView::item:selected { background: #2563eb; color: #fff; } "
        "QTableView::item:focus { background: #2563eb; } "
    );
    tableView->resizeColumnsToContents();
    tableView->resizeRowsToContents();
    tableView->verticalHeader()->setVisible(false);
    
    // Podświetlenie wyników wyszukiwania
    highlightSearchResults(model, filter, searchType);
    
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Database Error", 
            QString("Error loading orders: %1").arg(e.what()));
    } catch (...) {
        QMessageBox::critical(this, "Database Error", 
            "Unknown error occurred while loading orders");
    }
}

void OrdersDbView::refreshOrders() { 
    loadOrders(); 
}
void OrdersDbView::addOrder() {
    emit requestShowNewOrder();
}

void OrdersDbView::editOrder() {
    if (selectedOrderId < 0) return;
    if (!orderIdToData.contains(selectedOrderId)) return;
    emit requestEditOrder(orderIdToData[selectedOrderId]);
}

void OrdersDbView::duplicateOrder() {
    if (selectedOrderId < 0) return;
    if (!orderIdToData.contains(selectedOrderId)) return;
    emit requestDuplicateOrder(orderIdToData[selectedOrderId]);
}

void OrdersDbView::deleteOrder() {
    if (selectedOrderId < 0) return;
    if (QMessageBox::question(this, "Usuń zamówienie", "Czy na pewno usunąć zamówienie?") == QMessageBox::Yes) {
        auto& db = DbManager::instance();
        if (db.deleteOrder(selectedOrderId)) refreshOrders();
        else QMessageBox::warning(this, "Błąd", "Nie udało się usunąć zamówienia.");
    }
}
void OrdersDbView::onSelectionChanged(const QItemSelection &selected, const QItemSelection &) {
    bool hasSel = !selected.indexes().isEmpty();
    btnEdit->setEnabled(hasSel);
    btnDelete->setEnabled(hasSel);
    btnDuplicate->setEnabled(hasSel);
    btnPreview->setEnabled(hasSel);
    btnPrint->setEnabled(hasSel);
    if (hasSel) {
        // Pobierz ID z UserRole+1 z pierwszej kolumny
        QModelIndex idx = selected.indexes().first();
        selectedOrderId = idx.sibling(idx.row(), 0).data(Qt::UserRole + 1).toInt();
    } else {
        selectedOrderId = -1;
    }
}

void OrdersDbView::previewOrder() {
    if (selectedOrderId < 0 || !orderIdToData.contains(selectedOrderId)) {
        QMessageBox::information(this, "Podgląd zamówienia", "Nie wybrano zamówienia.");
        return;
    }
    const QMap<QString, QVariant> &order = orderIdToData[selectedOrderId];
    auto& db = DbManager::instance();
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
            if (width.isEmpty()) continue; // pomiń niewypełnione
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

void OrdersDbView::openPrintDialog() {
    if (selectedOrderId < 0) {
        QMessageBox::warning(this, "Błąd", "Nie wybrano żadnego zamówienia.");
        return;
    }
    
    PrintDialog *printDialog = new PrintDialog(selectedOrderId, this);
    printDialog->exec();
    printDialog->deleteLater();
}

void OrdersDbView::highlightSearchResults(QStandardItemModel* model, const QString& filter, int searchType) {
    if (filter.isEmpty() || !model) {
        return;
    }
    
    // Wyczyść poprzednią listę podświetlonych wierszy
    highlightedRows.clear();
    
    // Znajdź wiersze, które odpowiadają kryteriom wyszukiwania
    for (int row = 0; row < model->rowCount(); ++row) {
        bool shouldHighlight = false;
        
        if (searchType == 0) { // Nr zamówienia
            QString orderNumber = model->item(row, 0)->text();
            if (orderNumber.contains(filter, Qt::CaseInsensitive)) {
                shouldHighlight = true;
            }
        } else if (searchType == 1) { // Nr klienta
            QString clientNumber = model->item(row, 3)->text();
            if (clientNumber.contains(filter, Qt::CaseInsensitive)) {
                shouldHighlight = true;
            }
        } else if (searchType == 2) { // Firma
            QString clientName = model->item(row, 4)->text();
            if (clientName.contains(filter, Qt::CaseInsensitive)) {
                shouldHighlight = true;
            }
        }
        
        if (shouldHighlight) {
            highlightedRows.append(row);
            // Podświetl wszystkie komórki w wierszu na pomarańczowo
            for (int col = 0; col < model->columnCount(); ++col) {
                QModelIndex idx = model->index(row, col);
                model->setData(idx, QColor("#fbbf24"), Qt::BackgroundRole);
            }
        }
    }
    
    // Ustawienie obsługi czyszczenia podświetleń po kliknięciu w wiersz
    if (!highlightedRows.isEmpty()) {
        // Najpierw odłączamy poprzednie połączenie, jeśli istnieje
        if (highlightConnection) {
            disconnect(highlightConnection);
        }
        
        // Teraz łączymy sygnał z nową lambdą
        highlightConnection = connect(tableView->selectionModel(), &QItemSelectionModel::selectionChanged, 
            [this, model](const QItemSelection &, const QItemSelection &) {
                // Wyczyść wszystkie podświetlenia
                for (int row : highlightedRows) {
                    if (row < model->rowCount()) {
                        for (int col = 0; col < model->columnCount(); ++col) {
                            model->setData(model->index(row, col), QVariant(), Qt::BackgroundRole);
                        }
                    }
                }
                // Wyczyść listę podświetlonych wierszy
                highlightedRows.clear();
                
                // Odłącz to połączenie
                disconnect(highlightConnection);
                highlightConnection = QMetaObject::Connection();
            }
        );
    }
}
