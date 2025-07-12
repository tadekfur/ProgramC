#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>
#include <QMap>
#include <QObject>
#include <QDate>
#include <QSqlError>

class DbManager : public QObject {
    Q_OBJECT
public:
    static DbManager& instance(); // singleton
    QSqlDatabase database(); // Główne połączenie
    QSqlDatabase getPooledConnection(); // Połączenie z puli
    void returnPooledConnection(QSqlDatabase& connection);
    static bool isOpen();
    // Klienci
    QVector<QMap<QString, QVariant>> getClients();
    bool addClient(const QMap<QString, QVariant>& data);
    bool updateClient(int id, const QMap<QString, QVariant>& data);
    bool deleteClient(int id);
    // Sprawdza czy klient z danym NIP już istnieje, zwraca ID klienta lub -1 jeśli nie ma
    int findClientByNip(const QString& nip);
    int findClientByNumber(const QString &clientNumber);
    int getMaxClientNumber();
    bool addClientWithAddresses(const QMap<QString, QVariant>& data, const QList<QMap<QString, QVariant>>& addresses);
    int getNextUniqueClientNumber(); // Zwraca unikatowy numer klienta, nigdy nieużyty
    void markClientNumberUsed(int clientNumber); // Zapisuje numer jako użyty
    bool updateClientWithAddresses(int id, const QMap<QString, QVariant>& data, const QList<QMap<QString, QVariant>>& addresses);
    // Zamówienia
    QVector<QMap<QString, QVariant>> getOrders();
    QMap<QString, QVariant> getOrderById(int orderId);
    bool addOrder(const QMap<QString, QVariant>& orderData, const QVector<QMap<QString, QVariant>>& items);
    bool updateOrder(int id, const QMap<QString, QVariant>& orderData, const QVector<QMap<QString, QVariant>>& items);
    bool deleteOrder(int id);
    // --- CRUD dla adresów dostawy ---
    QVector<QMap<QString, QVariant>> getDeliveryAddresses(int clientId = -1);
    bool addDeliveryAddress(const QMap<QString, QVariant>& data);
    bool updateDeliveryAddress(const QMap<QString, QVariant>& data);
    bool migrateDeliveryAddresses(); // Migracja istniejących danych adresów dostawy
    // Aktualizuje tylko datę dostawy zamówienia
    bool updateOrderDeliveryDate(int id, const QDate& newDate);
    QVector<QMap<QString, QVariant>> getOrderItems(int orderId);
    
    // Wyczyszczenie wszystkich zamówień z bazy danych
    bool clearAllOrders();
    
    // Pobieranie zamówień materiałów z dodatkowymi szczegółami (dostawca, podsumowanie materiałów)
    QVector<QMap<QString, QVariant>> getMaterialsOrdersWithDetails();
    
    // Funkcja do sprawdzenia liczby zamówień w bazie danych (do debugowania)
    int getOrdersCount();
    
    // Pobranie ostatniego błędu bazy danych
    QSqlError lastError() const;
    
    // Pomocnicza funkcja do sprawdzania istnienia kolumny w tabeli
    bool checkColumnExistence(const QString& tableName, const QString& columnName) const;
    QString getClientNameById(int clientId) const;

    // --- CRUD dla dostawców (suppliers) ---
    QVector<QMap<QString, QVariant>> getSuppliers();
    QMap<QString, QVariant> getSupplierById(int supplierId);
    bool addSupplier(const QMap<QString, QVariant>& data);
    bool updateSupplier(int id, const QMap<QString, QVariant>& data);
    bool deleteSupplier(int id);

    // --- CRUD dla katalogu materiałów (materials_catalog) ---
    QVector<QMap<QString, QVariant>> getMaterialsCatalog();
    QMap<QString, QVariant> getMaterialById(int materialId);
    bool addMaterial(const QMap<QString, QVariant>& data);
    bool updateMaterial(int id, const QMap<QString, QVariant>& data);
    bool deleteMaterial(int id);

    // --- CRUD dla zamówień materiałów (materials_orders) ---
    QVector<QMap<QString, QVariant>> getMaterialsOrders();
    QMap<QString, QVariant> getMaterialsOrderById(int orderId);
    bool addMaterialsOrder(const QMap<QString, QVariant>& orderData, const QVector<QMap<QString, QVariant>>& items);
    bool updateMaterialsOrder(int id, const QMap<QString, QVariant>& orderData, const QVector<QMap<QString, QVariant>>& items);
    bool deleteMaterialsOrder(int id);
    QVector<QMap<QString, QVariant>> getMaterialsOrderItems(int orderId);
    // Nowa metoda do obsługi checkboxa "Zrealizowane"
    bool setMaterialsOrderDone(int orderId, bool done);

    // --- Numeracja zamówień materiałów ---
    QString getNextMaterialsOrderNumber();

    // --- PODPOWIEDZI (QCompleter) dla materiałów ---
    QVector<QVariant> getUniqueMaterialWidths();
    QVector<QVariant> getUniqueMaterialLengths();
    QVector<QVariant> getUniqueMaterialRolls();
    // --- Automatyczne ładowanie pozycji zamówienia materiałów ---
    QVector<QMap<QString, QVariant>> getMaterialsOrderItemsForOrder(int orderId);

signals:
    void dbConnectionError(const QString &errorMsg);
    void orderAdded(); // Sygnał emitowany po dodaniu nowego zamówienia

private:
    DbManager(QObject *parent = nullptr); // prywatny konstruktor
    DbManager(const DbManager&) = delete;
    DbManager& operator=(const DbManager&) = delete;
    
    // Główne połączenie do bazy danych
    QSqlDatabase db;
    
    // Connection pool
    static const int POOL_SIZE = 3;
    QVector<QSqlDatabase> connectionPool;
    QVector<bool> connectionAvailable;
    int getAvailableConnection();
    void releaseConnection(int connectionId);
    
    // Helper methods
    void initializeTables(); // Create basic tables for SQLite
    bool executeTransaction(const std::function<bool()>& operation); // Zunifikowana obsługa transakcji

    QSqlError m_lastError; // Dodano pole do przechowywania ostatniego błędu SQL
};
