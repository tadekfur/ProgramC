#include "clients_db_view.h"
#include "db/dbmanager.h"
#include "views/client_dialog.h"
#include "views/client_full_dialog.h"
#include <QStandardItemModel>
#include <QMessageBox>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QItemSelection>
#include <QTimer>
#include <QSettings>
#include <QKeyEvent>
#include <QStatusBar>
#include <QToolTip>

ClientsDbView::ClientsDbView(QWidget *parent, bool selectMode) : QWidget(parent), selectionMode(selectMode) {
    setupUI();
    restoreTableState();
    loadClients();
}

void ClientsDbView::setupUI() {
    searchEdit = new QLineEdit(this);
    searchEdit->setMaximumWidth(360);
    searchEdit->setPlaceholderText("Wpisz frazę...");
    searchEdit->setToolTip("Wyszukaj klienta po nazwie, mieście, NIP lub numerze");
    searchCombo = new QComboBox(this);
    searchCombo->addItems({"Wszystko", "Nazwa", "Nazwa skrócona", "Miasto", "NIP", "Nr klienta"});
    searchCombo->setMaximumWidth(150);
    searchCombo->setToolTip("Wybierz pole do wyszukiwania");
    connect(searchEdit, &QLineEdit::textChanged, this, &ClientsDbView::loadClients);
    connect(searchCombo, &QComboBox::currentTextChanged, this, &ClientsDbView::loadClients);
    QLabel *searchLabel = new QLabel("Szukaj: ", this);
    QHBoxLayout *searchLayout = new QHBoxLayout;
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(8);
    searchLayout->addWidget(searchLabel, 0, Qt::AlignLeft);
    searchLayout->addWidget(searchEdit, 0, Qt::AlignLeft);
    searchLayout->addWidget(searchCombo, 0, Qt::AlignLeft);
    btnAdd = nullptr;
    if (selectionMode) {
        btnSelectToOrder = new QPushButton("Wstaw do zamówienia", this);
        btnSelectToOrder->setToolTip("Wstaw wybranego klienta do zamówienia");
        btnSelectToOrder->setEnabled(false);
        searchLayout->addWidget(btnSelectToOrder);
        connect(btnSelectToOrder, &QPushButton::clicked, this, [this]() {
            auto data = getSelectedClientData();
            if (!data.isEmpty()) emit clientSelected(data);
        });
        connect(tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this](const QItemSelection &selected, const QItemSelection &){
            btnSelectToOrder->setEnabled(!selected.indexes().isEmpty());
        });
    } else {
        btnAdd = new QPushButton("Dodaj klienta", this);
        btnAdd->setToolTip("Dodaj nowego klienta (Insert)");
        searchLayout->addWidget(btnAdd);
        connect(btnAdd, &QPushButton::clicked, this, &ClientsDbView::addClient);
    }
    btnEdit = new QPushButton("Edytuj dane", this);
    btnEdit->setToolTip("Edytuj wybranego klienta (Enter)");
    btnDelete = new QPushButton("Usuń", this);
    btnDelete->setToolTip("Usuń wybranego klienta (Delete)");
    btnEdit->setEnabled(false);
    btnDelete->setEnabled(false);
    searchLayout->addSpacing(16);
    searchLayout->addWidget(btnEdit);
    searchLayout->addWidget(btnDelete);
    searchLayout->addStretch();
    tableView = new QTableView(this);
    tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tableView->setSortingEnabled(true);
    mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(searchLayout);
    mainLayout->addWidget(tableView);
    statusBar = new QStatusBar(this);
    mainLayout->addWidget(statusBar);
    setLayout(mainLayout);
    connect(btnEdit, &QPushButton::clicked, this, &ClientsDbView::editClient);
    connect(btnDelete, &QPushButton::clicked, this, &ClientsDbView::deleteClient);
    if (selectionMode) {
        btnSelectToOrder = new QPushButton("Wstaw do zamówienia", this);
        btnSelectToOrder->setToolTip("Wstaw wybranego klienta do zamówienia");
        btnSelectToOrder->setEnabled(false);
        searchLayout->addWidget(btnSelectToOrder);
        connect(btnSelectToOrder, &QPushButton::clicked, this, [this]() {
            auto data = getSelectedClientData();
            if (!data.isEmpty()) emit clientSelected(data);
        });
        connect(tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this](const QItemSelection &selected, const QItemSelection &){
            btnSelectToOrder->setEnabled(!selected.indexes().isEmpty());
        });
    }
}

void ClientsDbView::saveTableState() {
    QSettings s("TwojaFirma", "EtykietyManager");
    s.setValue("clients_table_state", tableView->horizontalHeader()->saveState());
}
void ClientsDbView::restoreTableState() {
    QSettings s("TwojaFirma", "EtykietyManager");
    QByteArray state = s.value("clients_table_state").toByteArray();
    if (!state.isEmpty()) tableView->horizontalHeader()->restoreState(state);
}
void ClientsDbView::showStatus(const QString &msg, int timeoutMs) {
    if (statusBar) statusBar->showMessage(msg, timeoutMs);
}
void ClientsDbView::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Insert) {
        addClient();
        event->accept();
    } else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        editClient();
        event->accept();
    } else if (event->key() == Qt::Key_Delete) {
        deleteClient();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void ClientsDbView::refreshClients() {
    qDebug() << "[ClientsDbView::refreshClients] Wywołano";
    loadClients();
}

void ClientsDbView::loadClients() {
    qDebug() << "[ClientsDbView::loadClients] Wywołano";
    auto& db = DbManager::instance();
    auto clients = db.getClients();
    QString filter = searchEdit ? searchEdit->text().trimmed() : "";
    QString mode = searchCombo ? searchCombo->currentText() : "Wszystko";
    QList<QMap<QString, QVariant>> filtered;
    int autoSelectRow = -1;
    if (!filter.isEmpty()) {
        for (int i = 0; i < clients.size(); ++i) {
            const auto &c = clients[i];
            bool match = false;
            if (mode == "Wszystko") {
                match = c["name"].toString().contains(filter, Qt::CaseInsensitive) ||
                        c["short_name"].toString().contains(filter, Qt::CaseInsensitive) ||
                        c["city"].toString().contains(filter, Qt::CaseInsensitive) ||
                        c["nip"].toString().contains(filter, Qt::CaseInsensitive) ||
                        c["client_number"].toString().contains(filter, Qt::CaseInsensitive);
            } else if (mode == "Nazwa" && c["name"].toString().contains(filter, Qt::CaseInsensitive)) {
                match = true;
            } else if (mode == "Nazwa skrócona" && c["short_name"].toString().contains(filter, Qt::CaseInsensitive)) {
                match = true;
            } else if (mode == "Miasto" && c["city"].toString().contains(filter, Qt::CaseInsensitive)) {
                match = true;
            } else if (mode == "NIP" && c["nip"].toString().contains(filter, Qt::CaseInsensitive)) {
                match = true;
            } else if (mode == "Nr klienta" && c["client_number"].toString().contains(filter, Qt::CaseInsensitive)) {
                match = true;
            }
            if (match) {
                if (autoSelectRow == -1) autoSelectRow = filtered.size();
                filtered.append(c);
            }
        }
    } else {
        filtered = clients;
    }
    // Wyświetl wszystkie dane klienta
    QStringList headers = {"ID", "Numer klienta", "Nazwa", "Nazwa skrócona", "Osoba kontaktowa", "Telefon", "E-mail", "Ulica", "Kod pocztowy", "Miasto", "NIP"};
    auto *model = new QStandardItemModel(filtered.size(), headers.size(), this);
    model->setHorizontalHeaderLabels(headers);
    for (int row = 0; row < filtered.size(); ++row) {
        auto item0 = new QStandardItem(QString::number(filtered[row]["id"].toInt()));
        item0->setTextAlignment(Qt::AlignCenter);
        model->setItem(row, 0, item0);
        auto item1 = new QStandardItem(filtered[row]["client_number"].toString());
        item1->setTextAlignment(Qt::AlignCenter);
        model->setItem(row, 1, item1);
        auto item2 = new QStandardItem(filtered[row]["name"].toString());
        item2->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        model->setItem(row, 2, item2);
        auto item3 = new QStandardItem(filtered[row]["short_name"].toString());
        item3->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        model->setItem(row, 3, item3);
        auto item4 = new QStandardItem(filtered[row]["contact_person"].toString());
        item4->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        model->setItem(row, 4, item4);
        auto item5 = new QStandardItem(filtered[row]["phone"].toString());
        item5->setTextAlignment(Qt::AlignCenter);
        model->setItem(row, 5, item5);
        auto item6 = new QStandardItem(filtered[row]["email"].toString());
        item6->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        model->setItem(row, 6, item6);
        auto item7 = new QStandardItem(filtered[row]["street"].toString());
        item7->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        model->setItem(row, 7, item7);
        auto item8 = new QStandardItem(filtered[row]["postal_code"].toString());
        item8->setTextAlignment(Qt::AlignCenter);
        model->setItem(row, 8, item8);
        auto item9 = new QStandardItem(filtered[row]["city"].toString());
        item9->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        model->setItem(row, 9, item9);
        auto item10 = new QStandardItem(filtered[row]["nip"].toString());
        item10->setTextAlignment(Qt::AlignCenter);
        model->setItem(row, 10, item10);
    }
    tableView->setModel(model);
    connect(tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ClientsDbView::onSelectionChanged);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    tableView->horizontalHeader()->setStretchLastSection(false);
    // Ustaw pogrubiony font dla nagłówków kolumn
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
        "QTableView::item:selected { background: #2563eb; color: #fff; } " // niebieski dla zaznaczenia kursorem
        "QTableView::item:focus { background: #2563eb; } "
    );
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    tableView->horizontalHeader()->setStretchLastSection(false);
    tableView->resizeColumnsToContents();
    tableView->verticalHeader()->setVisible(false);
    // Automatyczne podświetlenie pierwszego wyniku na pomarańczowo
    if (autoSelectRow >= 0 && !filtered.isEmpty()) {
        for (int row = 0; row < model->rowCount(); ++row) {
            for (int col = 0; col < model->columnCount(); ++col) {
                QModelIndex idx = model->index(row, col);
                if (row == autoSelectRow) {
                    model->setData(idx, QColor("#fbbf24"), Qt::BackgroundRole);
                } else {
                    model->setData(idx, QVariant(), Qt::BackgroundRole);
                }
            }
        }
        // Usunięcie pomarańczowego po kliknięciu w wiersz
        QTimer::singleShot(0, this, [this, model]( ) {
            connect(tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [model](const QItemSelection &selected, const QItemSelection &){
                for (int row = 0; row < model->rowCount(); ++row) {
                    for (int col = 0; col < model->columnCount(); ++col) {
                        model->setData(model->index(row, col), QVariant(), Qt::BackgroundRole);
                    }
                }
            });
        });
    }
}

QMap<QString, QVariant> ClientsDbView::getSelectedClientData() const {
    QMap<QString, QVariant> clientData;
    auto *model = tableView->model();
    auto sel = tableView->selectionModel()->selectedRows();
    if (sel.isEmpty()) return clientData;
    int row = sel.first().row();
    for (int col = 0; col < model->columnCount(); ++col) {
        QString header = model->headerData(col, Qt::Horizontal).toString();
        clientData[header] = model->data(model->index(row, col));
    }
    return clientData;
}

void ClientsDbView::addClient() {
    qDebug() << "[DEBUG] Wywołano ClientsDbView::addClient()";
    ClientFullDialog dlg(this, ClientFullDialog::Add);
    connect(&dlg, &ClientFullDialog::clientAdded, this, &ClientsDbView::refreshClients); // automatyczne odświeżanie
    dlg.exec();
    // Nie zapisujemy już klienta tutaj! Zapis i obsługa konfliktów są w dialogu.
    // Widok zostanie odświeżony przez sygnał clientAdded.
}

void ClientsDbView::editClient() {
    if (selectedClientId < 0) return;
    auto& db = DbManager::instance();
    auto clients = db.getClients();
    QMap<QString, QVariant> data;
    int rowToSelect = -1;
    for (int i = 0; i < clients.size(); ++i) {
        if (clients[i]["id"].toInt() == selectedClientId) {
            data = clients[i];
            rowToSelect = i;
            break;
        }
    }
    if (data.isEmpty()) return;
    auto addresses = db.getDeliveryAddresses(selectedClientId);
    ClientFullDialog dlg(this, ClientFullDialog::Edit);
    dlg.setClientData(data);
    dlg.setDeliveryAddresses(addresses);
    if (dlg.exec() == QDialog::Accepted) {
        if (db.updateClient(selectedClientId, dlg.clientData())) {
            loadClients();
            showStatus("Zaktualizowano dane klienta");
            if (rowToSelect >= 0) {
                tableView->selectRow(rowToSelect);
            }
        } else {
            QMessageBox::warning(this, "Błąd", "Nie udało się zaktualizować klienta.");
        }
    }
}

void ClientsDbView::deleteClient() {
    if (selectedClientId < 0) return;
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Potwierdzenie usunięcia", "Czy na pewno chcesz usunąć tego klienta?", QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        auto& db = DbManager::instance();
        db.deleteClient(selectedClientId);
        loadClients();
        showStatus("Usunięto klienta");
    }
}

void ClientsDbView::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    Q_UNUSED(deselected);
    if (selected.indexes().isEmpty()) {
        btnEdit->setEnabled(false);
        btnDelete->setEnabled(false);
        selectedClientId = -1;
    } else {
        btnEdit->setEnabled(true);
        btnDelete->setEnabled(true);
        // Ustal wybrany wiersz na podstawie indeksu wiersza
        int row = selected.indexes().first().row();
        selectedClientId = tableView->model()->data(tableView->model()->index(row, 0)).toInt();
    }
}

ClientsDbView::~ClientsDbView() {
    saveTableState();
}
