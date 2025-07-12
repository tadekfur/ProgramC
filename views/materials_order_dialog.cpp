#include "materials_order_dialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QDialogButtonBox>

MaterialsOrderDialog::MaterialsOrderDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Podgląd zamówienia materiałów");
    setModal(true);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    // Placeholder, real content set in setOrderData
    mainLayout->addWidget(new QLabel("Ładowanie zamówienia..."));
}

void MaterialsOrderDialog::setOrderData(const QMap<QString, QVariant> &order, const QVector<QMap<QString, QVariant>> &items)
{
    QLayout *oldLayout = layout();
    if (oldLayout) {
        QLayoutItem *child;
        while ((child = oldLayout->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }
    }
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (!mainLayout) {
        mainLayout = new QVBoxLayout(this);
        setLayout(mainLayout);
    }
    // Dane zamówienia
    QString info = QString("<b>Nr zamówienia:</b> %1<br><b>Data zamówienia:</b> %2<br><b>Dostawca:</b> %3, %4, %5<br><b>Adres dostawy:</b> %6, %7, %8<br><b>Termin dostawy:</b> %9<br><b>Uwagi:</b> %10")
        .arg(order.value("order_number").toString())
        .arg(order.value("order_date").toString())
        .arg(order.value("supplier_name").toString())
        .arg(order.value("supplier_city").toString())
        .arg(order.value("supplier_street").toString())
        .arg(order.value("delivery_company").toString())
        .arg(order.value("delivery_city").toString())
        .arg(order.value("delivery_street").toString())
        .arg(order.value("delivery_date").toString())
        .arg(order.value("notes").toString());
    mainLayout->addWidget(new QLabel(info));
    // Tabela materiałów
    QTableWidget *table = new QTableWidget(items.size(), 4, this);
    table->setHorizontalHeaderLabels({"Materiał", "Szerokość", "Długość", "Ilość"});
    for (int i = 0; i < items.size(); ++i) {
        table->setItem(i, 0, new QTableWidgetItem(items[i].value("material_name").toString()));
        table->setItem(i, 1, new QTableWidgetItem(items[i].value("width").toString()));
        table->setItem(i, 2, new QTableWidgetItem(items[i].value("length").toString()));
        table->setItem(i, 3, new QTableWidgetItem(items[i].value("quantity").toString()));
    }
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(table);
    // Przycisk zamknięcia
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}
