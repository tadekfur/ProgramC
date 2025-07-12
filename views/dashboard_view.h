#pragma once
#include <QWidget>
#include "models/order.h"

class DashboardView : public QWidget {
    Q_OBJECT
public:
    explicit DashboardView(QWidget *parent = nullptr);
public slots:
    void refreshDashboard();
    void onOrderStatusChanged(int orderId, Order::Status newStatus);
    void previewOrder(int orderId); // slot do podglądu zamówienia

signals:
    void orderStatusChanged(int orderId, Order::Status newStatus);
};

QWidget* createDashboardGrid(QWidget *parent = nullptr);
