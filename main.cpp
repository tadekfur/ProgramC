#include <QApplication>
#include <QMessageBox>
#include <QCoreApplication>
#include <memory>
#include "mainwindow.h"
#include "views/login_dialog.h"
#include "utils/secure_user_manager.h"
#include "utils/session_manager.h"
#include "utils/secure_config.h"
#include "db/dbmanager.h"
#include <QDebug>

// Globalny wskaźnik na okno główne, aby uniknąć tworzenia wielu instancji
MainWindow* g_mainWindow = nullptr;

// Forward declarations
void setupApplicationMetadata();
void showMainWindow(const User &user, LoginDialog *loginDialog);

int main(int argc, char *argv[])
{
    qDebug() << "[DEBUG] main() start";
    // Setup application metadata FIRST
    setupApplicationMetadata();
    
    QApplication app(argc, argv);
    // Pozwalamy aplikacji działać nawet gdy nie ma widocznych okien
    app.setQuitOnLastWindowClosed(false);
    app.setStyle("Fusion");
    
    try {
        // Initialize SecureUserManager
        SecureUserManager::instance().loadUsersFromFile();
        
        // Initialize SessionManager
        SessionManager& sessionManager = SessionManager::instance();
        
        // Tworzenie okna logowania z nullptr jako rodzicem
        // Używamy normalnego wskaźnika, ponieważ chcemy zachować kontrolę nad oknem
        auto loginDialog = new LoginDialog(
            SecureUserManager::instance().users(), nullptr);
        
        // Nie ustawiamy flagi WA_DeleteOnClose dla okna logowania
        
        // Setup login success handling
        QObject::connect(loginDialog, &QDialog::accepted, [loginDialog, &sessionManager]() {
            qDebug() << "[DEBUG] LoginDialog accepted";
            try {
                User selectedUser = loginDialog->selectedUser();
                
                // Create session
                QString sessionId = sessionManager.createSession(selectedUser);
                qDebug() << "[DEBUG] Session created:" << sessionId;
                
                showMainWindow(selectedUser, loginDialog);
                
            } catch (const std::exception &e) {
                QMessageBox::critical(qApp->activeWindow(), "Error in login handler", 
                    QString("Exception: %1").arg(e.what()));
            } catch (...) {
                QMessageBox::critical(qApp->activeWindow(), "Error in login handler", 
                    "Unknown exception occurred!");
            }
        });
        
        // Setup application quit on login dialog rejection
        QObject::connect(loginDialog, &QDialog::rejected, &app, &QApplication::quit);
        
        // Setup session expiry handling
        QObject::connect(&sessionManager, &SessionManager::sessionExpired, [loginDialog]() {
            qDebug() << "[DEBUG] Session expired";
            if (g_mainWindow) {
                g_mainWindow->hide();
            }
            if (loginDialog) {
                loginDialog->refreshUserList(); // Odśwież listę użytkowników
                loginDialog->show();
                loginDialog->raise();
                loginDialog->activateWindow();
                loginDialog->resetPasswordField();
            }
            QMessageBox::information(qApp->activeWindow(), "Session Expired", 
                "Your session has expired. Please log in again.");
        });
        
        // Setup inactivity warning
        QObject::connect(&sessionManager, &SessionManager::inactivityWarning, [](int secondsLeft) {
            qDebug() << "[DEBUG] Inactivity warning:" << secondsLeft << "seconds left";
            QMessageBox::warning(qApp->activeWindow(), "Session Warning", 
                QString("Your session will expire in %1 seconds due to inactivity.").arg(secondsLeft));
        });
        
        loginDialog->show();
        loginDialog->raise();
        loginDialog->activateWindow();
        
        return app.exec();
        
    } catch (const std::exception &e) {
        QMessageBox::critical(qApp->activeWindow(), "Critical Error", 
                            QString("Application failed to start: %1").arg(e.what()));
        return 1;
    }
}

void setupApplicationMetadata() {
    QCoreApplication::setOrganizationName("TwojaFirma");
    QCoreApplication::setApplicationName("EtykietyManager");
    QCoreApplication::setApplicationVersion("2.0.0");
}

void showMainWindow(const User &user, LoginDialog *loginDialog) {
    qDebug() << "[DEBUG] showMainWindow() start";
    try {
        if (!g_mainWindow) {
            g_mainWindow = new MainWindow(nullptr);
            qDebug() << "[DEBUG] MainWindow utworzony";
            QObject::connect(g_mainWindow, &MainWindow::logoutRequested, [loginDialog]() {
                qDebug() << "[DEBUG] Logout requested";
                // Destroy current session
                SessionManager::instance().destroySession(SessionManager::instance().getCurrentSessionId());
                
                if (loginDialog) {
                    if (g_mainWindow) {
                        g_mainWindow->hide();
                    }
                    loginDialog->refreshUserList(); // Odśwież listę użytkowników
                    loginDialog->show();
                    loginDialog->raise();
                    loginDialog->activateWindow();
                    loginDialog->resetPasswordField();
                }
            });
            QObject::connect(g_mainWindow, &QWidget::destroyed, qApp, [](){ 
                QApplication::quit(); 
            });
        }
        qDebug() << "[DEBUG] showMainWindow() - setUser";
        g_mainWindow->setUser(user);
        g_mainWindow->setProperty("logging_out", false);
        g_mainWindow->show();
        g_mainWindow->raise();
        g_mainWindow->activateWindow();
        qDebug() << "[DEBUG] showMainWindow() - MainWindow pokazany";
        if (loginDialog) {
            loginDialog->hide();
        }
        qDebug() << "[DEBUG] showMainWindow() end";
    } catch (const std::exception &e) {
        QMessageBox::critical(qApp->activeWindow(), "Error in showMainWindow", 
            QString("Exception: %1").arg(e.what()));
    }
}
