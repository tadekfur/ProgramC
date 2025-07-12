#ifndef QR_CODE_GENERATOR_H
#define QR_CODE_GENERATOR_H

#include <QImage>
#include <QString>

QImage generateQrCode(const QString& data, int size = 128);

#endif // QR_CODE_GENERATOR_H
