#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QDateEdit>
#include <QTextEdit>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QCompleter>
#include <QStringListModel>

class MaterialsOrderForm : public QWidget {
    Q_OBJECT
public:
    explicit MaterialsOrderForm(QWidget *parent = nullptr);
    ~MaterialsOrderForm();

    // Tryb tylko do podglądu
    void setViewOnly(bool viewOnly);
    bool isViewOnly() const { return m_viewOnly; }
    void loadOrderFromDb(int orderId);
    void clearForm(); // Przeniesiona deklaracja metody clearForm() do sekcji public
    void loadNewOrder(); // Dodana metoda do ładowania nowego zamówienia
    QLineEdit* getOrderNumberEdit() const { return orderNumberEdit; } // Dodany getter
    QString generateOrderNumber();
    void setLoadedOrderId(int id) { loadedOrderId = id; } // Dodany setter

private slots:
    void addMaterialRow();
    void handleDeleteMaterial();
    void removeMaterialRow(int row);
    void handleSelectSupplier();
    void handleSaveSupplier();
    void handleSelectDelivery();
    void handleSaveDelivery();
    void handleEditSupplier();
    void handleEditDelivery();
    void handleSaveOrder();
    void updateMaterialCompleter();
    void showOrderSelectDialog();
    void sendOrderByEmail(const QString &pdfPath, const QString &email, const QString &supplierName, const QString &orderNumber);
    void handleEmailSent(bool success, const QString &message);

private:
    void setupUi();
    void refreshMaterialTypes();

    // Dostawca
    QLineEdit *supplierNameEdit;
    QLineEdit *supplierContactEdit;
    QLineEdit *supplierPhoneEdit;
    QLineEdit *supplierEmailEdit;
    QLineEdit *supplierStreetEdit;
    QLineEdit *supplierCityEdit;
    QLineEdit *supplierPostalEdit;
    QLineEdit *supplierCountryEdit;
    QPushButton *btnSelectSupplier;
    QPushButton *btnSaveSupplier;
    QPushButton *btnEditSupplier;  // Nowy przycisk edycji dostawcy
    // Adres dostawy
    QLineEdit *deliveryCompanyEdit;
    QLineEdit *deliveryStreetEdit2;
    QLineEdit *deliveryCityEdit2;
    QLineEdit *deliveryPostalEdit2;
    QLineEdit *deliveryCountryEdit2;
    QLineEdit *deliveryNipEdit; // Dodane pole NIP
    QPushButton *btnSelectDelivery;
    QPushButton *btnSaveDelivery;
    QPushButton *btnEditDelivery;  // Nowy przycisk edycji adresu dostawy

    // Materiały
    QTableWidget *materialsTable;
    QPushButton *btnAddMaterial;
    QPushButton *btnDeleteMaterial; // nowy przycisk
    QStringList materialTypes;
    QCompleter *materialCompleter;
    QStringListModel *materialModel;
    // Nowe pola dla dynamicznych combo
    QStringList widthTypes;
    QStringList lengthTypes;
    QStringList qtyTypes;
    QStringListModel *widthModel;
    QStringListModel *lengthModel;
    QStringListModel *qtyModel;

    // Pozostałe
    QDateEdit *deliveryDateEdit;
    QTextEdit *notesEdit;
    QLineEdit *orderNumberEdit;
    QPushButton *btnSaveOrder;
    QPushButton *btnLoadOrder; // Przycisk do ładowania zamówienia
    int loadedOrderId = -1; // ID aktualnie załadowanego zamówienia (do edycji)
    int currentDeliveryAddressId = -1; // ID aktualnie wybranego adresu dostawy
    bool m_viewOnly = false;
    
    // Metody pomocnicze do okien dialogowych
    bool showEditSupplierDialog(QMap<QString, QVariant> &data);
    bool showEditDeliveryDialog(QMap<QString, QVariant> &data);
};
