#include "mainwindow.h"
#include "utils/sidebar_manager.h"
#include "utils/page_manager.h"
#include "utils/settings_manager.h"
#include "utils/session_manager.h"
#include "views/settings_dialog.h"
#include "views/order_notification_dialog.h"
#include "views/clients_db_view.h"
#include "views/materials_pdf_page.h"
#include "views/materials_orders_db_view.h"
#include "network/notificationserver.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QScreen>
#include <QShortcut>
#include <QHostAddress>
#include <QMessageBox>
#include <QDebug>
#include <QCloseEvent>
#include <QStatusBar>
#include <QLabel>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) 
    : QMainWindow(parent), m_sidebarManager(nullptr), m_pageManager(nullptr),
      m_notifServer(nullptr), m_udpListener(nullptr), m_sessionStatusLabel(nullptr),
      m_sessionUpdateTimer(nullptr) {
    qDebug() << "[DEBUG] MainWindow konstruktor START";
    try {
        setupUI();
        setupSessionManagement();
        // Temporarily skip notifications setup (it was causing crashes)
        // TODO: Fix setupNotifications() later
        // setupNotifications();
        setupKeyboardShortcuts();
        connectSignals();
        qDebug() << "[DEBUG] MainWindow konstruktor END";
    } catch (const std::exception &e) {
        QMessageBox::critical(nullptr, "MainWindow Error", 
            QString("Exception in MainWindow constructor: %1").arg(e.what()));
    } catch (...) {
        QMessageBox::critical(nullptr, "MainWindow Error", 
            "Unknown exception in MainWindow constructor");
    }
}

void MainWindow::setupUI() {
    qDebug() << "[DEBUG] MainWindow::setupUI() START";
    // Ustaw pełny ekran i dopasowanie do rozdzielczości
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect geometry = screen->geometry();
    resize(geometry.width(), geometry.height());
    setWindowState(Qt::WindowMaximized);
    setStyleSheet(QString("QMainWindow { background: %1; }").arg("#f6f8fa"));

    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    // Utworz managery
    m_sidebarManager = new SidebarManager(this);
    m_pageManager = new PageManager(this);

    // Utworz UI
    auto *sidebar = m_sidebarManager->createSidebar();
    auto *pages = m_pageManager->createPages();

    // Layout główny
    auto *mainLayout = new QHBoxLayout(m_centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(m_pageManager->pagesWidget(), 1);
    qDebug() << "[DEBUG] MainWindow::setupUI() END";
}

void MainWindow::setupNotifications() {
    try {
        auto &settings = SettingsManager::instance();
        
        // Setup notification server and UDP listener
        m_notifServer = new NotificationServer(this);
        int notificationPort = settings.getNotificationServerPort();
        m_notifServer->startServer(notificationPort);
        
        if (!m_notifServer->isListening()) {
            qWarning() << "Failed to start notification server on port" << notificationPort;
            // Continue without notification server - not critical for basic functionality
        } else {
            qDebug() << "Notification server started on port" << notificationPort;
        }
        
        m_udpListener = new QUdpSocket(this);
        int udpPort = settings.getUdpListenerPort();
        if (!m_udpListener->bind(QHostAddress::Any, udpPort)) {
            qWarning() << "Failed to bind UDP listener on port" << udpPort << ":" << m_udpListener->errorString();
            // Continue without UDP listener - not critical for basic functionality
            return;
        } else {
            qDebug() << "UDP listener bound to port" << udpPort;
        }
        
        // Connect UDP notifications with error handling
        connect(m_udpListener, &QUdpSocket::readyRead, this, [this]() {
            try {
                while (m_udpListener->hasPendingDatagrams()) {
                    QByteArray datagram;
                    datagram.resize(m_udpListener->pendingDatagramSize());
                    
                    if (m_udpListener->readDatagram(datagram.data(), datagram.size()) == -1) {
                        qWarning() << "Failed to read UDP datagram:" << m_udpListener->errorString();
                        continue;
                    }
                    
                    handleNotificationMessage(QString::fromUtf8(datagram));
                }
            } catch (const std::exception &e) {
                qWarning() << "Error processing UDP notification:" << e.what();
            } catch (...) {
                qWarning() << "Unknown error processing UDP notification";
            }
        });
        
    } catch (const std::exception &e) {
        qCritical() << "Critical error setting up notifications:" << e.what();
        // Notifications are not critical - continue without them
        m_notifServer = nullptr;
        m_udpListener = nullptr;
    } catch (...) {
        qCritical() << "Unknown critical error setting up notifications";
        m_notifServer = nullptr;
        m_udpListener = nullptr;
    }
}

void MainWindow::setupKeyboardShortcuts() {
    // Skróty klawiszowe
    connect(new QShortcut(QKeySequence("Ctrl+1"), this), &QShortcut::activated, [this]() {
        m_sidebarManager->dashboardButton()->setChecked(true);
        m_pageManager->switchToPage(PageManager::Dashboard);
    });
    
    connect(new QShortcut(QKeySequence("Ctrl+2"), this), &QShortcut::activated, [this]() {
        m_sidebarManager->newOrderButton()->setChecked(true);
        m_pageManager->switchToPage(PageManager::NewOrder);
    });
    
    connect(new QShortcut(QKeySequence("Ctrl+3"), this), &QShortcut::activated, [this]() {
        m_sidebarManager->ordersDbButton()->setChecked(true);
        m_pageManager->switchToPage(PageManager::OrdersDb);
    });
    
    connect(new QShortcut(QKeySequence("Ctrl+4"), this), &QShortcut::activated, [this]() {
        m_sidebarManager->clientsDbButton()->setChecked(true);
        m_pageManager->switchToPage(PageManager::ClientsDb);
        if (m_pageManager->clientsDbView()) {
            qDebug() << "[MainWindow] Wywołuję refreshClients na ClientsDbView";
            m_pageManager->clientsDbView()->refreshClients();
        }
    });
    
    connect(new QShortcut(QKeySequence("Ctrl+5"), this), &QShortcut::activated, [this]() {
        m_sidebarManager->productionSummaryButton()->setChecked(true);
        m_pageManager->switchToPage(PageManager::ProductionSummary);
    });
    
    connect(new QShortcut(QKeySequence("Ctrl+6"), this), &QShortcut::activated, [this]() {
        m_sidebarManager->ordersPdfButton()->setChecked(true);
        m_pageManager->switchToPage(PageManager::OrdersPdf);
    });
    
    connect(new QShortcut(QKeySequence("Ctrl+7"), this), &QShortcut::activated, [this]() {
        m_sidebarManager->materialsPdfButton()->setChecked(true);
        m_pageManager->switchToPage(PageManager::MaterialsPdf);
    });
    
    connect(new QShortcut(QKeySequence("Ctrl+8"), this), &QShortcut::activated, [this]() {
        m_sidebarManager->materialsOrdersDbButton()->setChecked(true);
        m_pageManager->switchToPage(PageManager::MaterialsOrdersDb);
    });
}

void MainWindow::connectSignals() {
    // Połączenia sidebar manager
    connect(m_sidebarManager, &SidebarManager::settingsRequested, this, &MainWindow::openSettingsDialog);
    connect(m_sidebarManager, &SidebarManager::logoutRequested, this, &MainWindow::handleLogout);
    connect(m_sidebarManager, &SidebarManager::closeRequested, []() {
        qApp->exit(0);
    });

    // --- Połączenia przycisków menu z przełączaniem stron ---
    auto *menuGroup = m_sidebarManager->menuGroup();
    connect(menuGroup, QOverload<QAbstractButton *, bool>::of(&QButtonGroup::buttonToggled), 
            this, [this](QAbstractButton *btn, bool checked) {
        if (!checked) return;
        int buttonId = m_sidebarManager->menuGroup()->id(btn);
        // --- Jeśli kliknięto przycisk zamówień materiałów, wyczyść formularz ---
        if (buttonId == PageManager::MaterialsPdf) {
            QWidget* materialsPdfWidget = nullptr;
            // Użyj zawsze tego samego QStackedWidget!
            if (m_pageManager) {
                QStackedWidget* pages = m_pageManager->pagesWidget();
                if (pages && pages->count() > PageManager::MaterialsPdf)
                    materialsPdfWidget = pages->widget(PageManager::MaterialsPdf);
            }
            auto materialsPdfPage = dynamic_cast<MaterialsPdfPage*>(materialsPdfWidget);
            if (materialsPdfPage && materialsPdfPage->form) {
                materialsPdfPage->form->clearForm();
            }
        }
        m_pageManager->switchToPage(static_cast<PageManager::PageIndex>(buttonId));
        
        // Dodano: wymuś odświeżenie widoku klientów po przełączeniu na bazę klientów
        if (buttonId == PageManager::ClientsDb && m_pageManager->clientsDbView()) {
            qDebug() << "[MainWindow] (sidebar) Wywołuję refreshClients na ClientsDbView po kliknięciu w menu boczne";
            m_pageManager->clientsDbView()->refreshClients();
        }

        // Odświeżaj widok bazy zamówień materiałów przy każdym przejściu na ten widok
        if (buttonId == PageManager::MaterialsOrdersDb && m_pageManager->materialsOrdersDbView()) {
            auto dbView = qobject_cast<MaterialsOrdersDbView*>(m_pageManager->materialsOrdersDbView());
            if (dbView) dbView->refreshOrders();
        }
    });
    
    // Dodane połączenie: aktualizacja przycisków menu gdy strona zmieni się z innego powodu
    connect(m_pageManager, &PageManager::pageChanged, this, [this](PageManager::PageIndex page) {
        int pageIndex = static_cast<int>(page);
        QAbstractButton *button = m_sidebarManager->menuGroup()->button(pageIndex);
        if (button && !button->isChecked()) {
            // Tymczasowo blokujemy sygnały, żeby uniknąć pętli
            QSignalBlocker blocker(m_sidebarManager->menuGroup());
            
            // Odznacz wszystkie przyciski i ustaw zaznaczenie tylko dla aktywnej strony
            for (auto *btn : m_sidebarManager->menuGroup()->buttons()) {
                QPushButton *pushBtn = qobject_cast<QPushButton*>(btn);
                if (pushBtn) {
                    bool isActive = (btn == button);
                    pushBtn->setChecked(isActive);
                }
            }
        }
    });
    
    // Połącz sygnał clientAdded z NewOrderView z odświeżeniem bazy klientów
    if (m_pageManager && m_pageManager->clientsDbView()) {
        connect(this, SIGNAL(clientAdded()), m_pageManager->clientsDbView(), SLOT(refreshClients()));
    }
}

void MainWindow::setUser(const User &user) {
    qDebug() << "[DEBUG] MainWindow::setUser() START";
    m_currentUser = user;
    
    // Resetuj flagę wylogowania przy ponownym logowaniu
    setProperty("logging_out", false);
    
    if (m_sidebarManager) {
        m_sidebarManager->setUser(user);
    }
    if (m_pageManager) {
        m_pageManager->setCurrentUser(user);
    }
    qDebug() << "[DEBUG] MainWindow::setUser() END";
}

void MainWindow::openSettingsDialog() {
    // Sprawdź uprawnienia - tylko administratorzy mogą otwierać ustawienia
    if (m_currentUser.role != User::Role::Admin) {
        QMessageBox::warning(this, "Brak uprawnień", 
            "Tylko administratorzy mają dostęp do ustawień aplikacji.");
        return;
    }
    
    auto *dialog = new SettingsDialog(this);
    dialog->setWindowModality(Qt::ApplicationModal);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->exec();
}

void MainWindow::handleLogout() {
    // Ustaw właściwość, która informuje, że okno zamyka się z powodu wylogowania
    setProperty("logging_out", true);
    emit logoutRequested();
}

void MainWindow::refreshDashboard() {
    // Odśwież dashboard jeśli jest aktywny
    if (m_pageManager) {
        // TODO: Add refresh capability to dashboard
    }
}

void MainWindow::handleNotificationMessage(const QString &message) {
    try {
        if (message.startsWith("NEW_ORDER:")) {
            QString orderNumber = message.mid(10).trimmed();
            if (orderNumber.isEmpty()) {
                qWarning() << "Received NEW_ORDER notification with empty order number";
                return;
            }
            
            auto *dlg = new OrderNotificationDialog(orderNumber, this);
            if (!dlg) {
                qCritical() << "Failed to create OrderNotificationDialog";
                return;
            }
            
            dlg->setWindowModality(Qt::ApplicationModal);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            
            connect(dlg, &OrderNotificationDialog::removeOrder, this, 
                    [this, orderNumber]() {
                handleOrderRemoval(orderNumber);
            });
            
            dlg->exec();
            
        } else if (message.startsWith("REMOVE_ORDER:")) {
            QString orderNumber = message.mid(13).trimmed();
            if (orderNumber.isEmpty()) {
                qWarning() << "Received REMOVE_ORDER notification with empty order number";
                return;
            }
            
            handleOrderRemoval(orderNumber);
            
        } else {
            qDebug() << "Unknown notification message format:" << message;
        }
        
    } catch (const std::exception &e) {
        qWarning() << "Error handling notification message:" << e.what() << "Message:" << message;
    } catch (...) {
        qWarning() << "Unknown error handling notification message:" << message;
    }
}

void MainWindow::handleOrderRemoval(const QString &orderNumber) {
    try {
        if (orderNumber.isEmpty()) {
            qWarning() << "Cannot remove order: empty order number";
            return;
        }
        
        qDebug() << "Handling order removal for:" << orderNumber;
        
        // TODO: Implement actual order removal logic here
        // This should interact with the database and update views
        
        // For now, just refresh the dashboard and orders view
        refreshDashboard();
        
        // TODO: Add database operation here
        // DbManager::instance().removeOrder(orderNumber);
        
        // TODO: Update relevant views
        // if (m_pageManager && m_pageManager->ordersDbView()) {
        //     m_pageManager->ordersDbView()->refreshOrders();
        // }
        
    } catch (const std::exception &e) {
        qCritical() << "Error removing order" << orderNumber << ":" << e.what();
        QMessageBox::warning(this, "Error", 
                           QString("Failed to remove order %1: %2").arg(orderNumber, e.what()));
    } catch (...) {
        qCritical() << "Unknown error removing order" << orderNumber;
        QMessageBox::warning(this, "Error", 
                           QString("Failed to remove order %1 due to unknown error").arg(orderNumber));
    }
}

void MainWindow::sendOrderRemovalNotification(const QString &orderNumber) {
    try {
        if (orderNumber.isEmpty()) {
            qWarning() << "Cannot send removal notification: empty order number";
            return;
        }
        
        if (!m_udpListener) {
            qWarning() << "UDP listener not available, cannot send removal notification";
            return;
        }
        
        QUdpSocket udp;
        QByteArray datagram = QString("REMOVE_ORDER:%1").arg(orderNumber).toUtf8();
        
        qint64 result = udp.writeDatagram(datagram, QHostAddress::Broadcast, 9001);
        if (result == -1) {
            qWarning() << "Failed to send order removal notification:" << udp.errorString();
        } else {
            qDebug() << "Sent order removal notification for:" << orderNumber;
        }
        
    } catch (const std::exception &e) {
        qWarning() << "Error sending order removal notification:" << e.what();
    } catch (...) {
        qWarning() << "Unknown error sending order removal notification";
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    qDebug() << "[DEBUG] MainWindow::closeEvent() START";
    // Sprawdź, czy zamknięcie jest wynikiem wylogowania
    bool isLoggingOut = property("logging_out").toBool();
    
    if (isLoggingOut) {
        // Jeśli to jest wylogowanie, tylko ukryj okno i nie zamykaj aplikacji
        event->ignore(); // Nie pozwól na zamknięcie
        hide(); // Zamiast tego ukryj okno
        qDebug() << "[DEBUG] MainWindow::closeEvent() - logging out, hiding window";
        return;
    }
    
    // Pokaż dialog potwierdzenia zamknięcia aplikacji
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        "Zamykanie aplikacji", 
        "Czy na pewno chcesz zamknąć aplikację?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // Zaakceptuj zamknięcie i zakończ aplikację
        event->accept();
        // Musimy wywołać quit() zamiast close(), aby zakończyć całą aplikację
        QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
        qDebug() << "[DEBUG] MainWindow::closeEvent() - accepted, quitting app";
    } else {
        // Ignoruj zamknięcie
        event->ignore();
        qDebug() << "[DEBUG] MainWindow::closeEvent() - rejected by user";
    }
}

void MainWindow::setupSessionManagement() {
    qDebug() << "[DEBUG] MainWindow::setupSessionManagement() START";
    
    // Tworzenie etykiety statusu sesji
    m_sessionStatusLabel = new QLabel(this);
    m_sessionStatusLabel->setStyleSheet("QLabel { color: #666; font-size: 12px; }");
    
    // Dodanie do status bar
    statusBar()->addPermanentWidget(m_sessionStatusLabel);
    
    // Timer do aktualizacji statusu sesji
    m_sessionUpdateTimer = new QTimer(this);
    connect(m_sessionUpdateTimer, &QTimer::timeout, this, &MainWindow::updateSessionStatus);
    m_sessionUpdateTimer->start(30000); // Aktualizuj co 30 sekund
    
    // Wstępna aktualizacja statusu
    updateSessionStatus();
    
    qDebug() << "[DEBUG] MainWindow::setupSessionManagement() END";
}

void MainWindow::updateSessionStatus() {
    SessionManager& sessionManager = SessionManager::instance();
    
    if (!sessionManager.isLoggedIn()) {
        m_sessionStatusLabel->setText("Nie zalogowany");
        return;
    }
    
    QString sessionId = sessionManager.getCurrentSessionId();
    QDateTime expiry = sessionManager.getSessionExpiry(sessionId);
    
    if (expiry.isValid()) {
        QDateTime now = QDateTime::currentDateTime();
        int secondsLeft = now.secsTo(expiry);
        
        if (secondsLeft > 0) {
            int minutes = secondsLeft / 60;
            int seconds = secondsLeft % 60;
            
            QString timeStr;
            if (minutes > 0) {
                timeStr = QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
            } else {
                timeStr = QString("0:%1").arg(seconds, 2, 10, QChar('0'));
            }
            
            QString statusText = QString("Sesja: %1 | Wygasa za: %2")
                .arg(m_currentUser.login)
                .arg(timeStr);
            
            // Zmień kolor jeśli sesja wygasa wkrótce
            if (secondsLeft < 300) { // Mniej niż 5 minut
                m_sessionStatusLabel->setStyleSheet("QLabel { color: #ff6b6b; font-size: 12px; font-weight: bold; }");
            } else if (secondsLeft < 900) { // Mniej niż 15 minut
                m_sessionStatusLabel->setStyleSheet("QLabel { color: #ffa500; font-size: 12px; }");
            } else {
                m_sessionStatusLabel->setStyleSheet("QLabel { color: #666; font-size: 12px; }");
            }
            
            m_sessionStatusLabel->setText(statusText);
        } else {
            m_sessionStatusLabel->setText("Sesja wygasła");
            m_sessionStatusLabel->setStyleSheet("QLabel { color: #ff0000; font-size: 12px; font-weight: bold; }");
        }
    } else {
        m_sessionStatusLabel->setText("Błąd sesji");
        m_sessionStatusLabel->setStyleSheet("QLabel { color: #ff0000; font-size: 12px; }");
    }
}

void MainWindow::recordUserActivity() {
    SessionManager::instance().recordActivity();
    updateSessionStatus(); // Natychmiastowo zaktualizuj status
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    recordUserActivity();
    QMainWindow::mousePressEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    recordUserActivity();
    QMainWindow::keyPressEvent(event);
}

void MainWindow::focusInEvent(QFocusEvent *event) {
    recordUserActivity();
    QMainWindow::focusInEvent(event);
}
