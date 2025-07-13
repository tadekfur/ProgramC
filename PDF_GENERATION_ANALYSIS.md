# Analiza problemu z generowaniem plików PDF

## Zidentyfikowane problemy

### 1. **Główny problem: Brak danych produkcji w plikach PDF**

**Przyczyna:** Niezgodność między schematem bazy danych a kodem aplikacji.

- **Schemat bazy danych** (`order_items` table) zawiera tylko podstawowe pola:
  - `id`, `order_id`, `product_name`, `quantity`, `unit_price`
  
- **Kod aplikacji** próbuje używać szczegółowych pól:
  - `width`, `height`, `material`, `ordered_quantity`, `quantity_type`, `roll_length`, `core`, `price`, `price_type`, `zam_rolki`

### 2. **Metoda `getOrderItems` w DbManager**

**Lokalizacja:** `db/dbmanager.cpp:975`

**Problem:** Metoda używa zapytania SQL które pobiera tylko podstawowe pola:
```sql
SELECT id, order_id, product_name, quantity, unit, price, vat_rate, notes 
FROM order_items WHERE order_id = ?
```

**Oczekiwane pola** przez skrypty Python:
```sql
SELECT id, order_id, width, height, material, ordered_quantity, quantity_type, 
       roll_length, core, price, price_type, zam_rolki, notes 
FROM order_items WHERE order_id = ?
```

### 3. **Skrypty Python oczekują szczegółowych danych**

**Pliki:** `utils/production_ticket.py`, `utils/order_confirmation.py`

**Problem:** Skrypty próbują uzyskać dostęp do pól takich jak:
- `width` / `Szerokość`
- `height` / `Wysokość`
- `material` / `Rodzaj materiału`
- `roll_length` / `nawój/długość`
- `core` / `Średnica rdzenia`
- `ordered_quantity` / `zam. ilość`
- `quantity_type` / `Typ ilości`
- `zam_rolki` / `zam. rolki`
- `price_type` / `CenaTyp`

**Efekt:** Tabela z danymi produkcji jest pusta lub zawiera tylko podstawowe informacje.

## Rozwiązania

### Rozwiązanie 1: Aktualizacja schematu bazy danych

**Lokalizacja:** `db/dbmanager.cpp` w metodzie `ensureDatabase()`

**Dodać kod** aktualizujący schemat `order_items` table:

```cpp
// Dodaj szczegółowe kolumny do tabeli order_items
QStringList requiredColumns = {
    "width", "height", "material", "ordered_quantity", "quantity_type", 
    "roll_length", "core", "price_type", "zam_rolki"
};

QSqlRecord orderItemsRecord = db.record("order_items");
for (const QString& column : requiredColumns) {
    if (!orderItemsRecord.contains(column)) {
        QString alterQuery = QString("ALTER TABLE order_items ADD COLUMN %1 TEXT").arg(column);
        if (!query.exec(alterQuery)) {
            qWarning() << "Nie udało się dodać kolumny" << column << "do tabeli order_items:" << query.lastError().text();
        } else {
            qDebug() << "Dodano kolumnę" << column << "do tabeli order_items";
        }
    }
}
```

### Rozwiązanie 2: Aktualizacja metody `getOrderItems`

**Lokalizacja:** `db/dbmanager.cpp:975`

**Zastąpić zapytanie SQL:**

```cpp
QVector<QMap<QString, QVariant>> DbManager::getOrderItems(int orderId) {
    QVector<QMap<QString, QVariant>> items;
    
    QSqlQuery query(db);
    query.prepare("SELECT id, order_id, product_name, quantity, unit, price, vat_rate, notes, "
                 "width, height, material, ordered_quantity, quantity_type, roll_length, "
                 "core, price_type, zam_rolki "
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
        
        // Dodaj szczegółowe pola dla PDF
        item["width"] = query.value(8);
        item["height"] = query.value(9);
        item["material"] = query.value(10);
        item["ordered_quantity"] = query.value(11);
        item["quantity_type"] = query.value(12);
        item["roll_length"] = query.value(13);
        item["core"] = query.value(14);
        item["price_type"] = query.value(15);
        item["zam_rolki"] = query.value(16);
        
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
```

### Rozwiązanie 3: Aktualizacja mapowania danych w Python

**Lokalizacja:** `utils/production_ticket.py` i `utils/order_confirmation.py`

**Problem:** Mapowanie nazw pól między C++ a Python jest niespójne.

**Rozwiązanie:** Dodać fallback mapowanie w klasach `OrderItemObj`:

```python
class OrderItemObj:
    def __init__(self, data_dict):
        # Mapowanie pól pozycji zamówienia z bazy danych
        self.width = data_dict.get('width', data_dict.get('Szerokość', ''))
        self.height = data_dict.get('height', data_dict.get('Wysokość', ''))
        self.material = data_dict.get('material', data_dict.get('Rodzaj materiału', ''))
        self.ordered_quantity = data_dict.get('ordered_quantity', data_dict.get('zam. ilość', ''))
        self.quantity_type = data_dict.get('quantity_type', data_dict.get('Typ ilości', ''))
        self.roll_length = data_dict.get('roll_length', data_dict.get('nawój/długość', ''))
        self.core = data_dict.get('core', data_dict.get('Średnica rdzenia', ''))
        self.price = data_dict.get('price', data_dict.get('Cena', ''))
        self.price_type = data_dict.get('price_type', data_dict.get('CenaTyp', ''))
        self.zam_rolki = data_dict.get('zam_rolki', data_dict.get('zam. rolki', ''))
        
        # Dodaj inne pola z obiektu orderitem jeśli potrzebne
        for key, value in data_dict.items():
            if not hasattr(self, key):
                setattr(self, key, value)
```

### Rozwiązanie 4: Weryfikacja danych przed generowaniem PDF

**Lokalizacja:** `utils/python_pdf_generator.cpp`

**Dodać debug logging** w metodzie `orderItemsToJson()`:

```cpp
QJsonArray PythonPdfGenerator::orderItemsToJson(const QVector<QMap<QString, QVariant>>& orderItems)
{
    QJsonArray array;
    
    for (const auto& item : orderItems) {
        QJsonObject obj;
        
        // Debug: wyświetl dostępne pola
        qDebug() << "[PythonPdfGenerator] Pola w order item:";
        for (auto it = item.begin(); it != item.end(); ++it) {
            qDebug() << "  " << it.key() << ":" << it.value().toString();
        }
        
        // Mapuj wszystkie pola
        for (auto it = item.begin(); it != item.end(); ++it) {
            obj[it.key()] = QJsonValue::fromVariant(it.value());
        }
        
        array.append(obj);
    }
    
    return array;
}
```

## Podsumowanie

Głównym problemem jest **niezgodność między schematem bazy danych a kodem aplikacji**. Tabela `order_items` zawiera tylko podstawowe pola, ale kod próbuje używać szczegółowych pól produkcyjnych.

## Zaimplementowane poprawki

### ✅ 1. Dodano migrację schematu bazy danych
**Lokalizacja:** `db/dbmanager.cpp` (po linii 1122)
- Dodano kod automatycznie dodający brakujące kolumny do tabeli `order_items`
- Kolumny: `width`, `height`, `material`, `ordered_quantity`, `quantity_type`, `roll_length`, `core`, `price_type`, `zam_rolki`

### ✅ 2. Zaktualizowano metodę `getOrderItems`
**Lokalizacja:** `db/dbmanager.cpp:975`
- Rozszerzono zapytanie SQL o wszystkie szczegółowe pola
- Dodano mapowanie wszystkich pól produkcyjnych

### ✅ 3. Dodano debug logging
**Lokalizacje:** 
- `utils/python_pdf_generator.cpp` - logowanie danych przekazywanych do Python
- `utils/production_ticket.py` - logowanie mapowania danych
- `utils/order_confirmation.py` - logowanie konwersji danych

### ✅ 4. Poprawiono mapowanie danych w Python
**Lokalizacja:** `utils/production_ticket.py`
- Dodano fallback mapowanie nazw pól (np. `width` → `Szerokość`)
- Dodano debug logging do klasy `OrderItemObj`

### ✅ 5. Utworzono skrypt testowy
**Lokalizacja:** `test_pdf_generation.py`
- Skrypt do testowania generowania PDF z przykładowymi danymi
- Weryfikuje czy wszystkie pola są poprawnie przekazywane

## Instrukcje testowania

1. **Uruchom aplikację** - automatycznie zostanie wykonana migracja bazy danych
2. **Stwórz nowe zamówienie** z szczegółowymi danymi (width, height, material, itp.)
3. **Wygeneruj PDF** - sprawdź logi w konsoli, czy dane są poprawnie przekazywane
4. **Sprawdź PDF** - tabela "Dane produkcji" powinna być wypełniona

## Alternatywne testowanie

```bash
# Uruchom test skryptu Python
python test_pdf_generation.py
```

**Oczekiwany efekt:** Po implementacji tych poprawek, tabele z danymi produkcji będą poprawnie wypełnione w plikach PDF dla production ticket i order confirmation.