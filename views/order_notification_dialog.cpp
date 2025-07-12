#include "order_notification_dialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFont>

OrderNotificationDialog::OrderNotificationDialog(const QString &orderNumber, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Wykonano zamówienie");
    setModal(true);
    setMinimumSize(420, 220);
    QVBoxLayout *layout = new QVBoxLayout(this);
    label = new QLabel(this);
    QFont font;
    font.setPointSize(22);
    font.setBold(true);
    label->setFont(font);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    setOrderNumber(orderNumber);
    removeBtn = new QPushButton("Usuń zamówienie", this);
    removeBtn->setMinimumHeight(40);
    cancelBtn = new QPushButton("Anuluj", this);
    cancelBtn->setMinimumHeight(40);
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(removeBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);
    connect(removeBtn, &QPushButton::clicked, this, [this]() { emit removeOrder(); accept(); });
    connect(cancelBtn, &QPushButton::clicked, this, [this]() { emit cancel(); reject(); });
}

void OrderNotificationDialog::setOrderNumber(const QString &orderNumber) {
    label->setText(QString("<b>Wykonano zamówienie nr: %1</b>").arg(orderNumber));
}
