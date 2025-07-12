#pragma once
#include <QDialog>
#include <QTableWidget>
#include <QDialogButtonBox>
#include <QMap>
#include <QVariant>
#include <QVector>
#include <QPushButton>

class ClientSelectDialog : public QDialog {
    Q_OBJECT
public:
    explicit ClientSelectDialog(QWidget *parent = nullptr);
    QMap<QString, QVariant> selectedClient() const;
private slots:
    void onSelectionChanged();
private:
    QTableWidget *table;
    QDialogButtonBox *buttonBox;
    QMap<QString, QVariant> selected;
    void loadClients(const QString &filter = QString(), const QString &mode = QString("Wszystko"));
};
