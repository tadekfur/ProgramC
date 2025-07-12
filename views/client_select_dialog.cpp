#include "client_select_dialog.h"
#include "db/dbmanager.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

ClientSelectDialog::ClientSelectDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Wybierz klienta");
    resize(1200, 600);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    // Pole wyszukiwania
    QHBoxLayout *searchLayout = new QHBoxLayout;
    QLabel *searchLabel = new QLabel("Szukaj:", this);
    QLineEdit *searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Wpisz frazę...");
    QComboBox *searchCombo = new QComboBox(this);
    searchCombo->addItems({"Wszystko", "Nazwa", "Nazwa skrócona", "Miasto", "NIP", "Nr klienta"});
    searchCombo->setMaximumWidth(150);
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(searchEdit);
    searchLayout->addWidget(searchCombo);
    searchLayout->addStretch();
    mainLayout->addLayout(searchLayout);
    // Tabela
    table = new QTableWidget(this);
    table->setColumnCount(11);
    table->setHorizontalHeaderLabels({"ID", "Numer klienta", "Nazwa", "Nazwa skrócona", "Osoba kontaktowa", "Telefon", "E-mail", "Ulica", "Kod pocztowy", "Miasto", "NIP"});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLayout->addWidget(table);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
    loadClients();
    connect(table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ClientSelectDialog::onSelectionChanged);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(searchEdit, &QLineEdit::textChanged, this, [this, searchEdit, searchCombo]() { loadClients(searchEdit->text(), searchCombo->currentText()); });
    connect(searchCombo, &QComboBox::currentTextChanged, this, [this, searchEdit, searchCombo]() { loadClients(searchEdit->text(), searchCombo->currentText()); });
}

void ClientSelectDialog::loadClients(const QString &filter, const QString &mode) {
    auto clients = DbManager::instance().getClients();
    QVector<QMap<QString, QVariant>> filtered;
    if (!filter.isEmpty()) {
        for (const auto &c : clients) {
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
            if (match) filtered.append(c);
        }
    } else {
        filtered = clients;
    }
    table->setRowCount(filtered.size());
    for (int row = 0; row < filtered.size(); ++row) {
        table->setItem(row, 0, new QTableWidgetItem(QString::number(filtered[row]["id"].toInt())));
        table->setItem(row, 1, new QTableWidgetItem(filtered[row]["client_number"].toString()));
        table->setItem(row, 2, new QTableWidgetItem(filtered[row]["name"].toString()));
        table->setItem(row, 3, new QTableWidgetItem(filtered[row]["short_name"].toString()));
        table->setItem(row, 4, new QTableWidgetItem(filtered[row]["contact_person"].toString()));
        table->setItem(row, 5, new QTableWidgetItem(filtered[row]["phone"].toString()));
        table->setItem(row, 6, new QTableWidgetItem(filtered[row]["email"].toString()));
        table->setItem(row, 7, new QTableWidgetItem(filtered[row]["street"].toString()));
        table->setItem(row, 8, new QTableWidgetItem(filtered[row]["postal_code"].toString()));
        table->setItem(row, 9, new QTableWidgetItem(filtered[row]["city"].toString()));
        table->setItem(row, 10, new QTableWidgetItem(filtered[row]["nip"].toString()));
    }
}

void ClientSelectDialog::onSelectionChanged() {
    bool hasSelection = !table->selectionModel()->selectedRows().isEmpty();
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(hasSelection);
    if (hasSelection) {
        int row = table->selectionModel()->selectedRows().first().row();
        selected.clear();
        selected["id"] = table->item(row, 0)->text();
        selected["client_number"] = table->item(row, 1)->text();
        selected["name"] = table->item(row, 2)->text();
        selected["short_name"] = table->item(row, 3)->text();
        selected["contact_person"] = table->item(row, 4)->text();
        selected["phone"] = table->item(row, 5)->text();
        selected["email"] = table->item(row, 6)->text();
        selected["street"] = table->item(row, 7)->text();
        selected["postal_code"] = table->item(row, 8)->text();
        selected["city"] = table->item(row, 9)->text();
        selected["nip"] = table->item(row, 10)->text();
    } else {
        selected.clear();
    }
}

QMap<QString, QVariant> ClientSelectDialog::selectedClient() const {
    return selected;
}
