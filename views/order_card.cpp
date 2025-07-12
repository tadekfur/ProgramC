#include "order_card.h"
#include "db/dbmanager.h"
#include "views/production_table_view.h"
#include <QFont>
#include <QDialog>
#include <QVBoxLayout>
#include <QApplication>
#include <QStyle>
#include <QDebug>
#include <QMetaObject>
#include <QSqlQuery>
#include <QSqlError>
#include <QTableWidget>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QTimer>

static std::pair<QColor, QColor> getGradientForShippingDays(int workdaysLeft) {
    if (workdaysLeft < 0)
        return {QColor("#ff6666"), QColor("#ffffff")};
    if (workdaysLeft == 0)
        return {QColor("#ffe600"), QColor("#ffffff")};
    if (workdaysLeft == 1)
        return {QColor("#ffc966"), QColor("#ffffff")};
    if (workdaysLeft == 2)
        return {QColor("#b2d7ff"), QColor("#ffffff")};
    if (workdaysLeft == 3)
        return {QColor("#b5e7b2"), QColor("#ffffff")};
    if (workdaysLeft == 4)
        return {QColor("#cccccc"), QColor("#ffffff")};
    return {QColor("#ffffff"), QColor("#ffffff")};
}

OrderCard::OrderCard(const Order& order, QWidget* dashboard, QWidget* parent)
    : QFrame(parent), order(order), dashboard(dashboard), detailsDialog(nullptr)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    int workdaysLeft = countWorkdays(QDate::currentDate(), order.deliveryDate);
    auto grad = getGradientForShippingDays(workdaysLeft);
    setupUi(grad.first, grad.second);
    setMouseTracking(true);
}

void OrderCard::setupUi(const QColor& gradStart, const QColor& gradEnd) {
    setFrameShape(QFrame::Box);
    setLineWidth(1);
    setStyleSheet(QString(
        "QFrame {"
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2);"
        "border: 1.5px solid #bcd;"
        "border-radius: 8px;"
        "margin: 0px;" // zmniejszono margines
        "padding: 4px 4px;" // zmniejszono padding
        "}"
        "QFrame:hover { border: 2px solid #2196F3; }"
    ).arg(gradStart.name(), gradEnd.name()));

    // Ustaw styl toolTip na jasny dla całej karty
    setStyleSheet(styleSheet() +
        "\nQToolTip { background-color: #f9f9f9; color: #222; border: 1px solid #197a3d; font-size: 11px; }");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4); // zmniejszono marginesy
    mainLayout->setSpacing(2); // zmniejszono odstępy

    // Skrócona nazwa klienta
    QString shortName = !order.client.shortName.isEmpty()
        ? order.client.shortName
        : (order.client.name.left(20) + (order.client.name.length() > 20 ? "…" : ""));
    clientLabel = new QLabel(shortName);
    QFont font; font.setPointSize(11); font.setBold(true); // zmniejszono font
    clientLabel->setFont(font);
    clientLabel->setStyleSheet("color: #1976D2; font-weight: 700; padding-bottom: 1px;");
    mainLayout->addWidget(clientLabel);

    // Data dostawy
    dateLabel = new QLabel(order.deliveryDate.toString("dd.MM.yyyy"));
    QFont dateFont; dateFont.setPointSize(11); dateFont.setBold(true); // zmniejszono font
    dateLabel->setFont(dateFont);
    int daysLeft = QDate::currentDate().daysTo(order.deliveryDate);
    setDateLabelColor(daysLeft);
    dateLabel->setAlignment(Qt::AlignLeft);
    mainLayout->addWidget(dateLabel);

    // Dolny wiersz
    QHBoxLayout* bottomRow = new QHBoxLayout;
    bottomRow->setSpacing(4); // zmniejszono odstępy

    // --- Przyciski statusu zamówienia ---
    statusBtn1 = new QPushButton;
    statusBtn2 = new QPushButton;
    statusBtn3 = new QPushButton;
    statusBtn1->setFixedSize(28, 28);
    statusBtn2->setFixedSize(28, 28);
    statusBtn3->setFixedSize(28, 28);
    auto setStatusBtnStyle = [](QPushButton* btn, bool active) {
        if (active)
            btn->setStyleSheet("border:2px solid #197a3d; border-radius:14px; background:#1ecb5a;");
        else
            btn->setStyleSheet("border:2px solid #197a3d; border-radius:14px; background:transparent;");
    };
    setStatusBtnStyle(statusBtn1, order.status == Order::Przyjete);
    setStatusBtnStyle(statusBtn2, order.status == Order::Produkcja);
    setStatusBtnStyle(statusBtn3, order.status == Order::Gotowe);
    statusBtn1->setToolTip("Przyjęte do realizacji\nKliknij, aby ustawić ten etap");
    statusBtn2->setToolTip("Produkcja\nKliknij, aby ustawić ten etap");
    statusBtn3->setToolTip("Gotowe do wysyłki\nKliknij, aby ustawić ten etap");
    statusBtn1->setText("1");
    statusBtn2->setText("2");
    statusBtn3->setText("3");
    QFont stFont; stFont.setPointSize(10); stFont.setBold(true);
    statusBtn1->setFont(stFont);
    statusBtn2->setFont(stFont);
    statusBtn3->setFont(stFont);
    bottomRow->addWidget(statusBtn1);
    bottomRow->addWidget(statusBtn2);
    bottomRow->addWidget(statusBtn3);
    // --- Przycisk 'Wykonane' pod kółeczkami, wyrównany do lewej ---
    QHBoxLayout* doneRow = new QHBoxLayout;
    doneBtn = new QPushButton(QString::fromUtf8("\u2713 Wykonane"));
    QFont btnFont; btnFont.setPointSize(9); btnFont.setWeight(QFont::Medium);
    doneBtn->setFont(btnFont);
    doneBtn->setStyleSheet(
        "QPushButton { background: #4CAF50; color: white; border: none; border-radius: 5px; padding: 2px 8px; font-weight: 600; min-width: 70px; min-height: 20px; }"
        "QPushButton:hover { background: #45a049; }"
        "QPushButton:pressed { background: #3d8b40; }"
    );
    connect(doneBtn, &QPushButton::clicked, this, &OrderCard::markAsDone);
    doneRow->addWidget(doneBtn, 0, Qt::AlignLeft);
    doneRow->addStretch(1);
    // ---
    bottomRow->addStretch(1);
    orderBadge = new QLabel(order.orderNumber);
    QFont badgeFont; badgeFont.setPointSize(9); badgeFont.setBold(true);
    orderBadge->setFont(badgeFont);
    orderBadge->setStyleSheet(
        "QLabel { background: #2196F3; color: white; border-radius: 8px; padding: 2px 14px; font-weight: 700; min-width: 110px; max-width: 180px; margin-left: 4px; }"
    );
    orderBadge->setAlignment(Qt::AlignCenter);
    bottomRow->addWidget(orderBadge);
    arrowBtn = new QToolButton;
    arrowBtn->setArrowType(Qt::DownArrow);
    arrowBtn->setCheckable(true);
    arrowBtn->setChecked(false);
    arrowBtn->setStyleSheet("QToolButton { border: none; background: transparent; min-width: 14px; min-height: 14px; }");
    connect(arrowBtn, &QToolButton::toggled, this, &OrderCard::arrowToggled);
    bottomRow->addWidget(arrowBtn);
    mainLayout->addLayout(bottomRow);
    mainLayout->addLayout(doneRow);

    connect(statusBtn1, &QPushButton::clicked, this, [this]() {
        if (order.status != Order::Przyjete) {
            updateOrderStatus(Order::Przyjete);
            updateStatusButtons();
        }
    });
    connect(statusBtn2, &QPushButton::clicked, this, [this]() {
        if (order.status != Order::Produkcja) {
            updateOrderStatus(Order::Produkcja);
            updateStatusButtons();
        }
    });
    connect(statusBtn3, &QPushButton::clicked, this, [this]() {
        if (order.status != Order::Gotowe) {
            updateOrderStatus(Order::Gotowe);
            updateStatusButtons();
        }
    });

    updateStatusButtons();
}

void OrderCard::setDateLabelColor(int daysLeft) {
    if (daysLeft < 0)
        dateLabel->setStyleSheet("color: #d32f2f; font-weight: 700;");
    else if (daysLeft == 0)
        dateLabel->setStyleSheet("color: #f57c00; font-weight: 700;");
    else if (daysLeft <= 2)
        dateLabel->setStyleSheet("color: #1976d2; font-weight: 700;");
    else
        dateLabel->setStyleSheet("color: #388e3c; font-weight: 600;");
}

int OrderCard::countWorkdays(const QDate& start, const QDate& end) const {
    if (start == end) return 0;
    int step = (end > start) ? 1 : -1;
    int count = 0;
    QDate current = start;
    while (current != end) {
        current = current.addDays(step);
        if (current.dayOfWeek() < 6) count += step;
    }
    return count;
}

void OrderCard::arrowToggled(bool checked) {
    qDebug() << "[OrderCard] arrowToggled checked=" << checked;
    if (checked) {
        showDetailsDialog();
    } else {
        if (detailsDialog && detailsDialog->isVisible()) {
            qDebug() << "[OrderCard] Zamykam detailsDialog";
            detailsDialog->close();
        }
    }
}

void OrderCard::showDetailsDialog() {
    qDebug() << "[OrderCard] showDetailsDialog";
    if (detailsDialog && detailsDialog->isVisible()) {
        qDebug() << "[OrderCard] detailsDialog już otwarty, podnoszę okno";
        detailsDialog->raise();
        detailsDialog->activateWindow();
        return;
    }
    detailsDialog = new QDialog(nullptr); // bez rodzica
    detailsDialog->setWindowTitle("Szczegóły produkcji zamówienia");
    QVBoxLayout* layout = new QVBoxLayout(detailsDialog);
    // Nowy widok tabeli produkcji
    ProductionTableView* table = new ProductionTableView(detailsDialog);
    auto items = DbManager::instance().getOrderItems(order.id);
    table->setOrderItems(items);
    layout->addWidget(table);
    detailsDialog->setLayout(layout);
    detailsDialog->setMinimumWidth(1200);
    detailsDialog->setMinimumHeight(320);
    detailsDialog->show();
    detailsDialog->raise();
    detailsDialog->activateWindow();
    qDebug() << "[OrderCard] detailsDialog utworzony i pokazany";
}

void OrderCard::markAsDone() {
    QMessageBox msgbox(this);
    msgbox.setIcon(QMessageBox::NoIcon);
    msgbox.setWindowTitle("Potwierdzenie");
    msgbox.setText("Czy chcesz oznać zamówienie Nr " + order.orderNumber + " jako zrealizowane?");
    msgbox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgbox.setDefaultButton(QMessageBox::No);
    msgbox.setStyleSheet("QLabel { min-width: 320px; }");
    if (msgbox.exec() == QMessageBox::Yes) {
        qDebug() << "[OrderCard] Użytkownik potwierdził zmianę statusu";
        
        // Zapisz numer zamówienia przed aktualizacją
        QString orderNumber = order.orderNumber;
        int orderId = order.id;
        
        // Ustaw status na "Zrealizowane" w bazie danych
        QSqlQuery q(DbManager::instance().database());
        q.prepare("UPDATE orders SET status=? WHERE id=?");
        q.addBindValue(static_cast<int>(Order::Zrealizowane));
        q.addBindValue(orderId);
        if (!q.exec()) {
            qDebug() << "[OrderCard] Błąd SQL przy update statusu na Zrealizowane: " << q.lastError().text();
            QMessageBox::warning(this, "Błąd bazy", "Nie udało się zaktualizować statusu zamówienia: " + q.lastError().text());
            return;
        }
        
        qDebug() << "[OrderCard] Status zamówienia" << orderNumber << "zmieniony na Zrealizowane w bazie";
        
        // Emituj sygnał o zmianie statusu
        qDebug() << "[OrderCard] Emitowanie sygnału orderStatusChanged";
        emit orderStatusChanged(orderId, Order::Zrealizowane);
        
        // Ukryj kartę natychmiast
        qDebug() << "[OrderCard] Ukrywanie karty";
        this->hide();
        
        // Pokazuj komunikat sukcesu
        qDebug() << "[OrderCard] Pokazywanie komunikatu sukcesu";
        QMessageBox::information(nullptr, "Status zmieniony", 
            QString("Zamówienie %1 zostało oznaczone jako zrealizowane i usunięte z dashboard.").arg(orderNumber));
        
        // Oznacz kartę do usunięcia z opóźnieniem
        QTimer::singleShot(100, this, [this]() {
            this->deleteLater();
        });
        
        qDebug() << "[OrderCard] markAsDone zakończony pomyślnie";
    }
}

void OrderCard::updateOrderStatus(Order::Status newStatus) {
    qDebug() << "[OrderCard] updateOrderStatus: id=" << order.id << ", stary status=" << order.status << ", nowy status=" << newStatus;
    if (order.status == newStatus) {
        qDebug() << "[OrderCard] Status bez zmian, nie aktualizuję.";
        return;
    }
    order.status = newStatus;
    // Aktualizacja w bazie
    QSqlQuery q(DbManager::instance().database());
    q.prepare("UPDATE orders SET status=? WHERE id=?");
    q.addBindValue(static_cast<int>(newStatus));
    q.addBindValue(order.id);
    if (!q.exec()) {
        qDebug() << "[OrderCard] Błąd SQL przy update statusu: " << q.lastError().text();
        QMessageBox::warning(this, "Błąd bazy", "Nie udało się zaktualizować statusu zamówienia: " + q.lastError().text());
        return;
    }
    qDebug() << "[OrderCard] Status zamówienia zaktualizowany w bazie.";
    // Emituj sygnał o zmianie statusu
    emit orderStatusChanged(order.id, newStatus);
    // Przyciski są już aktualizowane przez updateStatusButtons() w onclick handlerach
}

void OrderCard::updateStatusButtons() {
    auto setStatusBtnStyle = [](QPushButton* btn, bool active) {
        if (active)
            btn->setStyleSheet("border:2px solid #197a3d; border-radius:14px; background:#1ecb5a;");
        else
            btn->setStyleSheet("border:2px solid #197a3d; border-radius:14px; background:transparent;");
    };
    
    // Tymczasowo wyłącz aktualizacje podczas zmiany stylu
    setUpdatesEnabled(false);
    
    setStatusBtnStyle(statusBtn1, order.status == Order::Przyjete);
    setStatusBtnStyle(statusBtn2, order.status == Order::Produkcja);
    setStatusBtnStyle(statusBtn3, order.status == Order::Gotowe);
    
    // Włącz aktualizacje z powrotem
    setUpdatesEnabled(true);
    update();
}

void OrderCard::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton)
        dragStartPos = event->pos();
    QFrame::mousePressEvent(event);
}

void OrderCard::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        if ((event->pos() - dragStartPos).manhattanLength() > 5) {
            QDrag* drag = new QDrag(this);
            QMimeData* mime = new QMimeData;
            mime->setData("application/x-order-id", QByteArray::number(order.id));
            drag->setMimeData(mime);
            drag->exec(Qt::MoveAction);
        }
    }
    QFrame::mouseMoveEvent(event);
}

void OrderCard::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit orderDoubleClicked(order.id);
    }
    QFrame::mouseDoubleClickEvent(event);
}
