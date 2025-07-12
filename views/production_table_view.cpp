#include <QHeaderView>
#include "production_table_view.h"

ProductionTableView::ProductionTableView(QWidget* parent)
    : QTableView(parent), model(new QStandardItemModel(this))
{
    setAlternatingRowColors(true);
    setShowGrid(true);
    setFrameShape(QFrame::Box);
    setLineWidth(1);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::NoSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setMinimumWidth(1100);
    setMinimumHeight(220);
    model->setHorizontalHeaderLabels({
        "Szerokość", "Wysokość", "Materiał", "Ilość", "Typ ilości", "Nawój/długość", "Rdzeń", "Cena", "Typ ceny"
    });
    setModel(model);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    horizontalHeader()->setStretchLastSection(false);
    horizontalHeader()->setMinimumHeight(32);
    verticalHeader()->setVisible(false);
}

void ProductionTableView::setOrderItems(const QList<QVariantMap>& items) {
    model->removeRows(0, model->rowCount());
    // Bufory na wartości do połączenia w jednej komórce
    QStringList widthList, heightList, materialList, quantityList, quantityTypeList, rollLengthList, coreList, priceList, priceTypeList;
    for (const auto& it : items) {
        QString width = it["width"].toString().trimmed();
        QString height = it["height"].toString().trimmed();
        QString material = it["material"].toString().trimmed();
        QString quantity = it["ordered_quantity"].toString().trimmed();
        // Dodajemy tylko jeśli wszystkie kluczowe dane są niepuste i ilość > 0
        bool ok = false;
        int quantityInt = quantity.toInt(&ok);
        if (!width.isEmpty() && !height.isEmpty() && !material.isEmpty() && ok && quantityInt > 0) {
            widthList << width;
            heightList << height;
            materialList << material;
            quantityList << quantity;
            quantityTypeList << it["quantity_type"].toString();
            rollLengthList << it["roll_length"].toString();
            coreList << it["core"].toString();
            QString priceType = it["price_type"].toString();
            QString priceValue;
            if (priceType == "tyś." || priceType == "tys." || priceType == "tysiąc") {
                priceValue = it["price_per_thousand"].toString();
            } else if (priceType == "rolka" || priceType == "rolkę") {
                priceValue = it["price_per_roll"].toString();
            } else {
                priceValue = it["price"].toString();
            }
            priceList << priceValue;
            priceTypeList << priceType;
        }
    }
    if (!widthList.isEmpty()) {
        QList<QStandardItem*> row;
        row << new QStandardItem(widthList.join("\n"));
        row << new QStandardItem(heightList.join("\n"));
        row << new QStandardItem(materialList.join("\n"));
        row << new QStandardItem(quantityList.join("\n"));
        row << new QStandardItem(quantityTypeList.join("\n"));
        row << new QStandardItem(rollLengthList.join("\n"));
        row << new QStandardItem(coreList.join("\n"));
        row << new QStandardItem(priceList.join("\n"));
        row << new QStandardItem(priceTypeList.join("\n"));
        model->appendRow(row);
    }
    resizeColumnsToContents();
    resizeRowsToContents();
}
