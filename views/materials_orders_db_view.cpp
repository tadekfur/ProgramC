#include "materials_orders_db_view.h"
#include "db/dbmanager.h"
#include "views/materials_order_dialog.h"
#include <QStandardItemModel>
#include <QHeaderView>
#include <QMessageBox>
#include <QSettings>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QItemSelectionModel>

MaterialsOrdersDbView::MaterialsOrdersDbView(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    loadOrders();
    // Automatyczne odświeżanie po dodaniu/edycji zamówienia
    connect(&DbManager::instance(), &DbManager::orderAdded,
            this, &MaterialsOrdersDbView::refreshOrders);
}

void MaterialsOrdersDbView::setupUi() {
    mainLayout = new QVBoxLayout(this);
    // --- Dodaj przyciski akcji na górze ---
    QHBoxLayout *btnLayout = new QHBoxLayout;
    // btnRefresh = new QPushButton("Odśwież"); // USUNIĘTY
    btnView = new QPushButton("Podgląd");
    btnEdit = new QPushButton("Edytuj");
    btnDelete = new QPushButton("Usuń");
    btnPrint = new QPushButton("Drukuj");
    btnDuplicate = new QPushButton("Wystaw jako nowe"); // NOWY PRZYCISK
    btnLayout->addWidget(btnView);
    btnLayout->addWidget(btnEdit);
    btnLayout->addWidget(btnDelete);
    btnLayout->addWidget(btnPrint);
    btnLayout->addWidget(btnDuplicate);
    btnLayout->addStretch(1);
    mainLayout->addLayout(btnLayout);
    // --- Tabela zamówień ---
    ordersTable = new QTableView(this);
    ordersModel = new QStandardItemModel(0, 9, this); // 0 wierszy, 9 kolumn
    ordersModel->setHorizontalHeaderLabels({
        "ID", "Nr zamówienia", "Data", "Dostawca", "Adres dostawy", "Termin dostawy", "Uwagi", "Materiały", "Zrealizowane"
    });
    ordersTable->setModel(ordersModel);

    ordersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ordersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ordersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ordersTable->setSortingEnabled(true); // Pozwól na sortowanie po kliknięciu w nagłówek
    ordersTable->setEditTriggers(QAbstractItemView::NoEditTriggers); // Wyłącz edycję poza checkboxem
    mainLayout->addWidget(ordersTable);
    // Przywróć szerokości kolumn
    QSettings settings("Termedia", "EtykietyManager");
    for (int i = 0; i < ordersModel->columnCount(); ++i) {
        int w = settings.value(QString("ordersTable/col%1").arg(i), 120).toInt();
        ordersTable->setColumnWidth(i, w);
    }
    connect(ordersTable->horizontalHeader(), &QHeaderView::sectionResized, this, [this](int col, int, int newSize) {
        QSettings settings("Termedia", "EtykietyManager");
        settings.setValue(QString("ordersTable/col%1").arg(col), newSize);
    });
    // --- Połączenia przycisków ---
    connect(btnView, &QPushButton::clicked, this, &MaterialsOrdersDbView::handleView);
    connect(btnEdit, &QPushButton::clicked, this, &MaterialsOrdersDbView::handleEdit);
    connect(btnDelete, &QPushButton::clicked, this, &MaterialsOrdersDbView::handleDelete);
    connect(btnPrint, &QPushButton::clicked, this, &MaterialsOrdersDbView::handlePrint);
    connect(btnDuplicate, &QPushButton::clicked, this, &MaterialsOrdersDbView::handleDuplicate);
    // Połączenie dla zmiany zaznaczenia
    connect(ordersTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MaterialsOrdersDbView::onSelectionChanged);
    // Połączenie dla zmiany danych w modelu (np. checkbox)
    connect(ordersModel, &QStandardItemModel::itemChanged, this, &MaterialsOrdersDbView::handleDoneChanged);
    // Domyślne sortowanie po numerze zamówienia (rosnąco)
    ordersTable->sortByColumn(1, Qt::AscendingOrder);
}

void MaterialsOrdersDbView::loadOrders() {
    // Usunięto blokowanie sygnałów, aby widok otrzymywał prawidłowe sygnały wstawiania/usuwania wierszy
    // QSignalBlocker blocker(ordersModel);
    ordersModel->removeRows(0, ordersModel->rowCount());
    QVector<QMap<QString, QVariant>> orders = DbManager::instance().getMaterialsOrdersWithDetails();
    qDebug() << "[DEBUG] Liczba zamówień do wyświetlenia:" << orders.size();

    auto safeToString = [](const QVariant& var) -> QString {
        if (var.isNull() || !var.isValid()) {
            return "";
        }
        return var.toString();
    };
    auto dateToString = [](const QVariant& var) -> QString {
        if (var.isNull() || !var.isValid()) {
            return "Brak daty";
        }
        if (var.canConvert<QDate>()) {
            QDate date = var.toDate();
            return date.isValid() ? date.toString("yyyy-MM-dd") : "Błędna data";
        }
        if (var.canConvert<QDateTime>()) {
            QDateTime dt = var.toDateTime();
            return dt.isValid() ? dt.date().toString("yyyy-MM-dd") : "Błędna data";
        }
        QDate date = QDate::fromString(var.toString(), Qt::ISODate);
        return date.isValid() ? date.toString("yyyy-MM-dd") : var.toString();
    };

    for (const auto &order : orders) {
        QString materialsCell = safeToString(order.value("materials_summary")).replace(", ", "\n");
        // Jeśli pole jest puste, ustaw pusty string
        if (materialsCell.trimmed().isEmpty()) {
            materialsCell = "";
        }
        QList<QStandardItem*> rowItems;
        bool isDone = order.value("done", false).toBool();
        rowItems << new QStandardItem(safeToString(order.value("id")))
                 << new QStandardItem(safeToString(order.value("order_number")))
                 << new QStandardItem(dateToString(order.value("order_date")))
                 << new QStandardItem(safeToString(order.value("supplier_name")))
                 << new QStandardItem(safeToString(order.value("delivery_company")))
                 << new QStandardItem(dateToString(order.value("delivery_date")))
                 << new QStandardItem(safeToString(order.value("notes")))
                 << new QStandardItem(materialsCell)
                 << new QStandardItem();
        // COFNIĘTO: rowItems[7]->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        // COFNIĘTO: rowItems[7]->setData(Qt::TextWordWrap, Qt::TextFormatRole);
        rowItems[8]->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        rowItems[8]->setCheckState(isDone ? Qt::Checked : Qt::Unchecked);
        for(QStandardItem* item : rowItems) item->setEditable(false);
        if (isDone) for(QStandardItem* item : rowItems) { item->setBackground(QBrush(QColor(220,220,220))); item->setForeground(QBrush(Qt::darkGray)); }
        ordersModel->appendRow(rowItems);
        // Automatyczna wysokość wiersza na podstawie liczby linii w komórce Materiały
        int linesCount = materialsCell.count('\n') + 1;
        int textHeight = ordersTable->fontMetrics().height() * linesCount + 10;
        int cappedHeight = qBound(28, textHeight, 150); // maks. 150px
        ordersTable->setRowHeight(ordersModel->rowCount()-1, cappedHeight);
    }
    // Po zakończeniu ładowania upewnij się, że widok został odświeżony
    ordersTable->reset();
}

void MaterialsOrdersDbView::handleDoneChanged(QStandardItem *item) {
    if (item->column() != 8) return;
    int row = item->row();
    int orderId = ordersModel->item(row, 0)->text().toInt();
    bool isDone = (item->checkState() == Qt::Checked);
    // Zaktualizuj w bazie (dodaj pole 'done' do tabeli jeśli nie istnieje)
    DbManager::instance().setMaterialsOrderDone(orderId, isDone);
    // Wyszarz lub odszarz wiersz
    for (int col = 0; col < ordersModel->columnCount(); ++col) {
        QStandardItem *it = ordersModel->item(row, col);
        if (it) {
            if (isDone) {
                it->setBackground(QBrush(QColor(220, 220, 220)));
                it->setForeground(QBrush(Qt::darkGray));
            } else {
                it->setBackground(QBrush(Qt::white));
                it->setForeground(QBrush(Qt::black));
            }
        }
    }
}

void MaterialsOrdersDbView::refreshOrders() {
    loadOrders();
}

void MaterialsOrdersDbView::handleEdit() {
    if (selectedOrderId < 0) {
        QMessageBox::warning(this, "Brak wyboru", "Wybierz zamówienie do edycji.");
        return;
    }
    emit requestEditOrder(selectedOrderId);
}

void MaterialsOrdersDbView::handleView() {
    if (selectedOrderId < 0) {
        QMessageBox::warning(this, "Brak wyboru", "Wybierz zamówienie do podglądu.");
        return;
    }
    // Pobierz dane bezpośrednio z bazy dla jednego zamówienia
    QMap<QString, QVariant> order = DbManager::instance().getMaterialsOrderById(selectedOrderId);
    QVector<QMap<QString, QVariant>> items = DbManager::instance().getMaterialsOrderItemsForOrder(selectedOrderId);

    MaterialsOrderDialog dlg(this);
    dlg.setOrderData(order, items);
    dlg.exec();
}

void MaterialsOrdersDbView::handlePrint() {
    if (selectedOrderId < 0) {
        QMessageBox::warning(this, "Brak wyboru", "Wybierz zamówienie do wydruku.");
        return;
    }
    // Zakładam, że generowanie PDF jest już obsługiwane w slocie/handlerze requestPrintOrder
    // Jeśli chcesz generować PDF bezpośrednio tutaj, podaj nazwę funkcji lub kod generujący PDF
    emit requestPrintOrder(selectedOrderId);
}

void MaterialsOrdersDbView::handleDelete() {
    if (selectedOrderId < 0) {
        QMessageBox::warning(this, "Brak wyboru", "Wybierz zamówienie do usunięcia.");
        return;
    }
    if (QMessageBox::question(this, "Usuń zamówienie", "Czy na pewno usunąć to zamówienie?") == QMessageBox::Yes) {
        if (DbManager::instance().deleteMaterialsOrder(selectedOrderId)) {
            QMessageBox::information(this, "Usunięto", "Zamówienie zostało usunięte.");
            refreshOrders();
        } else {
            QMessageBox::warning(this, "Błąd", "Nie udało się usunąć zamówienia.");
        }
    }
}

void MaterialsOrdersDbView::handleDuplicate() {
    if (selectedOrderId < 0) {
        QMessageBox::warning(this, "Brak wyboru", "Wybierz zamówienie do skopiowania jako nowe.");
        return;
    }
    emit requestDuplicateOrder(selectedOrderId);
}

void MaterialsOrdersDbView::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    Q_UNUSED(deselected);
    bool hasSelection = !selected.indexes().isEmpty();
    btnEdit->setEnabled(hasSelection);
    btnDelete->setEnabled(hasSelection);
    btnDuplicate->setEnabled(hasSelection);
    btnView->setEnabled(hasSelection);
    btnPrint->setEnabled(hasSelection);

    if (hasSelection) {
        selectedOrderId = ordersModel->item(selected.indexes().first().row(), 0)->text().toInt();
    } else {
        selectedOrderId = -1;
    }
}
