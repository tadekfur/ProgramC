#include "dbmanager.h"
#include "views/client_full_dialog.h"
#include "../utils/secure_config.h"
#include <QSqlError>
#include <QDebug>
#include <QSqlRecord>
#include <QVariant>
#include <QVector>
#include <QMap>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSettings>
#include <QRegularExpression>
#include <QDate>
#include <QUuid>

DbManager& DbManager::instance() {
    static DbManager instance;
    return instance;
}

DbManager::DbManager(QObject *parent) : QObject(parent) {
    // Force clean slate - remove any existing connections
    if (QSqlDatabase::contains("main_conn")) {
        QSqlDatabase::removeDatabase("main_conn");
    }
    
    // Usunięcie istniejących połączeń z puli
    for (int i = 0; i < POOL_SIZE; i++) {
        QString connName = QString("pool_conn_%1").arg(i);
        if (QSqlDatabase::contains(connName)) {
            QSqlDatabase::removeDatabase(connName);
        }
    }
    
    // Pobierz bezpieczną konfigurację
    SecureConfig& config = SecureConfig::instance();
    
    bool usePostgreSQL = false;
    
    // Spróbuj PostgreSQL jeśli jest skonfigurowany
    if (config.getDatabaseType() == "QPSQL") {
        db = QSqlDatabase::addDatabase("QPSQL", "main_conn");
        db.setHostName(config.getDatabaseHost());
        db.setPort(config.getDatabasePort());
        db.setDatabaseName(config.getDatabaseName());
        db.setUserName(config.getDatabaseUser());
        
        QString password = config.getDatabasePassword();
        if (password.isEmpty()) {
            qCritical() << "Database password not configured! Set DB_PASSWORD environment variable.";
            emit dbConnectionError("Database password not configured");
            return;
        }
        db.setPassword(password);
        
        if (db.open()) {
            usePostgreSQL = true;
            qDebug() << "Connected to PostgreSQL database";
        } else {
            qWarning() << "Failed to connect to PostgreSQL:" << db.lastError().text();
            QSqlDatabase::removeDatabase("main_conn");
        }
    }
    
    // Fallback do SQLite
    if (!usePostgreSQL) {
        db = QSqlDatabase::addDatabase("QSQLITE", "main_conn");
        db.setDatabaseName("etykiety_db.sqlite");
        
        if (db.open()) {
            qDebug() << "Connected to SQLite database";
            initializeTables();
        } else {
            qCritical() << "Failed to open SQLite database:" << db.lastError().text();
            emit dbConnectionError("Nie można otworzyć bazy danych SQLite");
            return;
        }
    }
    
    // Inicjalizacja connection pool
    connectionAvailable.resize(POOL_SIZE);
    connectionPool.resize(POOL_SIZE);
    
    for (int i = 0; i < POOL_SIZE; i++) {
        QString connName = QString("pool_conn_%1").arg(i);
        
        if (usePostgreSQL) {
            connectionPool[i] = QSqlDatabase::addDatabase("QPSQL", connName);
            connectionPool[i].setHostName(config.getDatabaseHost());
            connectionPool[i].setPort(config.getDatabasePort());
            connectionPool[i].setDatabaseName(config.getDatabaseName());
            connectionPool[i].setUserName(config.getDatabaseUser());
            connectionPool[i].setPassword(config.getDatabasePassword());
        } else {
            connectionPool[i] = QSqlDatabase::addDatabase("QSQLITE", connName);
            connectionPool[i].setDatabaseName("etykiety_db.sqlite");
        }
        
        if (!connectionPool[i].open()) {
            qWarning() << "Failed to open pooled connection" << i << ":" << connectionPool[i].lastError().text();
            emit dbConnectionError(QString("Nie można otworzyć połączenia %1 w puli").arg(i));
        }
        
        connectionAvailable[i] = true;
    }
    
    // --- Dodaj kolumnę 'done' do materials_orders jeśli nie istnieje ---
    QSqlQuery alterQ(db);
    alterQ.exec("PRAGMA table_info(materials_orders)");
    bool hasDone = false;
    while (alterQ.next()) {
        if (alterQ.value(1).toString() == "done") {
            hasDone = true;
            break;
        }
    }
    if (!hasDone) {
        QSqlQuery alterAdd(db);
        alterAdd.exec("ALTER TABLE materials_orders ADD COLUMN done INTEGER DEFAULT 0");
    }
}

QSqlDatabase DbManager::database() {
    return db;
}

bool DbManager::isOpen() {
    return QSqlDatabase::contains("main_conn") && QSqlDatabase::database("main_conn").isOpen();
}

QVector<QMap<QString, QVariant>> DbManager::getClients() {
    qDebug() << "[DbManager::getClients] Pobieram klientów z bazy...";
    QVector<QMap<QString, QVariant>> result;
    QSqlQuery q(db);
    q.exec("SELECT id, client_number, name, short_name, contact_person, phone, email, street, postal_code, city, nip FROM clients ORDER BY name");
    while (q.next()) {
        QMap<QString, QVariant> row;
        row["id"] = q.value(0);
        row["client_number"] = q.value(1);
        row["name"] = q.value(2);
        row["short_name"] = q.value(3);
        row["contact_person"] = q.value(4);
        row["phone"] = q.value(5);
        row["email"] = q.value(6);
        row["street"] = q.value(7);
        row["postal_code"] = q.value(8);
        row["city"] = q.value(9);
        row["nip"] = q.value(10);
        result.append(row);
    }
    qDebug() << "[DbManager::getClients] Liczba klientów w bazie:" << result.size();
    return result;
}

int DbManager::findClientByNip(const QString& nip) {
    // ZAWSZE oczyszczaj NIP do cyfr przed porównaniem
    QString cleanNip = ClientFullDialog::cleanNip(nip);
    QSqlQuery q(db);
    q.prepare("SELECT id, name FROM clients WHERE nip = ?");
    q.addBindValue(cleanNip);
    if (q.exec() && q.next()) {
        int clientId = q.value(0).toInt();
        QString clientName = q.value(1).toString();
        qDebug() << "Znaleziono istniejącego klienta z NIP" << cleanNip << "- ID:" << clientId << "Nazwa:" << clientName;
        return clientId;
    }
    qDebug() << "Nie znaleziono klienta z NIP" << cleanNip;
    return -1; // Nie znaleziono klienta z tym NIP
}

int DbManager::findClientByNumber(const QString &clientNumber) {
    // Zawsze porównuj z wiodącymi zerami (6 cyfr)
    QString formatted = clientNumber;
    if (formatted.length() < 6)
        formatted = formatted.rightJustified(6, '0');
    QSqlQuery q(db);
    q.prepare("SELECT id FROM clients WHERE client_number = ?");
    q.addBindValue(formatted);
    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return -1;
}

bool DbManager::addClient(const QMap<QString, QVariant>& data) {
    QSqlQuery q(db);
    q.prepare("INSERT INTO clients (client_number, name, short_name, contact_person, phone, email, street, postal_code, city, nip) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    q.addBindValue(data.value("client_number"));
    q.addBindValue(data.value("name"));
    q.addBindValue(data.value("short_name"));
    q.addBindValue(data.value("contact_person"));
    q.addBindValue(data.value("phone"));
    q.addBindValue(data.value("email"));
    q.addBindValue(data.value("street"));
    q.addBindValue(data.value("postal_code"));
    q.addBindValue(data.value("city"));
    q.addBindValue(data.value("nip"));
    bool ok = q.exec();
    if (!ok) m_lastError = q.lastError();
    return ok;
}

bool DbManager::updateClient(int id, const QMap<QString, QVariant>& data) {
    QSqlQuery q(db);
    q.prepare("UPDATE clients SET client_number=?, name=?, short_name=?, contact_person=?, phone=?, email=?, street=?, postal_code=?, city=?, nip=? WHERE id=?");
    q.addBindValue(data.value("client_number"));
    q.addBindValue(data.value("name"));
    q.addBindValue(data.value("short_name"));
    q.addBindValue(data.value("contact_person"));
    q.addBindValue(data.value("phone"));
    q.addBindValue(data.value("email"));
    q.addBindValue(data.value("street"));
    q.addBindValue(data.value("postal_code"));
    q.addBindValue(data.value("city"));
    q.addBindValue(data.value("nip"));
    q.addBindValue(id);
    bool ok = q.exec();
    if (!ok) m_lastError = q.lastError();
    return ok;
}

QVector<QMap<QString, QVariant>> DbManager::getOrders() {
    QVector<QMap<QString, QVariant>> result;
    QSqlQuery q(db);
    q.exec("SELECT id, order_number, order_date, delivery_date, client_id, notes, payment_term, status, delivery_company, delivery_street, delivery_postal_code, delivery_city, delivery_contact_person, delivery_phone FROM orders ORDER BY order_date DESC");
    while (q.next()) {
        QMap<QString, QVariant> row;
        row["id"] = q.value(0);
        QString orderNumber = q.value(1).toString();
        QRegularExpression re("^ZAM-\\d{4}-\\d{3}$");
        if (!re.match(orderNumber).hasMatch()) {
            // Automatyczna poprawa: nadaj nowy numer w formacie ZAM-YYYY-NNN
            QDate date = q.value(2).toDate();
            int id = q.value(0).toInt();
            QString newOrderNumber = QString("ZAM-%1-%2").arg(date.year()).arg(id, 3, 10, QChar('0'));
            row["order_number"] = newOrderNumber;
            // Zapisz poprawiony numer do bazy
            QSqlQuery qupdate(db);
            qupdate.prepare("UPDATE orders SET order_number=? WHERE id=?");
            qupdate.addBindValue(newOrderNumber);
            qupdate.addBindValue(id);
            qupdate.exec();
        } else {
            row["order_number"] = orderNumber;
        }
        row["order_date"] = q.value(2);
        row["delivery_date"] = q.value(3);
        row["client_id"] = q.value(4);
        row["notes"] = q.value(5);
        row["payment_term"] = q.value(6);
        row["status"] = q.value(7);
        // Nowe pola adresu dostawy:
        row["delivery_company"] = q.value(8);
        row["delivery_street"] = q.value(9);
        row["delivery_postal_code"] = q.value(10);
        row["delivery_city"] = q.value(11);
        row["delivery_contact_person"] = q.value(12);
        row["delivery_phone"] = q.value(13);
        result.append(row);
    }
    return result;
}

bool DbManager::addOrder(const QMap<QString, QVariant>& orderData, const QVector<QMap<QString, QVariant>>& items) {
    db.transaction();
    QSqlQuery q(db);
    q.prepare("INSERT INTO orders (order_number, order_date, delivery_date, client_id, notes, payment_term, delivery_company, delivery_street, delivery_postal_code, delivery_city, delivery_contact_person, delivery_phone, status) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    q.addBindValue(orderData.value("order_number"));
    q.addBindValue(orderData.value("order_date"));
    q.addBindValue(orderData.value("delivery_date"));
    q.addBindValue(orderData.value("client_id"));
    q.addBindValue(orderData.value("notes"));
    q.addBindValue(orderData.value("payment_term"));
    q.addBindValue(orderData.value("delivery_company"));
    q.addBindValue(orderData.value("delivery_street"));
    q.addBindValue(orderData.value("delivery_postal_code"));
    q.addBindValue(orderData.value("delivery_city"));
    q.addBindValue(orderData.value("delivery_contact_person"));
    q.addBindValue(orderData.value("delivery_phone"));
    q.addBindValue(orderData.contains("status") ? orderData.value("status").toInt() : 0); // Domyślny status: 0 = Przyjęte do realizacji
    if (!q.exec()) { db.rollback(); return false; }
    int orderId = q.lastInsertId().toInt();
    for (const auto &item : items) {
        QSqlQuery q2(db);
        q2.prepare("INSERT INTO order_items (order_id, width, height, material, ordered_quantity, quantity_type, roll_length, core, price, price_type, zam_rolki) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        q2.addBindValue(orderId);
        q2.addBindValue(item.value("width"));
        q2.addBindValue(item.value("height"));
        q2.addBindValue(item.value("material"));
        q2.addBindValue(item.value("ordered_quantity"));
        q2.addBindValue(item.value("quantity_type"));
        q2.addBindValue(item.value("roll_length"));
        q2.addBindValue(item.value("core"));
        q2.addBindValue(item.value("price"));
        q2.addBindValue(item.value("price_type"));
        q2.addBindValue(item.value("zam_rolki"));
        if (!q2.exec()) { db.rollback(); return false; }
    }
    db.commit();
    
    // Emituj sygnał o dodaniu nowego zamówienia
    qDebug() << "=== DbManager: Emituję sygnał orderAdded po zapisaniu zamówienia ===";
    emit orderAdded();
    qDebug() << "=== DbManager: Sygnał orderAdded wysłany ===";
    
    return true;
}

bool DbManager::addMaterialsOrder(const QMap<QString, QVariant>& orderData, const QVector<QMap<QString, QVariant>>& items) {
    QSqlQuery q(db);
    QSqlDatabase::database().transaction();
    // Dodaj zamówienie do materials_orders
    q.prepare("INSERT INTO materials_orders (order_number, order_date, delivery_date, notes, supplier_id, delivery_company, delivery_street, delivery_postal_code, delivery_city, delivery_country, done) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    q.addBindValue(orderData.value("order_number"));
    q.addBindValue(orderData.value("order_date"));
    q.addBindValue(orderData.value("delivery_date"));
    q.addBindValue(orderData.value("notes"));
    q.addBindValue(orderData.value("supplier_id"));
    q.addBindValue(orderData.value("delivery_company"));
    q.addBindValue(orderData.value("delivery_street"));
    q.addBindValue(orderData.value("delivery_postal_code"));
    q.addBindValue(orderData.value("delivery_city"));
    q.addBindValue(orderData.value("delivery_country"));
    q.addBindValue(orderData.contains("done") ? orderData.value("done").toInt() : 0);
    if (!q.exec()) {
        m_lastError = q.lastError();
        QSqlDatabase::database().rollback();
        return false;
    }
    int orderId = q.lastInsertId().toInt();
    // Dodaj pozycje zamówienia do materials_order_items
    QSqlQuery q2(db);
    for (const auto& item : items) {
        q2.prepare("INSERT INTO materials_order_items (order_id, material_id, material_name, width, length, quantity) VALUES (?, ?, ?, ?, ?, ?)");
        q2.addBindValue(orderId);
        q2.addBindValue(item.value("material_id"));
        q2.addBindValue(item.value("material_name"));
        q2.addBindValue(item.value("width"));
        q2.addBindValue(item.value("length"));
        q2.addBindValue(item.value("quantity"));
        if (!q2.exec()) {
            m_lastError = q2.lastError();
            QSqlDatabase::database().rollback();
            return false;
        }
    }
    QSqlDatabase::database().commit();
    emit orderAdded();
    return true;
}

bool DbManager::updateOrder(int id, const QMap<QString, QVariant>& orderData, const QVector<QMap<QString, QVariant>>& items) {
    return executeTransaction([&]() {
        QSqlQuery q(db);
        q.prepare("UPDATE orders SET order_number=?, order_date=?, delivery_date=?, client_id=?, notes=?, payment_term=?, delivery_company=?, delivery_street=?, delivery_postal_code=?, delivery_city=?, delivery_contact_person=?, delivery_phone=? WHERE id=?");
        q.addBindValue(orderData.value("order_number"));
        q.addBindValue(orderData.value("order_date"));
        q.addBindValue(orderData.value("delivery_date"));
        q.addBindValue(orderData.value("client_id"));
        q.addBindValue(orderData.value("notes"));
        q.addBindValue(orderData.value("payment_term"));
        q.addBindValue(orderData.value("delivery_company"));
        q.addBindValue(orderData.value("delivery_street"));
        q.addBindValue(orderData.value("delivery_postal_code"));
        q.addBindValue(orderData.value("delivery_city"));
        q.addBindValue(orderData.value("delivery_contact_person"));
        q.addBindValue(orderData.value("delivery_phone"));
        q.addBindValue(id);
        
        if (!q.exec()) {
            qWarning() << "Błąd aktualizacji zamówienia:" << q.lastError().text();
            m_lastError = q.lastError(); // Ustawienie m_lastError
            return false;
        }
        
        // Usuń stare pozycje
        QSqlQuery qdel(db);
        qdel.prepare("DELETE FROM order_items WHERE order_id=?");
        qdel.addBindValue(id);
        
        if (!qdel.exec()) {
            qWarning() << "Błąd usuwania pozycji zamówienia:" << qdel.lastError().text();
            m_lastError = qdel.lastError(); // Ustawienie m_lastError
            return false;
        }
        
        // Dodaj nowe pozycje
        for (const auto &item : items) {
            QSqlQuery q2(db);
            q2.prepare("INSERT INTO order_items (order_id, width, height, material, ordered_quantity, quantity_type, roll_length, core, price, price_type, zam_rolki) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            q2.addBindValue(id);
            q2.addBindValue(item.value("width"));
            q2.addBindValue(item.value("height"));
            q2.addBindValue(item.value("material"));
            q2.addBindValue(item.value("ordered_quantity"));
            q2.addBindValue(item.value("quantity_type"));
            q2.addBindValue(item.value("roll_length"));
            q2.addBindValue(item.value("core"));
            q2.addBindValue(item.value("price"));
            q2.addBindValue(item.value("price_type"));
            q2.addBindValue(item.value("zam_rolki"));
            
            if (!q2.exec()) {
                qWarning() << "Błąd dodawania pozycji zamówienia:" << q2.lastError().text();
                m_lastError = q2.lastError(); // Ustawienie m_lastError
                return false;
            }
        }
        
        return true;
    });
}

bool DbManager::deleteClient(int id) {
    // Najpierw usuń powiązane adresy dostawy
    QSqlQuery qdel(db);
    qDebug() << "[DEBUG] Usuwanie adresów dostawy klienta id:" << id;
    qdel.prepare("DELETE FROM delivery_addresses WHERE client_id=?");
    qdel.addBindValue(id);
    if (!qdel.exec()) {
        qDebug() << "[DEBUG] Błąd SQL (delete adresy):" << qdel.lastError().text();
        m_lastError = qdel.lastError(); // Ustawienie m_lastError
        return false;
    }
    // Usuń powiązane zamówienia (i pozycje zamówień)
    QSqlQuery qOrders(db);
    qOrders.prepare("SELECT id FROM orders WHERE client_id=?");
    qOrders.addBindValue(id);
    if (qOrders.exec()) {
        while (qOrders.next()) {
            int orderId = qOrders.value(0).toInt();
            deleteOrder(orderId);
        }
    }
    // Usuń klienta
    QSqlQuery q(db);
    q.prepare("DELETE FROM clients WHERE id=?");
    q.addBindValue(id);
    if (!q.exec()) {
        qDebug() << "[DEBUG] Błąd SQL (delete klient):" << q.lastError().text();
        m_lastError = q.lastError(); // Ustawienie m_lastError
        return false;
    }
    return true;
}

bool DbManager::deleteOrder(int id) {
    return executeTransaction([&]() {
        // Usuń elementy zamówienia
        QSqlQuery q1(db);
        q1.prepare("DELETE FROM order_items WHERE order_id=?");
        q1.addBindValue(id);
        
        if (!q1.exec()) {
            qWarning() << "Błąd usuwania pozycji zamówienia:" << q1.lastError().text();
            m_lastError = q1.lastError(); // Ustawienie m_lastError
            return false;
        }
        
        // Usuń zamówienie
        QSqlQuery q2(db);
        q2.prepare("DELETE FROM orders WHERE id=?");
        q2.addBindValue(id);
        
        if (!q2.exec()) {
            qWarning() << "Błąd usuwania zamówienia:" << q2.lastError().text();
            m_lastError = q2.lastError(); // Ustawienie m_lastError
            return false;
        }
        
        return true;
    });
}



bool DbManager::addDeliveryAddress(const QMap<QString, QVariant>& data) {
    if (!db.isOpen()) {
        qWarning() << "Błąd: Brak połączenia z bazą danych";
        return false;
    }
    
    // Sprawdź, czy kolumny istnieją w tabeli
    bool hasCountry = false;
    bool hasNip = false;
    
    if (db.driverName() == "QPSQL") {
        QSqlQuery checkQuery(db);
        if (checkQuery.exec("SELECT column_name FROM information_schema.columns WHERE table_name = 'delivery_addresses'")) {
            while (checkQuery.next()) {
                QString columnName = checkQuery.value(0).toString();
                if (columnName == "country") hasCountry = true;
                if (columnName == "nip") hasNip = true;
            }
        }
    } else {
        // Dla SQLite
        QSqlRecord record = db.record("delivery_addresses");
        hasCountry = record.contains("country");
        hasNip = record.contains("nip");
    }
    
    return executeTransaction([&]() {
        QSqlQuery q(db);
        
        // Buduj dynamicznie zapytanie w zależności od dostępnych kolumn
        QStringList columns = {"client_id", "name", "company", "street", "postal_code", 
                              "city", "contact_person", "phone"};
        QStringList placeholders;
        
        if (hasCountry) columns << "country";
        if (hasNip) columns << "nip";
        
        // Tworzymy placeholdery dla wartości (?, ?, ...)
        for (int i = 0; i < columns.size(); ++i) {
            placeholders << "?";
        }
        
        QString queryStr = QString("INSERT INTO delivery_addresses (%1) VALUES (%2)")
            .arg(columns.join(", "))
            .arg(placeholders.join(", "));
        
        qDebug() << "Zapytanie dodawania adresu dostawy:" << queryStr;
        
        if (!q.prepare(queryStr)) {
            m_lastError = q.lastError();
            qWarning() << "Błąd przygotowania zapytania dodawania adresu dostawy:" << q.lastError().text();
            return false;
        }
        
        // Dodaj wartości w odpowiedniej kolejności
        q.addBindValue(data.value("client_id", QVariant(-1)));
        q.addBindValue(data.value("name"));
        q.addBindValue(data.value("company"));
        q.addBindValue(data.value("street"));
        q.addBindValue(data.value("postal_code"));
        q.addBindValue(data.value("city"));
        q.addBindValue(data.value("contact_person"));
        q.addBindValue(data.value("phone"));
        
        if (hasCountry) {
            q.addBindValue(data.value("country", ""));
        }
        
        if (hasNip) {
            q.addBindValue(data.value("nip", ""));
        }
        
        if (!q.exec()) {
            m_lastError = q.lastError();
            qWarning() << "Błąd dodawania adresu dostawy:" << q.lastError().text();
            return false;
        }
        
        qDebug() << "Pomyślnie dodano nowy adres dostawy";
        return true;
    });
}

bool DbManager::updateDeliveryAddress(const QMap<QString, QVariant>& data) {
    if (!db.isOpen()) {
        qWarning() << "Błąd: Brak połączenia z bazą danych";
        return false;
    }
    
    if (!data.contains("id") || data["id"].toInt() <= 0) {
        qWarning() << "Błąd: Brak poprawnego ID adresu dostawy do aktualizacji";
        return false;
    }
    
    int id = data["id"].toInt();
    
    // Sprawdź, czy kolumny istnieją w tabeli - ulepszona wersja
    bool hasCountry = checkColumnExistence("delivery_addresses", "country");
    bool hasNip = checkColumnExistence("delivery_addresses", "nip");
    
    // Wymuszamy wykrycie kolumn, jeśli wiemy, że istnieją (zgodnie z informacjami użytkownika)
    if (!hasCountry || !hasNip) {
        qDebug() << "Uwaga: Wymuszam wykrycie kolumn 'country' i 'nip' (znane problemy z wykrywaniem)";
        hasCountry = hasNip = true;
    }
    
    qDebug() << "Aktualizacja adresu dostawy ID:" << id << "- wykryte kolumny: country:" << hasCountry << "nip:" << hasNip;
    
    return executeTransaction([&]() {
        QSqlQuery q(db);
        
        // Buduj dynamicznie zapytanie w zależności od dostępnych kolumn
        QStringList setClauses = {
            "name = ?", 
            "company = ?", 
            "street = ?", 
            "postal_code = ?", 
            "city = ?",
            "contact_person = ?", 
            "phone = ?"
        };
        
        // Dodaj opcjonalne kolumny, jeśli istnieją
        if (hasCountry) setClauses << "country = ?";
        if (hasNip) setClauses << "nip = ?";
        
        // Zbuduj podstawowe zapytanie
        QString queryStr = "UPDATE delivery_addresses SET " + setClauses.join(", ") + " WHERE id = ?";
        
        qDebug() << "Zapytanie aktualizacji adresu dostawy:" << queryStr;
        
        if (!q.prepare(queryStr)) {
            m_lastError = q.lastError();
            qWarning() << "Błąd przygotowania zapytania aktualizacji adresu dostawy:" << q.lastError().text();
            return false;
        }
        
        // Dodaj wartości w odpowiedniej kolejności (zgodnie z kolejnością w setClauses)
        q.addBindValue(data.value("name"));
        q.addBindValue(data.value("company"));
        q.addBindValue(data.value("street"));
        q.addBindValue(data.value("postal_code"));
        q.addBindValue(data.value("city"));
        q.addBindValue(data.value("contact_person"));
        q.addBindValue(data.value("phone"));
        
        // Dodaj opcjonalne wartości, jeśli kolumny istnieją
        if (hasCountry) {
            q.addBindValue(data.value("country", ""));
        }
        
        if (hasNip) {
            q.addBindValue(data.value("nip", ""));
        }
        
        // Dodaj ID warunku WHERE
        q.addBindValue(id);
        
        qDebug() << "Wartości do aktualizacji:" 
                 << "name:" << data.value("name").toString()
                 << "company:" << data.value("company").toString()
                 << "street:" << data.value("street").toString()
                 << "postal_code:" << data.value("postal_code").toString()
                 << "city:" << data.value("city").toString()
                 << "contact_person:" << data.value("contact_person").toString()
                 << "phone:" << data.value("phone").toString()
                 << "country:" << (hasCountry ? data.value("country", "").toString() : "[niedostępne]")
                 << "nip:" << (hasNip ? data.value("nip", "").toString() : "[niedostępne]")
                 << "id:" << id;
        
        if (!q.exec()) {
            m_lastError = q.lastError();
            qWarning() << "Błąd wykonania zapytania aktualizacji adresu dostawy:" << q.lastError().text();
            qWarning() << "Szczegóły błędu:" << q.lastError().databaseText();
            qWarning() << "ID adresu dostawy:" << id;
            qWarning() << "Dane do aktualizacji:" << data;
            return false;
        }
        
        int rowsAffected = q.numRowsAffected();
        if (rowsAffected == 0) {
            qWarning() << "Nie znaleziono adresu dostawy o ID:" << id << "do aktualizacji";
            return false;
        }
        
        qDebug() << "Pomyślnie zaktualizowano adres dostawy ID:" << id << ", zmodyfikowanych wierszy:" << rowsAffected;
        return true;
    });
}

bool DbManager::migrateDeliveryAddresses() {
    if (!db.isOpen()) {
        qWarning() << "Błąd: Brak połączenia z bazą danych";
        return false;
    }
    
    return executeTransaction([&]() {
        QSqlQuery q(db);
        
        // Sprawdź czy kolumny istnieją
        bool hasCountry = false;
        bool hasNip = false;
        
        if (db.driverName() == "QPSQL") {
            QSqlQuery checkColQuery(db);
            if (checkColQuery.exec("SELECT column_name FROM information_schema.columns WHERE table_name = 'delivery_addresses' AND column_name = 'country'")) {
                hasCountry = checkColQuery.next();
            }
            if (checkColQuery.exec("SELECT column_name FROM information_schema.columns WHERE table_name = 'delivery_addresses' AND column_name = 'nip'")) {
                hasNip = checkColQuery.next();
            }
        } else {
            QSqlRecord record = db.record("delivery_addresses");
            hasCountry = record.contains("country");
            hasNip = record.contains("nip");
        }
        
        if (!hasCountry || !hasNip) {
            qWarning() << "Błąd: Brak wymaganych kolumn w tabeli delivery_addresses";
            return false;
        }
        
        // Aktualizuj puste wartości dla istniejących rekordów
        if (!q.exec("UPDATE delivery_addresses SET country = 'Polska' WHERE country IS NULL OR country = ''")) {
            qWarning() << "Błąd aktualizacji pustych wartości country:" << q.lastError().text();
            return false;
        }
        
        // Dla NIP ustawiamy pusty string zamiast NULL, jeśli jest NULL
        if (!q.exec("UPDATE delivery_addresses SET nip = '' WHERE nip IS NULL")) {
            qWarning() << "Błąd aktualizacji pustych wartości nip:" << q.lastError().text();
            return false;
        }
        
        qDebug() << "Pomyślnie zaktualizowano dane w tabeli delivery_addresses";
        return true;
    });
}

int DbManager::getMaxClientNumber() {
    QSqlQuery q(db);
    q.exec("SELECT MAX(client_number::integer) FROM clients");
    if (q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

bool DbManager::addClientWithAddresses(const QMap<QString, QVariant>& data, const QList<QMap<QString, QVariant>>& addresses) {
    QMap<QString, QVariant> cleanData = data;
    cleanData["nip"] = ClientFullDialog::cleanNip(data.value("nip").toString());
    QSqlQuery q(db);
    db.transaction();
    qDebug() << "[DEBUG] Dodawanie klienta:" << cleanData;
    q.prepare("INSERT INTO clients (client_number, name, short_name, contact_person, phone, email, street, postal_code, city, nip) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    q.addBindValue(cleanData.value("client_number"));
    q.addBindValue(cleanData.value("name"));
    q.addBindValue(cleanData.value("short_name"));
    q.addBindValue(cleanData.value("contact_person"));
    q.addBindValue(cleanData.value("phone"));
    q.addBindValue(cleanData.value("email"));
    q.addBindValue(cleanData.value("street"));
    q.addBindValue(cleanData.value("postal_code"));
    q.addBindValue(cleanData.value("city"));
    q.addBindValue(cleanData.value("nip"));
    if (!q.exec()) {
        qDebug() << "[DEBUG] Błąd SQL (klient):" << q.lastError().text();
        m_lastError = q.lastError(); // Ustawienie m_lastError
        db.rollback();
        return false;
    }
    int clientId = 0;
    QSqlQuery qid(db);
    qid.exec("SELECT currval(pg_get_serial_sequence('clients','id'))");
    if (qid.next()) clientId = qid.value(0).toInt();
    QList<QMap<QString, QVariant>> addressesToAdd = addresses;
    // Dodaj domyślny adres jeśli nie ma żadnego adresu z company == short_name
    bool hasDefault = false;
    for (const auto& addr : addressesToAdd) {
        if (addr.value("company").toString() == data.value("short_name").toString()) {
            hasDefault = true;
            break;
        }
    }
    if (!hasDefault) {
        QMap<QString, QVariant> defaultAddr;
        defaultAddr["company"] = data.value("short_name");
        defaultAddr["street"] = data.value("street");
        defaultAddr["postal_code"] = data.value("postal_code");
        defaultAddr["city"] = data.value("city");
        defaultAddr["contact_person"] = data.value("contact_person");
        defaultAddr["phone"] = data.value("phone");
        addressesToAdd.append(defaultAddr);
    }
    for (const auto& addr : addressesToAdd) {
        QSqlQuery qa(db);
        qa.prepare("INSERT INTO delivery_addresses (client_id, company, street, postal_code, city, contact_person, phone) VALUES (?, ?, ?, ?, ?, ?, ?)");
        qa.addBindValue(clientId);
        qa.addBindValue(addr.value("company"));
        qa.addBindValue(addr.value("street"));
        qa.addBindValue(addr.value("postal_code"));
        qa.addBindValue(addr.value("city"));
        qa.addBindValue(addr.value("contact_person"));
        qa.addBindValue(addr.value("phone"));
        if (!qa.exec()) {
            qDebug() << "[DEBUG] Błąd SQL (adres dostawy):" << qa.lastError().text();
            m_lastError = qa.lastError();
            db.rollback();
            return false;
        }
    }
    if (!db.commit()) {
        qDebug() << "[DEBUG] Błąd przy commitowaniu transakcji po dodaniu klienta:" << db.lastError().text();
        db.rollback();
        return false;
    }
    return true;
}

int DbManager::getNextUniqueClientNumber() {
    QSqlQuery q(db);
    int nextNr = 0;
    qDebug() << "[DEBUG] Szukam największego numeru klienta w used_client_numbers i clients";
    q.exec("SELECT GREATEST(COALESCE((SELECT MAX(client_number) FROM used_client_numbers),0), COALESCE((SELECT MAX(client_number::integer) FROM clients),0))");
    if (q.next()) {
        nextNr = q.value(0).toInt();
    }
    qDebug() << "[DEBUG] Największy numer klienta:" << nextNr;
    // Szukaj kolejnego wolnego numeru, który nie istnieje w clients
    while (true) {
        ++nextNr;
        QString candidate = QString::number(nextNr).rightJustified(6, '0');
        QSqlQuery check(db);
        check.prepare("SELECT COUNT(*) FROM clients WHERE client_number=?");
        check.addBindValue(candidate);
        check.exec();
        check.next();
        int existsInClients = check.value(0).toInt();
        // Sprawdź, czy numer jest w used_client_numbers, ale tylko jeśli istnieje w clients
        if (existsInClients == 0) {
            break;
        }
    }
    qDebug() << "[DEBUG] Nowy numer klienta do przydzielenia:" << nextNr;
    return nextNr;
}

void DbManager::markClientNumberUsed(int clientNumber) {
    QSqlQuery q(db);
    q.prepare("INSERT INTO used_client_numbers (client_number) VALUES (?) ON CONFLICT DO NOTHING");
    q.addBindValue(clientNumber);
    q.exec();
}

bool DbManager::updateClientWithAddresses(int id, const QMap<QString, QVariant>& data, const QList<QMap<QString, QVariant>>& addresses) {
    QMap<QString, QVariant> cleanData = data;
    cleanData["nip"] = ClientFullDialog::cleanNip(data.value("nip").toString());
    db.transaction();
    QSqlQuery q(db);
    qDebug() << "[DEBUG] Aktualizacja klienta id:" << id << cleanData;
    q.prepare("UPDATE clients SET client_number=?, name=?, short_name=?, contact_person=?, phone=?, email=?, street=?, postal_code=?, city=?, nip=? WHERE id=?");
    q.addBindValue(cleanData.value("client_number"));
    q.addBindValue(cleanData.value("name"));
    q.addBindValue(cleanData.value("short_name"));
    q.addBindValue(cleanData.value("contact_person"));
    q.addBindValue(cleanData.value("phone"));
    q.addBindValue(cleanData.value("email"));
    q.addBindValue(cleanData.value("street"));
    q.addBindValue(cleanData.value("postal_code"));
    q.addBindValue(cleanData.value("city"));
    q.addBindValue(cleanData.value("nip"));
    q.addBindValue(id);
    if (!q.exec()) { qDebug() << "[DEBUG] Błąd SQL (update klient):" << q.lastError().text(); db.rollback(); return false; }
    // Usuń stare adresy
    QSqlQuery qdel(db);
    qDebug() << "[DEBUG] Usuwanie starych adresów klienta id:" << id;
    qdel.prepare("DELETE FROM delivery_addresses WHERE client_id=?");
    qdel.addBindValue(id);
    if (!qdel.exec()) { qDebug() << "[DEBUG] Błąd SQL (delete adresy):" << qdel.lastError().text(); db.rollback(); return false; }
    // Dodaj nowe adresy (z polem name)
    for (const auto& addr : addresses) {
        QSqlQuery qa(db);
        qDebug() << "[DEBUG] Dodawanie adresu dostawy (edycja):" << addr;
        qa.prepare("INSERT INTO delivery_addresses (client_id, name, company, street, postal_code, city, contact_person, phone) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
        qa.addBindValue(id);
        qa.addBindValue(addr.value("name"));
        qa.addBindValue(addr.value("company"));
        qa.addBindValue(addr.value("street"));
        qa.addBindValue(addr.value("postal_code"));
        qa.addBindValue(addr.value("city"));
        qa.addBindValue(addr.value("contact_person"));
        qa.addBindValue(addr.value("phone"));
        if (!qa.exec()) { qDebug() << "[DEBUG] Błąd SQL (adres - edycja):" << qa.lastError().text(); db.rollback(); return false; }
    }
    db.commit();
    return true;
}

bool DbManager::updateOrderDeliveryDate(int id, const QDate& newDate) {
    QSqlQuery q(db);
    q.prepare("UPDATE orders SET delivery_date=? WHERE id=?");
    q.addBindValue(newDate);
    q.addBindValue(id);
    return q.exec();
}

QVector<QMap<QString, QVariant>> DbManager::getDeliveryAddresses(int clientId) {
    QVector<QMap<QString, QVariant>> result;
    
    if (!db.isOpen()) {
        qWarning() << "Błąd: Brak połączenia z bazą danych";
        return result;
    }
    
    // Sprawdź, czy kolumny istnieją w tabeli
    bool hasCountry = false;
    bool hasNip = false;
    
    if (db.driverName() == "QPSQL") {
        QSqlQuery checkQuery(db);
        if (checkQuery.exec("SELECT column_name FROM information_schema.columns WHERE table_name = 'delivery_addresses'")) {
            while (checkQuery.next()) {
                QString columnName = checkQuery.value(0).toString();
                if (columnName == "country") hasCountry = true;
                if (columnName == "nip") hasNip = true;
            }
        } else {
            qWarning() << "Błąd sprawdzania kolumn w getDeliveryAddresses:" << checkQuery.lastError().text();
        }
    } else {
        // Dla SQLite
        QSqlRecord record = db.record("delivery_addresses");
        hasCountry = record.contains("country");
        hasNip = record.contains("nip");
    }
    
    QSqlQuery query(db);
    QString queryStr = "SELECT id, name, company, street, postal_code, city, contact_person, phone";
    
    // Dodaj tylko istniejące kolumny do zapytania
    if (hasCountry) queryStr += ", country";
    if (hasNip) queryStr += ", nip";
    
    queryStr += " FROM delivery_addresses";
    
    if (clientId > 0) {
        queryStr += " WHERE client_id = ?";
    }
    
    queryStr += " ORDER BY name, company";
    
    if (!query.prepare(queryStr)) {
        qWarning() << "Błąd przygotowania zapytania w getDeliveryAddresses:" << query.lastError().text();
        return result;
    }
    
    if (clientId > 0) {
        query.addBindValue(clientId);
    }
    
    if (!query.exec()) {
        qWarning() << "Błąd wykonania zapytania w getDeliveryAddresses:" << query.lastError().text();
        return result;
    }
    
    while (query.next()) {
        QMap<QString, QVariant> address;
        address["id"] = query.value("id");
        address["name"] = query.value("name");
        address["company"] = query.value("company");
        address["street"] = query.value("street");
        address["postal_code"] = query.value("postal_code");
        address["city"] = query.value("city");
        address["contact_person"] = query.value("contact_person");
        address["phone"] = query.value("phone");
        
        // Dodaj tylko istniejące kolumny
        if (hasCountry) {
            address["country"] = query.value("country");
        } else {
            address["country"] = "";
        }
        
        if (hasNip) {
            address["nip"] = query.value("nip");
        } else {
            address["nip"] = "";
        }
        
        result.append(address);
        
        qDebug() << "Pobrano adres dostawy:" << address["company"] << "-" << address["street"] << "," << address["postal_code"] << address["city"];
    }
    
    qDebug() << "Pobrano łącznie" << result.size() << "adresów dostawy";
    return result;
}

QVector<QMap<QString, QVariant>> DbManager::getOrderItems(int orderId) {
    QVector<QMap<QString, QVariant>> items;
    
    QSqlQuery query(db);
    query.prepare("SELECT id, order_id, product_name, quantity, unit, price, vat_rate, notes "
                 "FROM order_items WHERE order_id = ? ORDER BY id");
    query.addBindValue(orderId);
    
    if (!query.exec()) {
        qWarning() << "Błąd podczas pobierania pozycji zamówienia:" << query.lastError().text();
        return items;
    }
    
    while (query.next()) {
        QMap<QString, QVariant> item;
        item["id"] = query.value(0);
        item["order_id"] = query.value(1);
        item["product_name"] = query.value(2);
        item["quantity"] = query.value(3);
        item["unit"] = query.value(4);
        item["price"] = query.value(5);
        item["vat_rate"] = query.value(6);
        item["notes"] = query.value(7);
        
        // Oblicz wartość netto i brutto
        double quantity = item["quantity"].toDouble();
        double price = item["price"].toDouble();
        double vatRate = item["vat_rate"].toDouble();
        double netValue = quantity * price;
        double vatValue = netValue * (vatRate / 100.0);
        double grossValue = netValue + vatValue;
        
        item["net_value"] = netValue;
        item["vat_value"] = vatValue;
        item["gross_value"] = grossValue;
        
        items.append(item);
    }
    
    return items;
}

QMap<QString, QVariant> DbManager::getOrderById(int orderId) {
    QMap<QString, QVariant> result;
    QSqlQuery q(db);
    q.prepare("SELECT id, order_number, order_date, delivery_date, client_id, notes, payment_term, "
              "delivery_company, delivery_street, delivery_postal_code, delivery_city, "
              "delivery_contact_person, delivery_phone, status FROM orders WHERE id = ?");
    q.addBindValue(orderId);
    
    if (q.exec() && q.next()) {
        result["id"] = q.value(0);
        result["order_number"] = q.value(1);
        result["order_date"] = q.value(2).toDate();
        result["delivery_date"] = q.value(3).toDate();
        result["client_id"] = q.value(4);
        result["notes"] = q.value(5);
        result["payment_term"] = q.value(6);
        result["delivery_company"] = q.value(7);
        result["delivery_street"] = q.value(8);
        result["delivery_postal_code"] = q.value(9);
        result["delivery_city"] = q.value(10);
        result["delivery_contact_person"] = q.value(11);
        result["delivery_phone"] = q.value(12);
        result["status"] = q.value(13);
        
        // Dodaj dane klienta
        int clientId = q.value(4).toInt();
        QSqlQuery clientQuery(db);
        clientQuery.prepare("SELECT name, short_name, email, phone FROM clients WHERE id = ?");
        clientQuery.addBindValue(clientId);
        
        if (clientQuery.exec() && clientQuery.next()) {
            result["client_name"] = clientQuery.value(0);
            result["client_short_name"] = clientQuery.value(1);
            result["client_email"] = clientQuery.value(2);
            result["client_phone"] = clientQuery.value(3);
        }
        
        // Dodaj pozycje zamówienia
        auto items = getOrderItems(orderId);
        QVariantList itemsList;
        for (const auto& item : items) {
            itemsList.append(QVariant::fromValue(item));
        }
        result["items"] = itemsList;
    }
    
    return result;
}

void DbManager::initializeTables() {
    if (!db.isOpen()) return;
    
    QSqlQuery query(db);
    
    // Create basic clients table
    QString createClients = R"(
        CREATE TABLE IF NOT EXISTS clients (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            client_number INTEGER UNIQUE,
            name TEXT NOT NULL,
            nip TEXT,
            regon TEXT,
            address TEXT,
            city TEXT,
            postal_code TEXT,
            country TEXT,
            phone TEXT,
            email TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!query.exec(createClients)) {
        qDebug() << "Failed to create clients table:" << query.lastError().text();
    }
    
    // Create basic orders table
    QString createOrders = R"(
        CREATE TABLE IF NOT EXISTS orders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            order_number TEXT UNIQUE,
            client_id INTEGER,
            status TEXT DEFAULT 'pending',
            delivery_date DATE,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (client_id) REFERENCES clients (id)
        )
    )";
    
    if (!query.exec(createOrders)) {
        qDebug() << "Failed to create orders table:" << query.lastError().text();
    }
    
    // Create basic order_items table
    QString createOrderItems = R"(
        CREATE TABLE IF NOT EXISTS order_items (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            order_id INTEGER,
            product_name TEXT,
            quantity INTEGER,
            unit_price REAL,
            FOREIGN KEY (order_id) REFERENCES orders (id)
        )
    )";
    
    if (!query.exec(createOrderItems)) {
        qDebug() << "Failed to create order_items table:" << query.lastError().text();
    }
    
    // --- Tabele dla dostawców, materiałów i zamówień materiałów ---
    // Create suppliers table
    QString createSuppliers = R"(
        CREATE TABLE IF NOT EXISTS suppliers (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            street TEXT,
            city TEXT,
            postal_code TEXT,
            country TEXT,
            contact_person TEXT,
            phone TEXT,
            email TEXT
        )
    )";
    if (!query.exec(createSuppliers)) {
        qDebug() << "Failed to create suppliers table:" << query.lastError().text();
    }
    
    // Create materials_catalog table
    QString createMaterialsCatalog = R"(
        CREATE TABLE IF NOT EXISTS materials_catalog (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            width REAL,
            length REAL,
            unit TEXT
        )
    )";
    if (!query.exec(createMaterialsCatalog)) {
        qDebug() << "Failed to create materials_catalog table:" << query.lastError().text();
    }
    
    // Create materials_orders table
    QString createMaterialsOrders = R"(
        CREATE TABLE IF NOT EXISTS materials_orders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            order_number TEXT UNIQUE,
            order_date DATE,
            delivery_date DATE,
            notes TEXT,
            supplier_id INTEGER,
            delivery_company TEXT,
            delivery_street TEXT,
            delivery_postal_code TEXT,
            delivery_city TEXT,
            delivery_country TEXT,
            FOREIGN KEY (supplier_id) REFERENCES suppliers (id)
        )
    )";
    if (!query.exec(createMaterialsOrders)) {
        qDebug() << "Failed to create materials_orders table:" << query.lastError().text();
    }
    
    // Create materials_order_items table
    QString createMaterialsOrderItems = R"(
        CREATE TABLE IF NOT EXISTS materials_order_items (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            order_id INTEGER,
            material_id INTEGER,
            material_name TEXT,
            width REAL,
            length REAL,
            quantity REAL,
            FOREIGN KEY (order_id) REFERENCES materials_orders (id)
        )
    )";
    if (!query.exec(createMaterialsOrderItems)) {
        qDebug() << "Failed to create materials_order_items table:" << query.lastError().text();
    }
    
    // Sprawdź i zaktualizuj strukturę tabeli delivery_addresses, jeśli jest już utworzona
    if (db.tables().contains("delivery_addresses")) {
        // Sprawdź, czy kolumna 'country' istnieje
        bool hasCountry = false;
        bool hasNip = false;
        
        // Inicjalizacja zmiennej record przed blokiem warunkowym
        QSqlRecord record;
        
        // Sprawdź typ bazy danych
        bool isPostgreSQL = db.driverName() == "QPSQL";
        
        if (isPostgreSQL) {
            // Dla PostgreSQL używamy zapytania informacyjnego
            QSqlQuery checkColQuery(db);
            
            // Sprawdź czy kolumna 'country' istnieje
            if (checkColQuery.exec("SELECT column_name FROM information_schema.columns WHERE table_name = 'delivery_addresses' AND column_name = 'country'")) {
                hasCountry = checkColQuery.next();
                qDebug() << "Czy kolumna 'country' istnieje?" << hasCountry;
            } else {
                qWarning() << "Błąd sprawdzania kolumny 'country':" << checkColQuery.lastError().text();
            }
            
            // Sprawdź czy kolumna 'nip' istnieje
            if (checkColQuery.exec("SELECT column_name FROM information_schema.columns WHERE table_name = 'delivery_addresses' AND column_name = 'nip'")) {
                hasNip = checkColQuery.next();
                qDebug() << "Czy kolumna 'nip' istnieje?" << hasNip;
            } else {
                qWarning() << "Błąd sprawdzania kolumny 'nip':" << checkColQuery.lastError().text();
            }
            
            // Dodatkowe debugowanie - wyświetl wszystkie kolumny w tabeli
            QSqlQuery debugQuery(db);
            if (debugQuery.exec("SELECT column_name, data_type FROM information_schema.columns WHERE table_name = 'delivery_addresses'")) {
                qDebug() << "Kolumny w tabeli delivery_addresses:";
                while (debugQuery.next()) {
                    qDebug() << "- " << debugQuery.value(0).toString() << " (" << debugQuery.value(1).toString() << ")";
                }
            } else {
                qWarning() << "Błąd pobierania listy kolumn:" << debugQuery.lastError().text();
            }
        } else {
            // Dla SQLite używamy standardowego podejścia
            record = db.record("delivery_addresses");
            hasCountry = record.contains("country");
            hasNip = record.contains("nip");
            
            qDebug() << "SQLite - Kolumny w tabeli delivery_addresses:";
            for (int i = 0; i < record.count(); ++i) {
                qDebug() << "- " << record.fieldName(i);
            }
        }
        
        // Dodaj brakujące kolumny
        if (!hasCountry) {
            QString alterQuery;
            if (isPostgreSQL) {
                // Dla PostgreSQL używamy osobnego bloku DO dla każdej kolumny
                alterQuery = 
                    "DO $$ "
                    "BEGIN "
                    "   IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_name = 'delivery_addresses' AND column_name = 'country') THEN "
                    "       ALTER TABLE delivery_addresses ADD COLUMN country TEXT; "
                    "       RAISE NOTICE 'Dodano kolumnę country do tabeli delivery_addresses'; "
                    "   END IF; "
                    "END $$;";
            } else {
                // Dla SQLite
                alterQuery = "ALTER TABLE delivery_addresses ADD COLUMN country TEXT";
            }
            
            if (!query.exec(alterQuery)) {
                qWarning() << "Nie udało się dodać kolumny 'country' do tabeli delivery_addresses:" << query.lastError().text();
            } else {
                qDebug() << "Dodano kolumnę 'country' do tabeli delivery_addresses";
                hasCountry = true; // Oznacz jako dodaną, aby uniknąć ponownych prób
            }
        }
        
        if (!hasNip) {
            QString alterQuery;
            if (isPostgreSQL) {
                // Dla PostgreSQL używamy osobnego bloku DO dla każdej kolumny
                alterQuery = 
                    "DO $$ "
                    "BEGIN "
                    "   IF NOT EXISTS (SELECT 1 FROM information_schema.columns WHERE table_name = 'delivery_addresses' AND column_name = 'nip') THEN "
                    "       ALTER TABLE delivery_addresses ADD COLUMN nip TEXT; "
                    "       RAISE NOTICE 'Dodano kolumnę nip do tabeli delivery_addresses'; "
                    "   END IF; "
                    "END $$;";
            } else {
                // Dla SQLite
                alterQuery = "ALTER TABLE delivery_addresses ADD COLUMN nip TEXT";
            }
            
            if (!query.exec(alterQuery)) {
                qWarning() << "Nie udało się dodać kolumny 'nip' do tabeli delivery_addresses:" << query.lastError().text();
            } else {
                qDebug() << "Dodano kolumnę 'nip' do tabeli delivery_addresses";
                hasNip = true; // Oznacz jako dodaną, aby uniknąć ponownych prób
            }
        }
    } else {
        // Utwórz nową tabelę z wszystkimi wymaganymi kolumnami
        QString createDeliveryAddresses = 
            "CREATE TABLE IF NOT EXISTS delivery_addresses (\n"
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
            "    client_id INTEGER,\n"
            "    name TEXT,\n"
            "    company TEXT,\n"
            "    street TEXT,\n"
            "    postal_code TEXT,\n"
            "    city TEXT,\n"
            "    contact_person TEXT,\n"
            "    phone TEXT,\n"
            "    country TEXT,\n"
            "    nip TEXT,\n"
            "    FOREIGN KEY (client_id) REFERENCES clients (id)\n"
            ")";
        
        if (!query.exec(createDeliveryAddresses)) {
            qDebug() << "Failed to create delivery_addresses table:" << query.lastError().text();
        } else {
            qDebug() << "Created delivery_addresses table with all required columns";
            
            // Po utworzeniu nowej tabeli, ustaw domyślne wartości dla istniejących rekordów
            if (migrateDeliveryAddresses()) {
                qDebug() << "Successfully migrated existing delivery addresses data";
            } else {
                qWarning() << "Failed to migrate existing delivery addresses data";
            }
        }
    }
    
    qDebug() << "Database tables initialized successfully";
}

int DbManager::getAvailableConnection() {
    for (int i = 0; i < POOL_SIZE; i++) {
        if (connectionAvailable[i]) {
            connectionAvailable[i] = false;
            return i;
        }
    }
    return -1; // Brak dostępnych połączeń
}

void DbManager::releaseConnection(int connectionId) {
    if (connectionId >= 0 && connectionId < POOL_SIZE) {
        connectionAvailable[connectionId] = true;
    }
}

QSqlDatabase DbManager::getPooledConnection() {
    int connId = getAvailableConnection();
    if (connId >= 0) {
        return connectionPool[connId];
    }
    
    // Jeśli brak dostępnych połączeń, zwracamy główne połączenie
    return db;
}

bool DbManager::checkColumnExistence(const QString& tableName, const QString& columnName) const {
    if (!db.isOpen()) {
        qWarning() << "Błąd: Brak połączenia z bazą danych";
        return false;
    }
    
    // Metoda 1: Sprawdź przez QSqlRecord
    QSqlRecord record = db.record(tableName);
    if (record.contains(columnName)) {
        qDebug() << "Kolumna" << columnName << "znaleziona w tabeli" << tableName << "poprzez QSqlRecord";
        return true;
    }
    
    // Metoda 2: Sprawdź przez information_schema (dla PostgreSQL)
    if (db.driverName() == "QPSQL") {
        QSqlQuery query(db);
        QString queryStr = QString(
            "SELECT EXISTS ("
            "   SELECT 1 FROM information_schema.columns "
            "   WHERE table_name = :tableName AND column_name = :columnName"
            ")"
        );
        
        query.prepare(queryStr);
        query.bindValue(":tableName", tableName);
        query.bindValue(":columnName", columnName);
        
        if (query.exec() && query.next()) {
            bool exists = query.value(0).toBool();
            qDebug() << "Sprawdzenie kolumny" << columnName << "w tabeli" << tableName 
                     << "przez information_schema:" << (exists ? "istnieje" : "nie istnieje");
            return exists;
        } else {
            qWarning() << "Błąd podczas sprawdzania kolumny" << columnName 
                      << "w tabeli" << tableName << ":" << query.lastError().text();
        }
    }
    
    // Metoda 3: Sprawdź przez wykonanie próbnego zapytania (ostateczność)
    QSqlQuery testQuery(db);
    QString testQueryStr = QString("SELECT %1 FROM %2 LIMIT 1").arg(columnName, tableName);
    
    if (testQuery.exec(testQueryStr)) {
        qDebug() << "Kolumna" << columnName << "istnieje w tabeli" << tableName 
                 << "(sprawdzenie przez testowe zapytanie)";
        return true;
    } else if (testQuery.lastError().isValid()) {
        qDebug() << "Kolumna" << columnName << "prawdopodobnie nie istnieje w tabeli" 
                 << tableName << ":" << testQuery.lastError().text();
    }
    
    qWarning() << "Nie udało się potwierdzić istnienia kolumny" << columnName 
               << "w tabeli" << tableName;
    return false;
}

void DbManager::returnPooledConnection(QSqlDatabase& connection) {
    QString connectionName = connection.connectionName();
    if (connectionName.startsWith("pool_conn_")) {
        int connId = connectionName.mid(10).toInt();
        releaseConnection(connId);
    }
}

bool DbManager::executeTransaction(const std::function<bool()>& operation) {
    db.transaction();
    
    bool success = false;
    try {
        success = operation();
        
        if (success) {
            db.commit();
        } else {
            db.rollback();
        }
    } catch (const std::exception& e) {
        db.rollback();
        qWarning() << "Transaction error:" << e.what();
        success = false;
    } catch (...) {
        db.rollback();
        qWarning() << "Unknown transaction error";
        success = false;
    }
    
    return success;
}

bool DbManager::clearAllOrders() {
    qDebug() << "=== Rozpoczynanie czyszczenia bazy danych ===";
    qDebug() << "Typ bazy danych:" << db.driverName();
    qDebug() << "Nazwa bazy:" << db.databaseName();
    
    // Sprawdź liczbę zamówień przed usunięciem
    int countBefore = getOrdersCount();
    qDebug() << "Liczba zamówień przed usunięciem:" << countBefore;
    
    bool success = executeTransaction([this]() -> bool {
        // Najpierw usuń wszystkie pozycje zamówień
        QSqlQuery q1(db);
        qDebug() << "Usuwanie pozycji zamówień...";
        if (!q1.exec("DELETE FROM order_items")) {
            qWarning() << "Błąd usuwania pozycji zamówień:" << q1.lastError().text();
            m_lastError = q1.lastError(); // Ustawienie m_lastError
            return false;
        }
        int deletedItems = q1.numRowsAffected();
        qDebug() << "Usunięto" << deletedItems << "pozycji zamówień";
        
        // Następnie usuń wszystkie zamówienia
        QSqlQuery q2(db);
        qDebug() << "Usuwanie zamówień...";
        if (!q2.exec("DELETE FROM orders")) {
            qWarning() << "Błąd usuwania zamówień:" << q2.lastError().text();
            m_lastError = q2.lastError(); // Ustawienie m_lastError
            return false;
        }
        int deletedOrders = q2.numRowsAffected();
        qDebug() << "Usunięto" << deletedOrders << "zamówień";
        
        // Zresetuj licznik autoincrement dla ID (tylko dla SQLite)
        if (db.driverName() == "QSQLITE") {
            QSqlQuery q3(db);
            if (!q3.exec("DELETE FROM sqlite_sequence WHERE name='orders'")) {
                qDebug() << "Informacja: nie można zresetować licznika ID dla zamówień";
            }
            
            QSqlQuery q4(db);
            if (!q4.exec("DELETE FROM sqlite_sequence WHERE name='order_items'")) {
                qDebug() << "Informacja: nie można zresetować licznika ID dla pozycji zamówień";
            }
        } else if (db.driverName() == "QPSQL") {
            // Dla PostgreSQL można zresetować sekwencje
            QSqlQuery q3(db);
            q3.exec("ALTER SEQUENCE orders_id_seq RESTART WITH 1");
            
            QSqlQuery q4(db);
            q4.exec("ALTER SEQUENCE order_items_id_seq RESTART WITH 1");
        }
        
        qDebug() << "Wszystkie zamówienia zostały usunięte z bazy danych";
        return true;
    });
    
    // Sprawdź liczbę zamówień po usunięciu
    int countAfter = getOrdersCount();
    qDebug() << "Liczba zamówień po usunięciu:" << countAfter;
    qDebug() << "=== Zakończenie czyszczenia bazy danych (sukces:" << success << ") ===";
    
    // Jeśli operacja się powiodła, wyślij sygnał do odświeżenia wszystkich widoków
    if (success) {
        emit orderAdded(); // To spowoduje odświeżenie zestawienia produkcji i innych widoków
        qDebug() << "Wysłano sygnał do odświeżenia wszystkich widoków";
    }
    
    return success;
}

int DbManager::getOrdersCount() {
    QSqlQuery q(db);
    if (q.exec("SELECT COUNT(*) FROM orders")) {
        if (q.next()) {
            int count = q.value(0).toInt();
            qDebug() << "Liczba zamówień w bazie danych:" << count;
            return count;
        }
    } else {
        qWarning() << "Błąd podczas sprawdzania liczby zamówień:" << q.lastError().text();
    }
    return -1; // Błąd
}

QSqlError DbManager::lastError() const {
    return m_lastError;
}

QString DbManager::getClientNameById(int clientId) const {
    QSqlQuery q(db);
    q.prepare("SELECT name FROM clients WHERE id = ?");
    q.addBindValue(clientId);
    if (q.exec() && q.next()) {
        return q.value(0).toString();
    }
    return QString();
}

// --- CRUD dla dostawców (suppliers) ---
QVector<QMap<QString, QVariant>> DbManager::getSuppliers() {
    QVector<QMap<QString, QVariant>> result;
    QSqlQuery q(db);
    q.exec("SELECT id, name, street, city, postal_code, country, contact_person, phone, email FROM suppliers ORDER BY name");
    while (q.next()) {
        QMap<QString, QVariant> row;
        row["id"] = q.value(0);
        row["name"] = q.value(1);
        row["street"] = q.value(2);
        row["city"] = q.value(3);
        row["postal_code"] = q.value(4);
        row["country"] = q.value(5);
        row["contact_person"] = q.value(6);
        row["phone"] = q.value(7);
        row["email"] = q.value(8);
        result.append(row);
    }
    return result;
}

QMap<QString, QVariant> DbManager::getSupplierById(int supplierId) {
    QMap<QString, QVariant> row;
    QSqlQuery q(db);
    q.prepare("SELECT id, name, street, city, postal_code, country, contact_person, phone, email FROM suppliers WHERE id = ?");
    q.addBindValue(supplierId);
    if (q.exec() && q.next()) {
        row["id"] = q.value(0);
        row["name"] = q.value(1);
        row["street"] = q.value(2);
        row["city"] = q.value(3);
        row["postal_code"] = q.value(4);
        row["country"] = q.value(5);
        row["contact_person"] = q.value(6);
        row["phone"] = q.value(7);
        row["email"] = q.value(8);
    }
    return row;
}

bool DbManager::addSupplier(const QMap<QString, QVariant>& data) {
    QSqlQuery q(db);
    q.prepare("INSERT INTO suppliers (name, street, city, postal_code, country, contact_person, phone, email) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    q.addBindValue(data.value("name"));
    q.addBindValue(data.value("street"));
    q.addBindValue(data.value("city"));
    q.addBindValue(data.value("postal_code"));
    q.addBindValue(data.value("country"));
    q.addBindValue(data.value("contact_person"));
    q.addBindValue(data.value("phone"));
    q.addBindValue(data.value("email"));
    bool ok = q.exec();
    if (!ok) m_lastError = q.lastError();
    return ok;
}

bool DbManager::updateSupplier(int id, const QMap<QString, QVariant>& data) {
    QSqlQuery q(db);
    q.prepare("UPDATE suppliers SET name=?, street=?, city=?, postal_code=?, country=?, contact_person=?, phone=?, email=? WHERE id=?");
    q.addBindValue(data.value("name"));
    q.addBindValue(data.value("street"));
    q.addBindValue(data.value("city"));
    q.addBindValue(data.value("postal_code"));
    q.addBindValue(data.value("country"));
    q.addBindValue(data.value("contact_person"));
    q.addBindValue(data.value("phone"));
    q.addBindValue(data.value("email"));
    q.addBindValue(id);
    bool ok = q.exec();
    if (!ok) m_lastError = q.lastError();
    return ok;
}

bool DbManager::deleteSupplier(int id) {
    QSqlQuery q(db);
    q.prepare("DELETE FROM suppliers WHERE id=?");
    q.addBindValue(id);
    bool ok = q.exec();
    if (!ok) m_lastError = q.lastError();
    return ok;
}

// --- CRUD dla katalogu materiałów (materials_catalog) ---
QVector<QMap<QString, QVariant>> DbManager::getMaterialsCatalog() {
    QVector<QMap<QString, QVariant>> result;
    QSqlQuery q(db);
    q.exec("SELECT id, name, width, length, unit FROM materials_catalog ORDER BY name");
    while (q.next()) {
        QMap<QString, QVariant> row;
        row["id"] = q.value(0);
        row["name"] = q.value(1);
        row["width"] = q.value(2);
        row["length"] = q.value(3);
        row["unit"] = q.value(4);
        result.append(row);
    }
    return result;
}

QMap<QString, QVariant> DbManager::getMaterialById(int materialId) {
    QMap<QString, QVariant> row;
    QSqlQuery q(db);
    q.prepare("SELECT id, name, width, length, unit FROM materials_catalog WHERE id = ?");
    q.addBindValue(materialId);
    if (q.exec() && q.next()) {
        row["id"] = q.value(0);
        row["name"] = q.value(1);
        row["width"] = q.value(2);
        row["length"] = q.value(3);
        row["unit"] = q.value(4);
    }
    return row;
}

bool DbManager::addMaterial(const QMap<QString, QVariant>& data) {
    QSqlQuery q(db);
    q.prepare("INSERT INTO materials_catalog (name, width, length, unit) VALUES (?, ?, ?, ?)");
    q.addBindValue(data.value("name"));
    q.addBindValue(data.value("width"));
    q.addBindValue(data.value("length"));
    q.addBindValue(data.value("unit"));
    bool ok = q.exec();
    if (!ok) m_lastError = q.lastError();
    return ok;
}

bool DbManager::updateMaterial(int id, const QMap<QString, QVariant>& data) {
    QSqlQuery q(db);
    q.prepare("UPDATE materials_catalog SET name=?, width=?, length=?, unit=? WHERE id=?");
    q.addBindValue(data.value("name"));
    q.addBindValue(data.value("width"));
    q.addBindValue(data.value("length"));
    q.addBindValue(data.value("unit"));
    q.addBindValue(id);
    bool ok = q.exec();
    if (!ok) m_lastError = q.lastError();
    return ok;
}

bool DbManager::deleteMaterial(int id) {
    QSqlQuery q(db);
    q.prepare("DELETE FROM materials_catalog WHERE id=?");
    q.addBindValue(id);
    bool ok = q.exec();
    if (!ok) m_lastError = q.lastError();
    return ok;
}

// --- CRUD dla zamówień materiałów (materials_orders) ---
QVector<QMap<QString, QVariant>> DbManager::getMaterialsOrders() {
    QVector<QMap<QString, QVariant>> result;
    QSqlQuery q(db);
    q.exec("SELECT id, order_number, order_date, delivery_date, notes, supplier_id, delivery_company, delivery_street, delivery_postal_code, delivery_city, delivery_country, done FROM materials_orders ORDER BY order_date DESC");
    while (q.next()) {
        QMap<QString, QVariant> row;
        row["id"] = q.value(0);
        row["order_number"] = q.value(1);
        row["order_date"] = q.value(2);
        row["delivery_date"] = q.value(3);
        row["notes"] = q.value(4);
        row["supplier_id"] = q.value(5);
        row["delivery_company"] = q.value(6);
        row["delivery_street"] = q.value(7);
        row["delivery_postal_code"] = q.value(8);
        row["delivery_city"] = q.value(9);
        row["delivery_country"] = q.value(10);
        row["done"] = q.value(11);
        result.append(row);
    }
    return result;
}

bool DbManager::setMaterialsOrderDone(int orderId, bool done) {
    QSqlQuery q(db);
    q.prepare("UPDATE materials_orders SET done=? WHERE id=?");
    q.addBindValue(done ? 1 : 0);
    q.addBindValue(orderId);
    bool ok = q.exec();
    if (!ok) m_lastError = q.lastError();
    return ok;
}

QMap<QString, QVariant> DbManager::getMaterialsOrderById(int orderId) {
    QMap<QString, QVariant> row;
    QSqlQuery q(db);
    q.prepare("SELECT id, order_number, order_date, delivery_date, notes, supplier_id, delivery_company, delivery_street, delivery_postal_code, delivery_city, delivery_country, done FROM materials_orders WHERE id = ?");
    q.addBindValue(orderId);
    if (q.exec() && q.next()) {
        row["id"] = q.value(0);
        row["order_number"] = q.value(1);
        row["order_date"] = q.value(2);
        row["delivery_date"] = q.value(3);
        row["notes"] = q.value(4);
        row["supplier_id"] = q.value(5);
        row["delivery_company"] = q.value(6);
        row["delivery_street"] = q.value(7);
        row["delivery_postal_code"] = q.value(8);
        row["delivery_city"] = q.value(9);
        row["delivery_country"] = q.value(10);
        row["done"] = q.value(11);
    }
    return row;
}

// --- PODPOWIEDZI (QCompleter) dla materiałów ---
QVector<QVariant> DbManager::getUniqueMaterialWidths() {
    QVector<QVariant> result;
    QSqlQuery q(db);
    q.exec("SELECT DISTINCT width FROM materials_catalog WHERE width IS NOT NULL ORDER BY width");
    while (q.next()) {
        result.append(q.value(0));
    }
    return result;
}

QVector<QVariant> DbManager::getUniqueMaterialLengths() {
    QVector<QVariant> result;
    QSqlQuery q(db);
    q.exec("SELECT DISTINCT length FROM materials_catalog WHERE length IS NOT NULL ORDER BY length");
    while (q.next()) {
        result.append(q.value(0));
    }
    return result;
}

QVector<QVariant> DbManager::getUniqueMaterialRolls() {
    QVector<QVariant> result;
    QSqlQuery q(db);
    q.exec("SELECT DISTINCT quantity FROM materials_order_items WHERE quantity IS NOT NULL ORDER BY quantity");
    while (q.next()) {
        result.append(q.value(0));
    }
    return result;
}

// --- Automatyczne ładowanie pozycji zamówienia materiałów ---
QVector<QMap<QString, QVariant>> DbManager::getMaterialsOrderItemsForOrder(int orderId) {
    QVector<QMap<QString, QVariant>> result;
    QSqlQuery q(db);
    q.prepare("SELECT id, order_id, material_id, material_name, width, length, quantity FROM materials_order_items WHERE order_id=?");
    q.addBindValue(orderId);
    if (q.exec()) {
        while (q.next()) {
            QMap<QString, QVariant> row;
            row["id"] = q.value(0);
            row["order_id"] = q.value(1);
            row["material_id"] = q.value(2);
            row["material_name"] = q.value(3);
            row["width"] = q.value(4);
            row["length"] = q.value(5);
            row["quantity"] = q.value(6);
            result.append(row);
        }
    }
    return result;
}

// --- Numeracja zamówień materiałów ---
QString DbManager::getNextMaterialsOrderNumber() {
    // Format: MO-YYYY-NNNN
    int year = QDate::currentDate().year();
    QString prefix = QString("MO-%1-").arg(year);
    QSqlQuery q(db);
    // Szukamy największego numeru z bieżącego roku
    q.prepare("SELECT order_number FROM materials_orders WHERE order_number LIKE ? ORDER BY order_number DESC LIMIT 1");
    q.addBindValue(prefix + "%");
    if (q.exec() && q.next()) {
        QString lastNumber = q.value(0).toString();
        QRegularExpression re(QString("MO-%1-(\\d{4})").arg(year));
        auto match = re.match(lastNumber);
        if (match.hasMatch()) {
            int n = match.captured(1).toInt();
            return prefix + QString::number(n + 1).rightJustified(4, '0');
        }
    }
    // Jeśli nie znaleziono żadnego numeru z tego roku, zaczynamy od 0001
    return prefix + "0001";
}

bool DbManager::deleteMaterialsOrder(int id) {
    QSqlDatabase::database().transaction();
    QSqlQuery q1(db);
    q1.prepare("DELETE FROM materials_order_items WHERE order_id=?");
    q1.addBindValue(id);
    if (!q1.exec()) {
        m_lastError = q1.lastError();
        QSqlDatabase::database().rollback();
        return false;
    }
    QSqlQuery q2(db);
    q2.prepare("DELETE FROM materials_orders WHERE id=?");
    q2.addBindValue(id);
    if (!q2.exec()) {
        m_lastError = q2.lastError();
        QSqlDatabase::database().rollback();
        return false;
    }
    QSqlDatabase::database().commit();
    return true;
}

bool DbManager::updateMaterialsOrder(int id, const QMap<QString, QVariant>& orderData, const QVector<QMap<QString, QVariant>>& items) {
    QSqlDatabase::database().transaction();
    QSqlQuery q(db);
    // Aktualizuj zamówienie w materials_orders
    q.prepare("UPDATE materials_orders SET order_number=?, order_date=?, delivery_date=?, notes=?, supplier_id=?, delivery_company=?, delivery_street=?, delivery_postal_code=?, delivery_city=?, delivery_country=?, done=? WHERE id=?");
    q.addBindValue(orderData.value("order_number"));
    q.addBindValue(orderData.value("order_date"));
    q.addBindValue(orderData.value("delivery_date"));
    q.addBindValue(orderData.value("notes"));
    q.addBindValue(orderData.value("supplier_id"));
    q.addBindValue(orderData.value("delivery_company"));
    q.addBindValue(orderData.value("delivery_street"));
    q.addBindValue(orderData.value("delivery_postal_code"));
    q.addBindValue(orderData.value("delivery_city"));
    q.addBindValue(orderData.value("delivery_country"));
    q.addBindValue(orderData.contains("done") ? orderData.value("done").toInt() : 0);
    q.addBindValue(id);
    if (!q.exec()) {
        m_lastError = q.lastError();
        QSqlDatabase::database().rollback();
        return false;
    }
    // Usuń stare pozycje zamówienia
    QSqlQuery qdel(db);
    qdel.prepare("DELETE FROM materials_order_items WHERE order_id=?");
    qdel.addBindValue(id);
    if (!qdel.exec()) {
        m_lastError = qdel.lastError();
        QSqlDatabase::database().rollback();
        return false;
    }
    // Dodaj nowe pozycje
    QSqlQuery q2(db);
    for (const auto& item : items) {
        q2.prepare("INSERT INTO materials_order_items (order_id, material_id, material_name, width, length, quantity) VALUES (?, ?, ?, ?, ?, ?)");
        q2.addBindValue(id);
        q2.addBindValue(item.value("material_id"));
        q2.addBindValue(item.value("material_name"));
        q2.addBindValue(item.value("width"));
        q2.addBindValue(item.value("length"));
        q2.addBindValue(item.value("quantity"));
        if (!q2.exec()) {
            m_lastError = q2.lastError();
            QSqlDatabase::database().rollback();
            return false;
        }
    }
    QSqlDatabase::database().commit();
    emit orderAdded();
    return true;
}

// --- Wydajne pobieranie zamówień materiałów z nazwą dostawcy i materiałami (JOIN) ---
QVector<QMap<QString, QVariant>> DbManager::getMaterialsOrdersWithDetails() {
    QVector<QMap<QString, QVariant>> result;
    QSqlQuery q(db);
    if (!q.exec(R"(
        SELECT mo.id, mo.order_number, mo.order_date, mo.delivery_date, mo.notes, mo.supplier_id,
               mo.delivery_company, mo.delivery_street, mo.delivery_postal_code, mo.delivery_city, mo.delivery_country, mo.done,
               s.name AS supplier_name
        FROM materials_orders mo
        LEFT JOIN suppliers s ON mo.supplier_id = s.id
        ORDER BY mo.order_date DESC
    )")) {
        qDebug() << "Błąd SQL (zamówienia z JOIN):" << q.lastError().text();
        return result;
    }
    QMap<int, QStringList> orderIdToMaterials;
    QSqlQuery q2(db);
    if (!q2.exec(R"(
        SELECT order_id, material_name, width, length, quantity
        FROM materials_order_items
    )")) {
        qDebug() << "Błąd SQL (pozycje materiałowe):" << q2.lastError().text();
    }
    while (q2.next()) {
        int orderId = q2.value(0).toInt();
        QString mat = q2.value(1).toString();
        QString width = q2.value(2).toString();
        QString length = q2.value(3).toString();
        QString qty = q2.value(4).toString();
        if (!mat.isEmpty())
            orderIdToMaterials[orderId] << QString("%1 %2x%3 %4").arg(mat, width, length, qty);
    }
    while (q.next()) {
        QMap<QString, QVariant> row;
        row["id"] = q.value(0);
        row["order_number"] = q.value(1);
        row["order_date"] = q.value(2);
        row["delivery_date"] = q.value(3);
        row["notes"] = q.value(4);
        row["supplier_id"] = q.value(5);
        row["delivery_company"] = q.value(6);
        row["delivery_street"] = q.value(7);
        row["delivery_postal_code"] = q.value(8);
        row["delivery_city"] = q.value(9);
        row["delivery_country"] = q.value(10);
        row["done"] = q.value(11);
        row["supplier_name"] = q.value(12);
        // Agregacja materiałów jako string
        int orderId = q.value(0).toInt();
        row["materials_summary"] = orderIdToMaterials.value(orderId).join(", ");
        result.append(row);
    }
    return result;
}
