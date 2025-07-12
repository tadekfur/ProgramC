#pragma once

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QItemSelection>
#include <QLineEdit>
#include <QComboBox>
#include <QSettings>
#include <QKeyEvent>
#include <QStatusBar>
#include <QToolTip>

class ClientsDbView : public QWidget {
    Q_OBJECT
public:
    explicit ClientsDbView(QWidget *parent = nullptr, bool selectMode = false);
    ~ClientsDbView();
protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void addClient(); // teraz: wstaw klienta do zamówienia
    void editClient();
    void deleteClient();
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

public slots:
    void refreshClients();

private:
    QTableView *tableView;
    QPushButton *btnAdd;
    QPushButton *btnEdit;
    QPushButton *btnDelete;
    QVBoxLayout *mainLayout;
    QHBoxLayout *btnLayout;
    QLineEdit *searchEdit;
    QComboBox *searchCombo;
    int selectedClientId = -1;
    QStatusBar *statusBar = nullptr;
    void setupUI();
    void loadClients();
    void saveTableState();
    void restoreTableState();
    void showStatus(const QString &msg, int timeoutMs = 2000);
    QMap<QString, QVariant> getSelectedClientData() const;
    QPushButton *btnSelectToOrder = nullptr;
    bool selectionMode = false;

signals:
    void clientSelected(const QMap<QString, QVariant> &clientData);
};
