#ifndef PDF_DRAWING_HELPERS_H
#define PDF_DRAWING_HELPERS_H

#include <QPainter>
#include <QRect>
#include <QString>

void drawMultiLineText(QPainter& painter, QRect rect, const QString& text, int lineHeight);

#endif // PDF_DRAWING_HELPERS_H
