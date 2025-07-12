#pragma once
#include <QWidget>
#include <QDate>
#include <QVector>
#include <QMap>
#include <QSqlDatabase>

class QLineEdit;
class QComboBox;
class QPushButton;
class QDateEdit;
class QTextEdit;
class QGroupBox;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QScrollArea;
class QLabel;
class QWidget;

class NewOrderView : public QWidget {
    Q_OBJECT
public:
    explicit NewOrderView(QWidget *parent = nullptr);
    void loadOrderData(const QMap<QString, QVariant> &orderData, bool editMode = true);
    void resetForm();
signals:
    void orderSaved();
    void clientAdded();
private:
    void setupUi();
    void addProductBlock(int number = -1);
    void removeProductBlock(QWidget *blockWidget);
    void relayoutProductBlocks();
    void fillFromClient();
    void fillFromOrder();
    void handleSelectClientFromDb();
    void handleAddNewClient();
    void handleSelectDeliveryAddress();
    void saveOrder();
    bool isPolishHoliday(const QDate &date) const;
    void updateOrderRolls(QMap<QString, QWidget*> &prodFields);
    QString generateOrderNumber(QSqlDatabase db);
    bool validateOrderForm(QString &errors);
    void fetchGusData(const QString& nip);
    // --- Pola formularza ---
    QLineEdit *nrEdit;
    QDateEdit *orderDateEdit;
    QDateEdit *deliveryDateEdit;
    QLineEdit *clientNumberEdit;
    QComboBox *paymentTermCombo;
    QPushButton *btnSelectClient;
    QPushButton *btnAddClient;
    QVector<QLineEdit*> clientFields;
    QGroupBox *prodContainer;
    QVBoxLayout *prodMainLayout;
    QGridLayout *prodGrid;
    QPushButton *btnAddPosition;
    QVector<QMap<QString, QWidget*>> prodFieldsList;
    QVector<QWidget*> prodBlocks;
    QGroupBox *deliveryAddrGroup;
    QVector<QLineEdit*> addrFields;
    QTextEdit *notesEdit;
    QPushButton *btnSave;
    QPushButton *btnAddrFromDb;
    QLabel *titleLabel;
    QScrollArea *formArea;
    QVBoxLayout *formLayout;
    QGroupBox *clientGroup;
};
