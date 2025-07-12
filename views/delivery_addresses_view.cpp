#include "delivery_addresses_view.h"
#include "db/dbmanager.h"
#include <QStandardItemModel>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QSettings>
#include <QDialogButtonBox>
#include <QHBoxLayout>

DeliveryAddressesView::DeliveryAddressesView(int clientId, QWidget *parent) : QWidget(parent) {
    mainLayout = new QVBoxLayout(this);
    tableView = new QTableView(this);
    mainLayout->addWidget(tableView);
    // Dodaj przyciski Wybierz i Anuluj
    QDialogButtonBox *buttonBox = new QDialogButtonBox(Qt::Horizontal, this);
    QPushButton *btnSelect = buttonBox->addButton("Wybierz", QDialogButtonBox::AcceptRole);
    QPushButton *btnCancel = buttonBox->addButton("Anuluj", QDialogButtonBox::RejectRole);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
    loadAddresses(clientId);
    // Dwuklik wybiera adres
    connect(tableView, &QTableView::doubleClicked, this, [this]() { emitSelectedAddress(); });
    // Wybierz
    connect(btnSelect, &QPushButton::clicked, this, [this]() { emitSelectedAddress(); });
    // Anuluj
    connect(btnCancel, &QPushButton::clicked, this, [this]() { close(); });
}

void DeliveryAddressesView::emitSelectedAddress() {
    auto sel = tableView->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;
    int row = sel.first().row();
    QMap<QString, QVariant> addressData;
    for (int col = 0; col < tableView->model()->columnCount(); ++col) {
        QString header = tableView->model()->headerData(col, Qt::Horizontal).toString();
        addressData[header] = tableView->model()->data(tableView->model()->index(row, col));
    }
    emit addressSelected(addressData);
    close();
}

void DeliveryAddressesView::loadAddresses(int clientId) {
    auto& db = DbManager::instance();
    auto addresses = db.getDeliveryAddresses(clientId);
    QStringList headers = {"Nazwa", "Firma", "Ulica", "Kod pocztowy", "Miasto", "Osoba kontaktowa", "Telefon", "ID"};
    auto *model = new QStandardItemModel(addresses.size(), headers.size(), this);
    model->setHorizontalHeaderLabels(headers);
    for (int row = 0; row < addresses.size(); ++row) {
        model->setItem(row, 0, new QStandardItem(addresses[row]["name"].toString()));
        model->setItem(row, 1, new QStandardItem(addresses[row]["company"].toString()));
        model->setItem(row, 2, new QStandardItem(addresses[row]["street"].toString()));
        model->setItem(row, 3, new QStandardItem(addresses[row]["postal_code"].toString()));
        model->setItem(row, 4, new QStandardItem(addresses[row]["city"].toString()));
        model->setItem(row, 5, new QStandardItem(addresses[row]["contact_person"].toString()));
        model->setItem(row, 6, new QStandardItem(addresses[row]["phone"].toString()));
        model->setItem(row, 7, new QStandardItem(QString::number(addresses[row]["id"].toInt())));
    }
    tableView->setModel(model);
    for (int i = 0; i < headers.size(); ++i) {
        tableView->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Interactive);
    }
    tableView->horizontalHeader()->setSectionsMovable(true);
    tableView->horizontalHeader()->setSectionsClickable(true);
    // Przywróć szerokości kolumn jeśli są zapisane
    QSettings settings("EtykietyManager", "DeliveryAddressesView");
    QByteArray state = settings.value("tableViewState").toByteArray();
    if (!state.isEmpty())
        tableView->horizontalHeader()->restoreState(state);
    tableView->horizontalHeader()->setStretchLastSection(false);
    // Ustaw pogrubiony font dla nagłówków kolumn
    QFont headerFont = tableView->horizontalHeader()->font();
    headerFont.setBold(true);
    tableView->horizontalHeader()->setFont(headerFont);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableView->setMinimumWidth(900);
    tableView->setMinimumHeight(400);
    tableView->setAlternatingRowColors(true);
    tableView->setStyleSheet(
        "QTableView { font-size: 15px; } "
        "QHeaderView::section { font-size: 15px; font-weight: bold; background: #f3f6fa; color: #222; border: 1px solid #ddd; padding: 6px 8px; } "
        "QTableView::item:selected { background: #2563eb; color: #fff; } "
        "QTableView::item:focus { background: #2563eb; } "
    );
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    tableView->horizontalHeader()->setStretchLastSection(false);
    tableView->resizeColumnsToContents();
    tableView->verticalHeader()->setVisible(false);
}

void DeliveryAddressesView::onSelectionChanged(const QItemSelection &selected, const QItemSelection &) {
    // Usunięto automatyczne emitowanie addressSelected przy zmianie zaznaczenia
}

DeliveryAddressesView::~DeliveryAddressesView() {
    QSettings settings("EtykietyManager", "DeliveryAddressesView");
    settings.setValue("tableViewState", tableView->horizontalHeader()->saveState());
}
