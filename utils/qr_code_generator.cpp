#include "qr_code_generator.h"
#include "qrcodegen.hpp"
#include <QImage>
#include <QBuffer>

using qrcodegen::QrCode;
using qrcodegen::QrSegment;

QImage generateQrCode(const QString& data, int size) {
    QrCode qr = QrCode::encodeText(data.toUtf8().constData(), QrCode::Ecc::LOW);
    int qrSize = qr.getSize();
    QImage image(qrSize, qrSize, QImage::Format_RGB32);
    image.fill(Qt::white);
    for (int y = 0; y < qrSize; ++y) {
        for (int x = 0; x < qrSize; ++x) {
            if (qr.getModule(x, y))
                image.setPixel(x, y, qRgb(0, 0, 0));
        }
    }
    return image.scaled(size, size, Qt::KeepAspectRatio, Qt::FastTransformation);
}
