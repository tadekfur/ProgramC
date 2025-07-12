#include "production_summary_view.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QPdfWriter>
#include <QPainter>
#include <QStandardPaths>
#include <QDateTime>
#include <QCalendar>
#include <QSqlQuery>
#include <QSqlError>
#include <QDesktopServices>
#include <QUrl>
#include <QPdfDocument>
#include <QPdfView>

ProductionSummaryView::ProductionSummaryView(QWidget *parent) : QWidget(parent), 
    m_tableWidget(nullptr),
    m_generateBtn(nullptr),
    m_exportPdfBtn(nullptr),
    m_exportExcelBtn(nullptr),
    m_weekLabel(nullptr),
    m_prevWeekBtn(nullptr),
    m_nextWeekBtn(nullptr),
    m_prevYearBtn(nullptr),
    m_nextYearBtn(nullptr),
    m_groupByCombo(nullptr)
{
    // Ustawienie bieżącego tygodnia i roku
    QDate currentDate = QDate::currentDate();
    m_currentYear = currentDate.year();
    m_currentWeek = currentDate.weekNumber();
    
    setupUI();
    setupConnections();
    refreshData();
}

ProductionSummaryView::~ProductionSummaryView() {
    // Qt automatycznie usuwa dzieci (obiekty UI) gdy rodzic jest usuwany
}

void ProductionSummaryView::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // Nagłówek i kontrolki nawigacji
    QHBoxLayout *headerLayout = new QHBoxLayout();
    
    QLabel *titleLabel = new QLabel("Zestawienie produkcji");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    
    // Kontrolki wyboru tygodnia i roku
    QHBoxLayout *dateControlLayout = new QHBoxLayout();
    dateControlLayout->setSpacing(5);
    
    m_prevYearBtn = new QPushButton("<<");
    m_prevYearBtn->setToolTip("Poprzedni rok");
    m_prevYearBtn->setMaximumWidth(40);
    
    m_prevWeekBtn = new QPushButton("<");
    m_prevWeekBtn->setToolTip("Poprzedni tydzień");
    m_prevWeekBtn->setMaximumWidth(40);
    
    m_weekLabel = new QLabel(getWeekLabel());
    m_weekLabel->setAlignment(Qt::AlignCenter);
    m_weekLabel->setMinimumWidth(200);
    
    m_nextWeekBtn = new QPushButton(">");
    m_nextWeekBtn->setToolTip("Następny tydzień");
    m_nextWeekBtn->setMaximumWidth(40);
    
    m_nextYearBtn = new QPushButton(">>");
    m_nextYearBtn->setToolTip("Następny rok");
    m_nextYearBtn->setMaximumWidth(40);
    
    dateControlLayout->addWidget(m_prevYearBtn);
    dateControlLayout->addWidget(m_prevWeekBtn);
    dateControlLayout->addWidget(m_weekLabel);
    dateControlLayout->addWidget(m_nextWeekBtn);
    dateControlLayout->addWidget(m_nextYearBtn);
    
    // Przyciski akcji
    m_generateBtn = new QPushButton("Generuj zestawienie");
    m_exportPdfBtn = new QPushButton("Eksport do PDF");
    m_exportExcelBtn = new QPushButton("Eksport do Excel");
    
    // Combo do grupowania
    QHBoxLayout *groupByLayout = new QHBoxLayout();
    QLabel *groupByLabel = new QLabel("Grupuj według:");
    m_groupByCombo = new QComboBox();
    m_groupByCombo->addItem("Materiał -> Wymiar -> Rdzeń");
    m_groupByCombo->addItem("Wymiar -> Materiał -> Rdzeń");
    m_groupByCombo->addItem("Rdzeń -> Materiał -> Wymiar");
    
    groupByLayout->addWidget(groupByLabel);
    groupByLayout->addWidget(m_groupByCombo);
    groupByLayout->addStretch();
    groupByLayout->addWidget(m_generateBtn);
    groupByLayout->addWidget(m_exportPdfBtn);
    groupByLayout->addWidget(m_exportExcelBtn);
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addLayout(dateControlLayout);
    
    // Tabela wyników
    m_tableWidget = new QTableWidget(0, 7, this);
    m_tableWidget->setHorizontalHeaderLabels({
        "Materiał", "Wymiar", "Średnica rdzenia", "Ilość łącznie", "Ilość rolek", "Liczba zamówień", "Wartość [PLN]"
    });
    
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->setEditTriggers(QTableWidget::NoEditTriggers);
    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->setSelectionBehavior(QTableWidget::SelectRows);
    
    // Dodanie wszystkiego do głównego layoutu
    mainLayout->addLayout(headerLayout);
    mainLayout->addLayout(groupByLayout);
    mainLayout->addWidget(m_tableWidget, 1);
    
    setLayout(mainLayout);
}

void ProductionSummaryView::setupConnections() {
    connect(m_generateBtn, &QPushButton::clicked, this, &ProductionSummaryView::generateReport);
    connect(m_exportPdfBtn, &QPushButton::clicked, this, &ProductionSummaryView::exportToPdf);
    connect(m_exportExcelBtn, &QPushButton::clicked, this, &ProductionSummaryView::exportToExcel);
    connect(m_prevWeekBtn, &QPushButton::clicked, this, [this](){ weekChanged(-1); });
    connect(m_nextWeekBtn, &QPushButton::clicked, this, [this](){ weekChanged(1); });
    connect(m_prevYearBtn, &QPushButton::clicked, this, [this](){ yearChanged(-1); });
    connect(m_nextYearBtn, &QPushButton::clicked, this, [this](){ yearChanged(1); });
    connect(m_groupByCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ProductionSummaryView::refreshData);
    
    // Połączenie z sygnałem orderAdded z DbManager
    auto& dbManager = DbManager::instance();
    connect(&dbManager, &DbManager::orderAdded, this, &ProductionSummaryView::onOrderAdded);
}

QString ProductionSummaryView::getWeekLabel() const {
    QDate startDate = getFirstDayOfWeek(m_currentYear, m_currentWeek);
    QDate endDate = getLastDayOfWeek(m_currentYear, m_currentWeek);
    
    return QString("Tydzień %1 (%2) - %3 do %4")
        .arg(m_currentWeek)
        .arg(m_currentYear)
        .arg(startDate.toString("dd.MM.yyyy"))
        .arg(endDate.toString("dd.MM.yyyy"));
}

QDate ProductionSummaryView::getFirstDayOfWeek(int year, int week) const {
    QDate date(year, 1, 1);
    date = date.addDays((week - 1) * 7);
    // Znajdź poniedziałek
    while (date.dayOfWeek() != 1) {
        date = date.addDays(-1);
    }
    return date;
}

QDate ProductionSummaryView::getLastDayOfWeek(int year, int week) const {
    QDate date = getFirstDayOfWeek(year, week);
    return date.addDays(6); // Niedziela (6 dni od poniedziałku)
}

void ProductionSummaryView::weekChanged(int direction) {
    m_currentWeek += direction;
    
    // Obsługa przejścia między latami
    if (m_currentWeek < 1) {
        m_currentYear--;
        m_currentWeek = QDate(m_currentYear, 12, 28).weekNumber(); // Ostatni tydzień poprzedniego roku
    } else if (m_currentWeek > QDate(m_currentYear, 12, 28).weekNumber()) {
        m_currentYear++;
        m_currentWeek = 1;
    }
    
    m_weekLabel->setText(getWeekLabel());
    refreshData();
}

void ProductionSummaryView::yearChanged(int direction) {
    m_currentYear += direction;
    
    // Sprawdzenie czy bieżący tydzień jest poprawny dla nowego roku
    int maxWeek = QDate(m_currentYear, 12, 28).weekNumber();
    if (m_currentWeek > maxWeek) {
        m_currentWeek = maxWeek;
    }
    
    m_weekLabel->setText(getWeekLabel());
    refreshData();
}

void ProductionSummaryView::generateReport() {
    refreshData();
}

void ProductionSummaryView::refreshData() {
    QDate startDate = getFirstDayOfWeek(m_currentYear, m_currentWeek);
    QDate endDate = getLastDayOfWeek(m_currentYear, m_currentWeek);
    
    QList<ProductionGroup> groups = getProductionData(startDate, endDate);
    fillTable(groups);
}

QList<ProductionGroup> ProductionSummaryView::getProductionData(const QDate &startDate, const QDate &endDate) {
    QList<ProductionGroup> result;
    QMap<QString, ProductionGroup> groupsMap;
    
    qDebug() << "=== Pobieranie danych produkcji ===";
    qDebug() << "Zakres dat:" << startDate.toString("yyyy-MM-dd") << "do" << endDate.toString("yyyy-MM-dd");
    
    // Tworzymy zapytanie do bazy danych
    DbManager& dbManager = DbManager::instance();
    QSqlDatabase db = dbManager.getPooledConnection();
    qDebug() << "Typ bazy danych:" << db.driverName();
    qDebug() << "Nazwa bazy:" << db.databaseName();
    
    // Najpierw sprawdźmy wszystkie zamówienia w bazie
    QSqlQuery debugQuery(db);
    if (debugQuery.exec("SELECT o.id, o.order_number, o.status, o.created_at FROM orders o ORDER BY o.id")) {
        qDebug() << "=== Wszystkie zamówienia w bazie ===";
        while (debugQuery.next()) {
            int id = debugQuery.value(0).toInt();
            QString orderNumber = debugQuery.value(1).toString();
            int status = debugQuery.value(2).toInt();
            QString createdAt = debugQuery.value(3).toString();
            qDebug() << "ID:" << id << "Numer:" << orderNumber << "Status:" << status << "Data:" << createdAt;
        }
        qDebug() << "=== Koniec listy wszystkich zamówień ===";
    }
    
    // Sprawdźmy też wszystkie pozycje zamówień
    QSqlQuery itemsQuery(db);
    if (itemsQuery.exec("SELECT oi.id, oi.order_id, o.order_number, oi.material, oi.width, oi.height, oi.core, oi.ordered_quantity FROM order_items oi JOIN orders o ON oi.order_id = o.id ORDER BY oi.order_id, oi.id")) {
        qDebug() << "=== Wszystkie pozycje zamówień w bazie ===";
        while (itemsQuery.next()) {
            int itemId = itemsQuery.value(0).toInt();
            int orderId = itemsQuery.value(1).toInt();
            QString orderNumber = itemsQuery.value(2).toString();
            QString material = itemsQuery.value(3).toString();
            QString width = itemsQuery.value(4).toString();
            QString height = itemsQuery.value(5).toString();
            QString core = itemsQuery.value(6).toString();
            QString quantity = itemsQuery.value(7).toString();
            
            qDebug() << "Pozycja ID:" << itemId << "Zamówienie:" << orderNumber 
                    << "Material:[" << material << "] Width:[" << width << "] Height:[" << height 
                    << "] Core:[" << core << "] Qty:" << quantity;
        }
        qDebug() << "=== Koniec listy wszystkich pozycji ===";
    }
    
    QSqlQuery q(db);
    q.prepare("SELECT o.order_number, oi.material, oi.width, oi.height, oi.core, oi.ordered_quantity, oi.quantity_type, oi.roll_length, "
              "oi.price, oi.price_type, o.status "
              "FROM order_items oi "
              "JOIN orders o ON oi.order_id = o.id "
              "WHERE o.status IN (0, 1, 2)"); // Tylko zamówienia w procesie produkcji: Przyjęte (0), W produkcji (1), Gotowe (2)
    
    // Usunięto filtr dat - pokazujemy wszystkie aktywne zamówienia
    
    if (q.exec()) {
        int rowCount = 0;
        while (q.next()) {
            rowCount++;
            QString orderNumber = q.value(0).toString();
            QString material = q.value(1).toString();
            QString width = q.value(2).toString();
            QString height = q.value(3).toString();
            QString core = q.value(4).toString();
            QString orderedQuantity = q.value(5).toString();
            QString quantityType = q.value(6).toString();
            QString rollLength = q.value(7).toString();
            QString price = q.value(8).toString();
            QString priceType = q.value(9).toString();
            int status = q.value(10).toInt();
            
            qDebug() << "Przetwarzanie zamówienia:" << orderNumber << "Status:" << status;
            
            // Pomijamy rekordy bez materiału lub szerokości
            if (material.trimmed().isEmpty() || width.trimmed().isEmpty()) {
                qDebug() << "Pomijam zamówienie" << orderNumber << "- brak materiału lub szerokości";
                qDebug() << "Material:" << material << "(puste:" << material.trimmed().isEmpty() << ")";
                qDebug() << "Width:" << width << "(puste:" << width.trimmed().isEmpty() << ")";
                qDebug() << "Height:" << height;
                qDebug() << "Core:" << core;
                continue;
            }
            
            // Tworzymy klucz grupowania na podstawie wymiarów (szerokość x wysokość)
            QString groupKey = QString("%1|%2x%3|%4").arg(material).arg(width).arg(height).arg(core);
            
            // Inicjalizacja grupy jeśli nie istnieje
            if (!groupsMap.contains(groupKey)) {
                ProductionGroup group;
                group.material = material;
                group.width = width;
                group.height = height;
                group.dimensions = QString("%1 x %2").arg(width).arg(height);
                group.core = core;
                group.quantity = 0.0;
                group.orderCount = 0;
                group.totalPrice = 0.0;
                groupsMap[groupKey] = group;
            }
            
            bool ok;
            double quantity = orderedQuantity.toDouble(&ok);
            if (!ok || quantity <= 0) continue;
            
            double actualQuantity = 0.0; // ilość w tysiącach
            
            if (quantityType.toLower().contains("tys")) {
                // Już w tysiącach - nie trzeba przeliczać
                actualQuantity = quantity;
            } else if (quantityType.toLower().contains("rolki") || 
                     quantityType.toLower().contains("rol")) {
                // Dla rolek: ilość rolek × nawój/długość / 1000
                double rollLengthValue = rollLength.toDouble(&ok);
                if (ok && rollLengthValue > 0) {
                    actualQuantity = (quantity * rollLengthValue / 1000.0);
                    // Dodaj ilość rolek do odpowiedniej grupy średnic rdzenia
                    groupsMap[groupKey].rollCount += static_cast<int>(quantity);
                }
            } else {
                // Inne jednostki - zakładamy że trzeba podzielić przez 1000
                actualQuantity = (quantity / 1000.0);
            }
            
            // Dodaj ilość do grupy
            if (actualQuantity > 0) {
                groupsMap[groupKey].quantity += actualQuantity;
            }

            // Obliczanie wartości zamówienia
            QString priceStr = price;
            priceStr.replace(",", ".");
            double priceValue = priceStr.toDouble(&ok);
            
            if (!ok || priceValue <= 0) {
                qWarning() << "Nieprawidłowa cena:" << price << "dla zamówienia" << orderNumber;
                continue;
            }

            QString measureType = quantityType.toLower();
            QString pricingType = priceType.toLower();
            double orderValue = 0.0;

            // Pobierz długość nawoju/rolki jeśli jest dostępna
            QString rollLengthStr = rollLength;
            rollLengthStr.replace(",", ".");
            double rollLengthValue = rollLengthStr.toDouble(&ok);
            if (!ok) {
                qWarning() << "Błąd konwersji długości nawoju:" << rollLength << "dla zamówienia" << orderNumber;
                rollLengthValue = 0.0;
            }

            qDebug() << "\n=== Obliczanie wartości dla zamówienia" << orderNumber << "===";
            qDebug() << "Material:" << material;
            qDebug() << "Ilość:" << quantity << measureType;
            qDebug() << "Cena:" << priceValue << pricingType;
            qDebug() << "Długość nawoju:" << rollLengthValue;

            if (pricingType.contains("rolk")) {
                // Cena za rolkę - zawsze mnożymy przez ilość rolek
                orderValue = quantity * priceValue;
                qDebug() << "Cena za rolkę:" << quantity << "rolek *" << priceValue << "PLN =" << orderValue << "PLN";
            }
            else if (pricingType.contains("tys") || pricingType.contains("tyś") || 
                    pricingType.contains("1000") || pricingType.contains("1 tys")) {
                // Cena za tysiąc (różne warianty zapisu)
                if (measureType.contains("tys") || measureType.contains("tyś")) {
                    // Jeśli ilość w tysiącach, mnożymy bezpośrednio
                    orderValue = quantity * priceValue;
                    qDebug() << "Cena za tysiąc (ilość w tys):" << quantity << "tys. *" << priceValue << "PLN =" << orderValue << "PLN";
                }
                else if (measureType.contains("rolk") || measureType.contains("rol")) {
                    if (rollLengthValue > 0) {
                        // Jeśli ilość w rolkach, przeliczamy na tysiące:
                        // ilość rolek × długość nawoju / 1000 × cena za tysiąc
                        orderValue = (quantity * rollLengthValue / 1000.0) * priceValue;
                        qDebug() << "Cena za tysiąc (ilość w rolkach):" << quantity << "rolek *" 
                               << rollLengthValue << "mb /" << "1000 *" << priceValue << "PLN =" << orderValue << "PLN";
                    } else {
                        qWarning() << "Brak długości nawoju dla zamówienia w rolkach:" << orderNumber;
                    }
                }
                else {
                    // Zakładamy że to sztuki - przeliczamy na tysiące
                    orderValue = (quantity / 1000.0) * priceValue;
                    qDebug() << "Cena za tysiąc (ilość w szt):" << quantity << "szt. /" 
                           << "1000 *" << priceValue << "PLN =" << orderValue << "PLN";
                }
            }
            else if (pricingType.contains("szt")) {
                // Cena za sztukę
                orderValue = quantity * priceValue;
                qDebug() << "Cena za sztukę:" << quantity << "szt. *" << priceValue << "PLN =" << orderValue << "PLN";
            }
            else {
                qWarning() << "Nieznany typ ceny:" << pricingType << "dla zamówienia" << orderNumber;
                continue;
            }

            // Aktualizuj całkowitą wartość dla grupy tylko jeśli coś zostało policzone
            if (orderValue > 0) {
                // Dodaj wartość zamówienia do sumy dla grupy
                groupsMap[groupKey].totalPrice += orderValue;
                qDebug() << "Dodano wartość" << orderValue << "PLN do grupy" << groupKey 
                        << "(suma:" << groupsMap[groupKey].totalPrice << "PLN)";
            } else {
                qWarning() << "Wartość 0 PLN dla zamówienia" << orderNumber 
                          << "(material:" << material << "typ ceny:" << pricingType 
                          << "typ miary:" << measureType << ")";
            }
            
            // Aktualizacja licznika zamówień
            if (!groupsMap[groupKey].orderNumbers.contains(orderNumber)) {
                groupsMap[groupKey].orderCount++;
                groupsMap[groupKey].orderNumbers.append(orderNumber);
            }
        }
        qDebug() << "Pobrano" << rowCount << "pozycji zamówień z bazy danych";
    } else {
        qWarning() << "Błąd podczas pobierania danych produkcji:" << q.lastError().text();
    }
    
    dbManager.returnPooledConnection(db);
    
    qDebug() << "Znaleziono" << groupsMap.size() << "grup produktów";
    qDebug() << "=== Koniec pobierania danych produkcji ===";
    
    // Konwersja mapy na listę
    QMapIterator<QString, ProductionGroup> it(groupsMap);
    while (it.hasNext()) {
        it.next();
        result.append(it.value());
    }
    
    return result;
}

void ProductionSummaryView::fillTable(const QList<ProductionGroup> &groups) {
    m_tableWidget->clearContents();
    m_tableWidget->setRowCount(0);
    
    // Grupowanie po wymiarach
    QMap<QString, ProductionGroup> dimensionGroups;
    
    // Najpierw zgrupuj po wymiarach
    for (const auto &group : groups) {
        QString dimensionKey = group.width;
        if (!group.height.isEmpty()) {
            dimensionKey = QString("%1 x %2").arg(group.width).arg(group.height);
        }
        
        QString fullKey = QString("%1|%2|%3").arg(group.material, dimensionKey, group.core);
        
        if (!dimensionGroups.contains(fullKey)) {
            dimensionGroups[fullKey] = group;
        } else {
            // Sumuj ilości dla tych samych wymiarów
            dimensionGroups[fullKey].quantity += group.quantity;
            // Dodaj unikalne numery zamówień
            for (const QString &orderNum : group.orderNumbers) {
                if (!dimensionGroups[fullKey].orderNumbers.contains(orderNum)) {
                    dimensionGroups[fullKey].orderCount++;
                    dimensionGroups[fullKey].orderNumbers.append(orderNum);
                }
            }
        }
    }
    
    // Konwersja mapy na listę do sortowania
    QList<ProductionGroup> sortedGroups;
    QMapIterator<QString, ProductionGroup> it(dimensionGroups);
    while (it.hasNext()) {
        it.next();
        sortedGroups.append(it.value());
    }
    
    // Sortowanie według wybranej opcji
    int groupingOption = m_groupByCombo->currentIndex();
    std::sort(sortedGroups.begin(), sortedGroups.end(), [](const ProductionGroup &a, const ProductionGroup &b) {
        // Zawsze sortuj najpierw po materiale
        if (a.material != b.material) return a.material < b.material;
        
        // Potem po wymiarach
        QString dimA = a.width + (a.height.isEmpty() ? "" : " x " + a.height);
        QString dimB = b.width + (b.height.isEmpty() ? "" : " x " + b.height);
        if (dimA != dimB) return dimA < dimB;
        
        // Na końcu po rdzeniu
        return a.core < b.core;
    });
    
    // Wypełnienie tabeli danymi
    double totalQuantity = 0.0;
    double totalValue = 0.0;
    int totalOrders = 0;
    QStringList allOrderNumbers;

    for (const auto &group : sortedGroups) {
        int row = m_tableWidget->rowCount();
        m_tableWidget->insertRow(row);
        
        // Materiał
        QTableWidgetItem *materialItem = new QTableWidgetItem(group.material);
        materialItem->setTextAlignment(Qt::AlignCenter);
        m_tableWidget->setItem(row, 0, materialItem);
        
        // Wymiar (szerokość x wysokość)
        QString dimensionText = group.width;
        if (!group.height.isEmpty()) {
            dimensionText = QString("%1 x %2").arg(group.width).arg(group.height);
        }
        QTableWidgetItem *dimensionItem = new QTableWidgetItem(dimensionText);
        dimensionItem->setTextAlignment(Qt::AlignCenter);
        m_tableWidget->setItem(row, 1, dimensionItem);
        
        // Rdzeń
        QTableWidgetItem *coreItem = new QTableWidgetItem(group.core);
        coreItem->setTextAlignment(Qt::AlignCenter);
        m_tableWidget->setItem(row, 2, coreItem);
        
        // Ilość (zawsze w tysiącach)
        QTableWidgetItem *quantityItem = new QTableWidgetItem(
            QString::number(group.quantity, 'f', 3) + " tys.");
        quantityItem->setTextAlignment(Qt::AlignCenter);
        m_tableWidget->setItem(row, 3, quantityItem);

        // Ilość rolek
        QTableWidgetItem *rollCountItem = new QTableWidgetItem(
            group.rollCount > 0 ? QString("%1").arg(group.rollCount) : "");
        rollCountItem->setTextAlignment(Qt::AlignCenter);
        m_tableWidget->setItem(row, 4, rollCountItem);
        
        // Liczba zamówień
        QTableWidgetItem *orderCountItem = new QTableWidgetItem(QString::number(group.orderCount));
        orderCountItem->setTextAlignment(Qt::AlignCenter);
        m_tableWidget->setItem(row, 5, orderCountItem);
        
        // Wartość zamówień
        QString formattedPrice = QString("%L1").arg(group.totalPrice, 0, 'f', 2);
        QTableWidgetItem *priceItem = new QTableWidgetItem(formattedPrice);
        priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_tableWidget->setItem(row, 6, priceItem);
        
        totalValue += group.totalPrice;
        
        // Aktualizacja sum
        totalQuantity += group.quantity;
        for (const QString &orderNum : group.orderNumbers) {
            if (!allOrderNumbers.contains(orderNum)) {
                allOrderNumbers.append(orderNum);
                totalOrders++;
            }
        }
        
        // Dodanie tooltipa z numerami zamówień
        QString ordersList = group.orderNumbers.join(", ");
        for (int col = 0; col < m_tableWidget->columnCount(); ++col) {
            if (m_tableWidget->item(row, col)) {
                m_tableWidget->item(row, col)->setToolTip("Zamówienia: " + ordersList);
            }
        }
    }
    
    // Dodanie wiersza sumy
    int totalRow = m_tableWidget->rowCount();
    m_tableWidget->insertRow(totalRow);
    
    // RAZEM
    QTableWidgetItem *totalLabelItem = new QTableWidgetItem("RAZEM");
    totalLabelItem->setFont(QFont("", -1, QFont::Bold));
    totalLabelItem->setTextAlignment(Qt::AlignCenter);
    m_tableWidget->setItem(totalRow, 0, totalLabelItem);
    
    // Puste komórki dla wymiarów i rdzenia
    for (int col = 1; col < 3; ++col) {
        QTableWidgetItem *emptyItem = new QTableWidgetItem("");
        emptyItem->setFont(QFont("", -1, QFont::Bold));
        emptyItem->setTextAlignment(Qt::AlignCenter);
        m_tableWidget->setItem(totalRow, col, emptyItem);
    }
    
    // Suma ilości łącznie (tys.)
    QTableWidgetItem *totalQuantityItem = new QTableWidgetItem(
        QString::number(totalQuantity, 'f', 3) + " tys.");
    totalQuantityItem->setFont(QFont("", -1, QFont::Bold));
    totalQuantityItem->setTextAlignment(Qt::AlignCenter);
    m_tableWidget->setItem(totalRow, 3, totalQuantityItem);
    
    // Pusta komórka dla "Ilość rolek" (kolumna 4)
    QTableWidgetItem *emptyRollsItem = new QTableWidgetItem("");
    emptyRollsItem->setFont(QFont("", -1, QFont::Bold));
    emptyRollsItem->setTextAlignment(Qt::AlignCenter);
    m_tableWidget->setItem(totalRow, 4, emptyRollsItem);
    
    // Suma zamówień (kolumna 5)
    QTableWidgetItem *totalOrdersItem = new QTableWidgetItem(QString::number(totalOrders));
    totalOrdersItem->setFont(QFont("", -1, QFont::Bold));
    totalOrdersItem->setTextAlignment(Qt::AlignCenter);
    m_tableWidget->setItem(totalRow, 5, totalOrdersItem);
    
    // Suma wartości (kolumna 6)
    QString formattedTotalValue = QString("%L1").arg(totalValue, 0, 'f', 2);
    QTableWidgetItem *totalValueItem = new QTableWidgetItem(formattedTotalValue);
    totalValueItem->setFont(QFont("", -1, QFont::Bold));
    totalValueItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    totalValueItem->setBackground(QBrush(QColor(240, 240, 240)));
    m_tableWidget->setItem(totalRow, 6, totalValueItem);
}

void ProductionSummaryView::exportToPdf() {
    QString fileName = QFileDialog::getOpenFileName(
        this, "Wybierz plik PDF do podglądu", 
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Pliki PDF (*.pdf)");
    if (fileName.isEmpty()) return;

    // Tworzymy okno podglądu PDF
    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle("Podgląd PDF: " + QFileInfo(fileName).fileName());
    dlg->resize(900, 1200);
    QVBoxLayout *layout = new QVBoxLayout(dlg);
    QPdfView *pdfView = new QPdfView(dlg);
    QPdfDocument *pdfDoc = new QPdfDocument(dlg);
    pdfDoc->load(fileName);
    pdfView->setDocument(pdfDoc);
    pdfView->setPageMode(QPdfView::PageMode::SinglePage);
    layout->addWidget(pdfView);
    dlg->setLayout(layout);
    dlg->exec();
}

void ProductionSummaryView::exportToExcel() {
    QString fileName = QFileDialog::getSaveFileName(
        this, "Eksport do CSV", 
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + 
        "/zestawienie_produkcji_" + QString::number(m_currentYear) + "_tydz" + QString::number(m_currentWeek) + ".csv",
        "Pliki CSV (*.csv)");
        
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Błąd", "Nie można otworzyć pliku do zapisu");
        return;
    }
    
    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#else
    out.setEncoding(QStringConverter::Utf8);
#endif
    
    // Nagłówek CSV
    out << "Materiał;Wymiar;Średnica rdzenia;Ilość łącznie;Liczba zamówień;Wartość [PLN]\n";
    
    // Dane
    for (int row = 0; row < m_tableWidget->rowCount(); ++row) {
        for (int col = 0; col < m_tableWidget->columnCount(); ++col) {
            QTableWidgetItem *item = m_tableWidget->item(row, col);
            if (!item) continue;
            
            QString cellText = item->text();
            
            // Znak średnika w tekście należy zastąpić znakiem podziału pola
            cellText.replace(";", ",");
            
            out << cellText;
            
            if (col < m_tableWidget->columnCount() - 1) {
                out << ";";
            }
        }
        out << "\n";
    }
    
    file.close();
    
    // Otwórz wygenerowany plik CSV
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
}

void ProductionSummaryView::onOrderAdded()
{
    qDebug() << "=== ProductionSummaryView::onOrderAdded() wywołane ===";
    qDebug() << "Bieżący tydzień:" << m_currentWeek << "rok:" << m_currentYear;
    
    // Po dodaniu nowego zamówienia odświeżamy raport
    generateReport();
    qDebug() << "=== ProductionSummaryView odświeżone ===";
}
