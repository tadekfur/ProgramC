#ifndef NEW_ORDER_DIALOG_H
#define NEW_ORDER_DIALOG_H

#include <QDialog>
#include <QDate>
#include <QVector>
#include <QMap>
#include <QFont>
#include <QCalendarWidget>
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

// Szkielet klasy dialogu wystawiania zamówienia
class NewOrderDialog : public QDialog {
    Q_OBJECT
public:
    explicit NewOrderDialog(QWidget *parent = nullptr);
    ~NewOrderDialog();
    void clearForm(); // Dodano metodę czyszczącą

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupUi();
    void addProductBlock(int number = -1);
    void removeProductBlock(QWidget *blockWidget);
    void relayoutProductBlocks();
    void fillFromClient(/*Client*/);
    void fillFromOrder(/*Order*/);
    void handleSelectClientFromDb();
    void handleAddNewClient();
    void handleSelectDeliveryAddress();
    void saveOrder();
    bool isPolishHoliday(const QDate &date) const;
    void updateOrderRolls(QMap<QString, QWidget*> &prodFields);
    QString generateOrderNumber(QSqlDatabase db);
    bool validateOrderForm(QString &errors);
    void fetchGusData(const QString& nip); // Nowa metoda do pobierania danych z GUS

    // --- Pola formularza ---
    QLineEdit *nrEdit;
    QDateEdit *orderDateEdit;
    QDateEdit *deliveryDateEdit;
    QLineEdit *clientNumberEdit;
    QComboBox *paymentTermCombo;
    QPushButton *btnSelectClient;
    QPushButton *btnAddClient;
    QVector<QLineEdit*> clientFields; // Firma, Nazwa skrócona, Osoba kontaktowa, Tel, Email, Ulica, Kod, Miasto, NIP
    QGroupBox *prodContainer;
    QVBoxLayout *prodMainLayout;
    QGridLayout *prodGrid;
    QPushButton *btnAddPosition;
    QVector<QMap<QString, QWidget*>> prodFieldsList; // Lista słowników pól pozycji
    QGroupBox *deliveryAddrGroup;
    QVector<QLineEdit*> addrFields;
    QPushButton *btnAddrFromDb;
    QTextEdit *notesEdit;
    QPushButton *btnSave;
    QPushButton *btnSaveClient;
    QPushButton *btnSaveDeliveryAddr; // Dodano przycisk zapisu adresu dostawy
    QScrollArea *formArea;
    QVBoxLayout *formLayout;
    QLabel *titleLabel;

    // --- Inne ---
    QVector<QWidget*> prodBlocks;

private slots:
    void handleSaveClient();
    void handleSaveDeliveryAddress(); // Dodano slot do obsługi zapisu adresu dostawy

signals:
    void clientAdded();
};

#endif // NEW_ORDER_DIALOG_H
