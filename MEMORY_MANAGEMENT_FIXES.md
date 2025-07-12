// PRZYKŁAD POPRAWEK DLA ZARZĄDZANIA PAMIĘCIĄ

// === PRZED (przykład z main.cpp) ===
LoginDialog* loginDialog = new LoginDialog(UserManager::instance().users(), nullptr);
MainWindow* w = new MainWindow();

// === PO ===
auto loginDialog = std::make_unique<LoginDialog>(UserManager::instance().users(), nullptr);
auto w = std::make_unique<MainWindow>();

// Lub lepiej z Qt parent system:
LoginDialog* loginDialog = new LoginDialog(UserManager::instance().users(), qApp);
MainWindow* w = new MainWindow();
w->setAttribute(Qt::WA_DeleteOnClose); // już jest

// === PRZED (MainWindow::MainWindow) ===
btnDashboard = new QPushButton("Tablica zleceń", sidebar);
DashboardView* dashboardView = new DashboardView();

// === PO ===
btnDashboard = new QPushButton("Tablica zleceń", sidebar); // parent określony = OK
auto dashboardView = new DashboardView(scroll); // parent określony

// === REKOMENDACJA: Dodaj do konstruktorów parent ===
// W views/*.cpp należy zawsze przekazywać parent widget
