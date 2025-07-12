#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QDateEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMap>
#include <QString>
#include <QDate>
#include "../db/dbmanager.h"
#include "../models/orderitem.h"

// Struktura do grupowania pozycji produkcji
struct ProductionGroup {
    QString material;
    QString width;
    QString height;
    QString dimensions;  // szerokość x wysokość
    QString core;
    double quantity = 0.0;
    int orderCount = 0;
    QStringList orderNumbers;
    QString quantityType;    // typ miary (tyś., rolki)
    double rollLength = 0.0; // długość/nawój rolki
    double totalPrice = 0.0; // suma wartości zamówień
    int rollCount = 0;      // ilość rolek
};

class ProductionSummaryView : public QWidget {
    Q_OBJECT

public:
    explicit ProductionSummaryView(QWidget *parent = nullptr);
    ~ProductionSummaryView();

private slots:
    void generateReport();
    void exportToPdf();
    void exportToExcel();
    void weekChanged(int direction);
    void yearChanged(int direction);
    void onOrderAdded();  // Slot wywoływany po dodaniu nowego zamówienia
    
private:
    void setupUI();
    void setupConnections();
    void refreshData();
    void fillTable(const QList<ProductionGroup> &groups);
    QList<ProductionGroup> getProductionData(const QDate &startDate, const QDate &endDate);
    QString getWeekLabel() const;
    QDate getFirstDayOfWeek(int year, int week) const;
    QDate getLastDayOfWeek(int year, int week) const;
    
    // UI components
    QTableWidget *m_tableWidget;
    QPushButton *m_generateBtn;
    QPushButton *m_exportPdfBtn;
    QPushButton *m_exportExcelBtn;
    QLabel *m_weekLabel;
    QPushButton *m_prevWeekBtn;
    QPushButton *m_nextWeekBtn;
    QPushButton *m_prevYearBtn;
    QPushButton *m_nextYearBtn;
    QComboBox *m_groupByCombo;
    
    // Data
    int m_currentWeek;
    int m_currentYear;
};
