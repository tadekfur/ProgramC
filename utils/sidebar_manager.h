#pragma once

#include <QWidget>
#include <QFrame>
#include <QPushButton>
#include <QLabel>
#include <QButtonGroup>
#include <QVBoxLayout>

class User;

class SidebarManager : public QObject {
    Q_OBJECT

public:
    explicit SidebarManager(QWidget *parent = nullptr);
    
    QFrame* createSidebar();
    void setUser(const User &user);
    
    // Gettery dla przycisków
    QPushButton* dashboardButton() const { return btnDashboard; }
    QPushButton* newOrderButton() const { return btnNewOrder; }
    QPushButton* ordersDbButton() const { return btnOrdersDb; }
    QPushButton* clientsDbButton() const { return btnClientsDb; }
    QPushButton* productionSummaryButton() const { return btnProductionSummary; }
    QPushButton* ordersPdfButton() const { return btnOrdersPdf; }
    QPushButton* materialsPdfButton() const { return btnMaterialsPdf; }
    QPushButton* settingsButton() const { return btnSettings; }
    QPushButton* logoutButton() const { return btnLogout; }
    QPushButton* closeButton() const { return btnClose; }
    QPushButton* materialsOrdersDbButton() const { return btnMaterialsOrdersDb; } // NOWY PRZYCISK
    
    QButtonGroup* menuGroup() const { return m_menuGroup; }

signals:
    void settingsRequested();
    void logoutRequested();
    void closeRequested();

private:
    void setupButtons();
    void setupUserInfo();
    void setupLegend();
    void styleButton(QPushButton *btn, bool checked = false);
    
    QWidget *m_parent;
    QFrame *m_sidebar;
    QVBoxLayout *m_layout;
    QButtonGroup *m_menuGroup;
    QLabel *m_userInfoLabel;
    
    // Przyciski menu
    QPushButton *btnDashboard;
    QPushButton *btnNewOrder;
    QPushButton *btnOrdersDb;
    QPushButton *btnClientsDb;
    QPushButton *btnProductionSummary;
    QPushButton *btnOrdersPdf;
    QPushButton *btnMaterialsPdf;
    QPushButton *btnMaterialsOrdersDb; // NOWY PRZYCISK
    QPushButton *btnSettings;
    QPushButton *btnLogout;
    QPushButton *btnClose;
};
