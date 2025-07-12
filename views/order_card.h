#pragma once
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDate>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QMessageBox>
#include <QWidget>
#include "models/order.h"
#include "models/client.h"

class OrderCard : public QFrame {
    Q_OBJECT
public:
    OrderCard(const Order& order, QWidget* dashboard, QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override; // dodane

private slots:
    void showDetailsDialog();
    void markAsDone();
    void arrowToggled(bool checked);
    void updateOrderStatus(Order::Status newStatus);
    void updateStatusButtons(); // nowy slot

signals:
    void orderStatusChanged(int orderId, Order::Status newStatus);
    void orderDoubleClicked(int orderId); // sygnał dwukliku

private:
    void setupUi(const QColor& gradStart, const QColor& gradEnd);
    int countWorkdays(const QDate& start, const QDate& end) const;
    void setDateLabelColor(int daysLeft);

    Order order;
    QWidget* dashboard;
    QLabel* clientLabel;
    QLabel* dateLabel;
    QLabel* orderBadge;
    QPushButton* doneBtn;
    QToolButton* arrowBtn;
    QPushButton* statusBtn1; // nowy
    QPushButton* statusBtn2; // nowy
    QPushButton* statusBtn3; // nowy
    QPoint dragStartPos;
    QDialog* detailsDialog; // zmiana z QWidget* na QDialog*
};
