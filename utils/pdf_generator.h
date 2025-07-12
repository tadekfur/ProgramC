#pragma once

#include <QMap>
#include <QVariant>
#include <QList>
#include <QString>
#include "../models/user.h"

class User;

bool generateOrderConfirmationPDF(const QMap<QString, QVariant>& orderData,
                                  const QMap<QString, QVariant>& clientData,
                                  const QList<QMap<QString, QVariant>>& orderItems,
                                  const QString& outputPath,
                                  const User& currentUser);

bool generateProductionTicketPDF(const QMap<QString, QVariant>& orderData,
                                 const QMap<QString, QVariant>& clientData,
                                 const QList<QMap<QString, QVariant>>& orderItems,
                                 const QString& outputPath);


