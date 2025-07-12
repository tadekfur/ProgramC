#ifndef PDF_UTILS_H
#define PDF_UTILS_H

#include <QVariant>
#include <QString>
#include <QMap>
#include <QList>
#include <QPainter>
#include <QRect>

namespace PdfUtils {
    QString formatPdfValue(const QVariant& value);
    QString formatCena(const QVariant& cena, const QVariant& cenaTyp);
    void loadFonts();
    void drawMultiLineText(QPainter& painter, QRect rect, const QString& text, int lineHeight);
    int mmToPt(double mm);
    QString getOrderValue(const QMap<QString, QVariant>& order, std::initializer_list<const char*> keys);
    QString getValueMultiKey(const QMap<QString, QVariant>& map, std::initializer_list<const char*> keys);
    QString getPdfOutputPath(const QString& fileName, const QString& defaultDir = QString());
}

#endif // PDF_UTILS_H
