#include "sidebar_manager.h"
#include "models/user.h"
#include "views/status_legend_widget.h"
#include <QApplication>
#include <QLabel>

SidebarManager::SidebarManager(QWidget *parent)
    : QObject(parent), m_parent(parent), m_sidebar(nullptr), m_menuGroup(nullptr) {
}

QFrame* SidebarManager::createSidebar() {
    m_sidebar = new QFrame(m_parent);
    m_sidebar->setFrameShape(QFrame::StyledPanel);
    m_sidebar->setFixedWidth(260);
    m_sidebar->setStyleSheet("QFrame { background: #e5e7eb; border-top-right-radius: 18px; border-bottom-right-radius: 18px; }");

    m_layout = new QVBoxLayout(m_sidebar);
    m_layout->setSpacing(18);
    m_layout->setContentsMargins(18, 32, 18, 18);

    setupButtons();
    setupUserInfo();
    setupLegend();

    return m_sidebar;
}

void SidebarManager::setupButtons() {
    m_menuGroup = new QButtonGroup(this);
    m_menuGroup->setExclusive(true); // Ustaw grupę jako ekskluzywną

    btnDashboard = new QPushButton("Tablica zleceń", m_sidebar);
    btnNewOrder = new QPushButton("Wystaw zamówienie", m_sidebar);
    btnOrdersDb = new QPushButton("Baza zamówień", m_sidebar);
    btnClientsDb = new QPushButton("Baza klientów", m_sidebar);
    btnProductionSummary = new QPushButton("Zestawienie produkcji", m_sidebar);
    btnOrdersPdf = new QPushButton("Podgląd\nzamówień", m_sidebar);
    btnMaterialsPdf = new QPushButton("zamawianie\nmateriałów", m_sidebar);
    btnMaterialsOrdersDb = new QPushButton("baza zamówień\nmateriałów", m_sidebar); // NOWY PRZYCISK

    // Upewnij się, że wszystkie przyciski są checkable
    btnDashboard->setCheckable(true);
    btnNewOrder->setCheckable(true);
    btnOrdersDb->setCheckable(true);
    btnClientsDb->setCheckable(true);
    btnProductionSummary->setCheckable(true);
    btnOrdersPdf->setCheckable(true);
    btnMaterialsPdf->setCheckable(true);
    btnMaterialsOrdersDb->setCheckable(true);

    styleButton(btnDashboard, true);
    styleButton(btnNewOrder);
    styleButton(btnOrdersDb);
    styleButton(btnClientsDb);
    styleButton(btnProductionSummary);
    styleButton(btnOrdersPdf);
    btnOrdersPdf->setStyleSheet(
        "QPushButton { background: #e0edff; color: #222; font-size: 15px; border-radius: 10px; margin-bottom: 2px; padding-top: 6px; padding-bottom: 6px; line-height: 1.1; } "
        "QPushButton:checked { background: #1e40af; color: #fff; } "
        "QPushButton:hover { background: #2563eb; color: #fff; } "
        "QPushButton { text-align: center; }"
    );
    styleButton(btnMaterialsPdf);
    btnMaterialsPdf->setStyleSheet(
        "QPushButton { background: #e0edff; color: #222; font-size: 15px; border-radius: 10px; margin-bottom: 2px; padding-top: 6px; padding-bottom: 6px; line-height: 1.1; } "
        "QPushButton:checked { background: #1e40af; color: #fff; } "
        "QPushButton:hover { background: #2563eb; color: #fff; } "
        "QPushButton { text-align: center; }"
    );
    styleButton(btnMaterialsOrdersDb); // Stylizacja nowego przycisku
    btnMaterialsOrdersDb->setStyleSheet(
        "QPushButton { background: #e0edff; color: #222; font-size: 15px; border-radius: 10px; margin-bottom: 2px; padding-top: 6px; padding-bottom: 6px; line-height: 1.1; } "
        "QPushButton:checked { background: #1e40af; color: #fff; } "
        "QPushButton:hover { background: #2563eb; color: #fff; } "
        "QPushButton { text-align: center; }"
    );

    m_menuGroup->addButton(btnDashboard, 0);
    m_menuGroup->addButton(btnNewOrder, 1);
    m_menuGroup->addButton(btnOrdersDb, 2);
    m_menuGroup->addButton(btnClientsDb, 3);
    m_menuGroup->addButton(btnProductionSummary, 4);
    m_menuGroup->addButton(btnOrdersPdf, 5);
    m_menuGroup->addButton(btnMaterialsPdf, 6);
    m_menuGroup->addButton(btnMaterialsOrdersDb, 7); // Dodaj do grupy nowy przycisk

    m_layout->addWidget(btnDashboard);
    m_layout->addWidget(btnNewOrder);
    m_layout->addWidget(btnOrdersDb);
    m_layout->addWidget(btnClientsDb);
    m_layout->addWidget(btnProductionSummary);
    m_layout->addWidget(btnOrdersPdf);
    m_layout->addWidget(btnMaterialsPdf);
    m_layout->addWidget(btnMaterialsOrdersDb); // Dodaj do layoutu pod materiałami
    m_layout->addStretch();

    // Przyciski systemowe
    btnSettings = new QPushButton("Ustawienia", m_sidebar);
    btnLogout = new QPushButton("Wyloguj", m_sidebar);
    btnClose = new QPushButton("Zamknij", m_sidebar);

    styleButton(btnSettings);
    styleButton(btnLogout);
    styleButton(btnClose);

    m_layout->addWidget(btnSettings);
    m_layout->addWidget(btnLogout);
    m_layout->addWidget(btnClose);

    // Połączenia sygnałów
    connect(btnSettings, &QPushButton::clicked, this, &SidebarManager::settingsRequested);
    connect(btnLogout, &QPushButton::clicked, this, &SidebarManager::logoutRequested);
    connect(btnClose, &QPushButton::clicked, this, &SidebarManager::closeRequested);
    // Nowy przycisk menu - do podpięcia w MainWindow/PageManager
    // connect(btnMaterialsOrdersDb, &QPushButton::clicked, ...);
}

void SidebarManager::setupUserInfo() {
    m_userInfoLabel = new QLabel(m_sidebar);
    m_userInfoLabel->setWordWrap(true);
    m_userInfoLabel->setStyleSheet(
        "font-size: 15px; color: #444; background: #fff; border-radius: 8px; "
        "border: 1px solid #bbb; margin-top: 8px; margin-bottom: 8px; padding: 8px;"
    );
    m_layout->addWidget(m_userInfoLabel);
}

void SidebarManager::setupLegend() {
    auto *legendWidget = new StatusLegendWidget(m_sidebar);
    m_layout->addWidget(legendWidget, 0, Qt::AlignBottom);
}

void SidebarManager::styleButton(QPushButton *btn, bool checked) {
    btn->setMinimumHeight(44);
    btn->setCheckable(true);
    btn->setStyleSheet(QString(
        "QPushButton { background: %1; color: %2; font-size: 18px; border-radius: 10px; margin-bottom: 2px; } "
        "QPushButton:checked { background: #1e40af; color: #fff; } "
        "QPushButton:hover { background: #2563eb; color: #fff; } "
    ).arg(checked ? "#1e40af" : "#e0edff")
     .arg(checked ? "#fff" : "#222"));
    btn->setChecked(checked);
}

void SidebarManager::setUser(const User &user) {
    if (m_userInfoLabel) {
        QString userInfo = QString("<b>%1</b><br>%2<br>%3")
            .arg(user.getDisplayName())
            .arg(user.position)
            .arg(user.email);
        m_userInfoLabel->setText(userInfo);
    }
    
    // Ukryj przycisk ustawień dla użytkowników innych niż Administrator
    if (btnSettings) {
        btnSettings->setVisible(user.role == User::Role::Admin);
    }
}
