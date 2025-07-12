#ifndef ORDERNOTIFICATIONDIALOG_H
#define ORDERNOTIFICATIONDIALOG_H

#include <QDialog>

class QPushButton;
class QLabel;

class OrderNotificationDialog : public QDialog {
    Q_OBJECT
public:
    explicit OrderNotificationDialog(const QString &orderNumber, QWidget *parent = nullptr);
    void setOrderNumber(const QString &orderNumber);

signals:
    void removeOrder();
    void cancel();

private:
    QLabel *label;
    QPushButton *removeBtn;
    QPushButton *cancelBtn;
};

#endif // ORDERNOTIFICATIONDIALOG_H
