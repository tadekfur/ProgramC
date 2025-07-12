#include "pdf_drawing_helpers.h"
#include <QPainter>
#include <QRect>
#include <QString>

void drawMultiLineText(QPainter& painter, QRect rect, const QString& text, int lineHeight) {
    QStringList lines = text.split("\n");
    int y = rect.top();
    for (const QString& line : lines) {
        painter.drawText(rect.left(), y + lineHeight, rect.width(), lineHeight, Qt::AlignLeft | Qt::AlignVCenter, line);
        y += lineHeight;
    }
}
