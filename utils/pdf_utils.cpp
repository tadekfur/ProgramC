#include "pdf_utils.h"
#include <QDate>
#include <QFileInfo>
#include <QDir>
#include <QFontDatabase>
#include <QSettings>
#include <QDebug>

namespace PdfUtils {

QString formatPdfValue(const QVariant& value) {
    if (value.canConvert<QDate>())
        return value.toDate().toString("yyyy-MM-dd");
    return value.isNull() ? "" : value.toString();
}

QString formatCena(const QVariant& cena, const QVariant& cenaTyp) {
    if (cena.isNull() || cena.toString().isEmpty()) return "";
    QString typ = cenaTyp.toString().toLower().replace(".", "").replace(" ", "");
    typ.replace("ę", "e").replace("ł", "l").replace("ó", "o").replace("ą", "a")
       .replace("ś", "s").replace("ć", "c").replace("ń", "n").replace("ź", "z").replace("ż", "z");
    if (typ.contains("rol"))
        return QString("%1 /rolka").arg(cena.toString());
    if (typ.contains("tys") || typ.contains("tyś"))
        return QString("%1 /tyś").arg(cena.toString());
    return cena.toString();
}

void loadFonts() {
    static bool loaded = false;
    if (loaded) return;
    if (QFontDatabase::addApplicationFont(":/fonts/DejaVuSans.ttf") == -1) {
        qWarning() << "Failed to load font: DejaVuSans.ttf";
    }
    if (QFontDatabase::addApplicationFont(":/fonts/DejaVuSans-Bold.ttf") == -1) {
        qWarning() << "Failed to load font: DejaVuSans-Bold.ttf";
    }
    loaded = true;
}

void drawMultiLineText(QPainter& painter, QRect rect, const QString& text, int lineHeight) {
    QStringList lines = text.split("\n");
    int y = rect.top();
    for (const QString& line : lines) {
        painter.drawText(rect.left(), y + lineHeight, rect.width(), lineHeight, Qt::AlignLeft | Qt::AlignVCenter, line);
        y += lineHeight;
    }
}

int mmToPt(double mm) {
    return static_cast<int>(mm * 72.0 / 25.4);
}

QString getOrderValue(const QMap<QString, QVariant>& order, std::initializer_list<const char*> keys) {
    for (const char* key : keys) {
        if (order.contains(key) && !order.value(key).toString().isEmpty())
            return order.value(key).toString();
    }
    return "";
}

QString getValueMultiKey(const QMap<QString, QVariant>& map, std::initializer_list<const char*> keys) {
    for (const char* key : keys) {
        if (map.contains(key) && !map.value(key).toString().isEmpty())
            return map.value(key).toString();
    }
    return "";
}

QString getPdfOutputPath(const QString& fileName, const QString& defaultDir) {
    QSettings settings;
    QString dir = settings.value("pdf/outputDir", defaultDir).toString();
    if (dir.isEmpty()) dir = defaultDir;
    QDir outDir(dir);
    if (!outDir.exists()) outDir.mkpath(".");
    return outDir.filePath(fileName);
}

} // namespace PdfUtils
