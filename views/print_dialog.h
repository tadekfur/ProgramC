#pragma once

#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include "models/order.h"

class EmailSender;

class PrintDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit PrintDialog(int orderId, QWidget *parent = nullptr);
    
private slots:
    void printConfirmation();
    void printAndEmailConfirmation();
    void printProduction();
    void onEmailSent(bool success, const QString &message);
    
private:
    void setupUI();
    
    int m_orderId;
    
    // Grupa potwierdzenie dla klienta
    QGroupBox *clientConfirmationGroup;
    QPushButton *btnPrintConfirmation;
    QPushButton *btnPrintAndEmail;
    
    // Grupa produkcja  
    QGroupBox *productionGroup;
    QPushButton *btnPrintProduction;
    
    // Layout
    QVBoxLayout *mainLayout;
    
    // Email sender
    EmailSender *m_emailSender;
};
