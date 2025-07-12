#include "pdf_generator.h"
#include "pdf_utils.h"
#include "qr_code_generator.h"
#include "pdf_drawing_helpers.h"
#include "resource_manager.h" // Nowy include
#include <QPdfWriter>
#include <QPainter>
#include <QFont>
#include <QFileInfo>
#include <QDir>
#include <QDate>
#include <QFontDatabase>
#include <QImage>
#include <QBuffer>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>
#include <QSettings>

// --- Główna funkcja generująca PDF potwierdzenia zamówienia dla klienta ---
bool generateOrderConfirmationPDF(const QMap<QString, QVariant>& orderData,
                                  const QMap<QString, QVariant>& clientData,
                                  const QList<QMap<QString, QVariant>>& orderItems,
                                  const QString& outputPath,
                                  const User& currentUser)
{
    // Tworzymy lokalną kopię orderData i automatycznie uzupełniamy brakujące klucze
    QMap<QString, QVariant> orderDataFilled = orderData;
    if (!orderDataFilled.contains("Dane zamawiającego") || !orderDataFilled.value("Dane zamawiającego").isValid()) {
        orderDataFilled["Dane zamawiającego"] = clientData;
    }
    if (!orderDataFilled.contains("Adres dostawy") || !orderDataFilled.value("Adres dostawy").isValid()) {
        // Spróbuj zbudować adres dostawy z pól orderData lub clientData
        QMap<QString, QVariant> adres;
        // Najpierw z orderData (jeśli są)
        adres["short_name"] = orderData.value("delivery_company").toString().isEmpty() ? clientData.value("short_name") : orderData.value("delivery_company");
        adres["street"] = orderData.value("delivery_street").toString().isEmpty() ? clientData.value("street") : orderData.value("delivery_street");
        adres["postal_code"] = orderData.value("delivery_postal_code").toString().isEmpty() ? clientData.value("postal_code") : orderData.value("delivery_postal_code");
        adres["city"] = orderData.value("delivery_city").toString().isEmpty() ? clientData.value("city") : orderData.value("delivery_city");
        adres["contact_person"] = orderData.value("delivery_contact_person").toString().isEmpty() ? clientData.value("contact_person") : orderData.value("delivery_contact_person");
        adres["phone"] = orderData.value("delivery_phone").toString().isEmpty() ? clientData.value("phone") : orderData.value("delivery_phone");
        orderDataFilled["Adres dostawy"] = adres;
    }
    // Generate order confirmation PDF using ResourceManager  
    QFileInfo fi(outputPath);
    PdfUtils::loadFonts();
    // Upewnij się, że katalog docelowy istnieje
    QFileInfo outFi(outputPath);
    QDir outDir = outFi.dir();
    if (!outDir.exists()) {
        if (!outDir.mkpath(".")) {
            qWarning() << "Nie udało się utworzyć katalogu docelowego dla PDF:" << outDir.absolutePath();
            return false;
        }
    }
    QPdfWriter writer(outputPath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(10, 10, 10, 10));
    writer.setResolution(72); // Wymuszenie 72 DPI, 1pt = 1/72 cala
    QPainter painter;
    if (!painter.begin(&writer)) {
        qWarning() << "QPainter nie został poprawnie zainicjowany! Ścieżka:" << outputPath;
        return false;
    }

    // Marginesy i pozycje w mm
    int leftMargin = PdfUtils::mmToPt(10);
    int rightMargin = PdfUtils::mmToPt(10);
    int topMargin = PdfUtils::mmToPt(10);
    int blok_y = PdfUtils::mmToPt(15);
    int blok_x = leftMargin + PdfUtils::mmToPt(60);
    int ramka_w = PdfUtils::mmToPt(120);
    int ramka_h = PdfUtils::mmToPt(11);

    // --- Dane firmy Termedia ---
    int y = blok_y;
    painter.setFont(QFont("DejaVu Sans", 13, QFont::Bold));
    painter.setPen(Qt::black);
    painter.drawText(leftMargin, y, "TERMEDIA");
    painter.setFont(QFont("DejaVu Sans", 8));
    int yFirm = y + PdfUtils::mmToPt(7);
    painter.drawText(leftMargin, yFirm, "ul. Przemysłowa 60");
    painter.drawText(leftMargin, yFirm + PdfUtils::mmToPt(4), "43-110 Tychy");
    painter.drawText(leftMargin, yFirm + PdfUtils::mmToPt(8), "bok@termedialabels.pl");
    painter.drawText(leftMargin, yFirm + PdfUtils::mmToPt(12), "www.termedialabels.pl");
    painter.drawText(leftMargin, yFirm + PdfUtils::mmToPt(16), "+48 503 179 658");

    // --- Blok "Zamówienie" i nr zamówienia ---
    QColor szary(60, 60, 60); // ciemny szary
    painter.setPen(Qt::NoPen);
    painter.setBrush(szary);
    painter.drawRect(blok_x, blok_y, ramka_w, ramka_h);
    painter.setFont(QFont("DejaVu Sans", 11, QFont::Bold));
    painter.setPen(Qt::white);
    // Wyśrodkowanie pionowe tekstów w ramce
    int textHeight = PdfUtils::mmToPt(8); // wysokość tekstu
    int centerY = blok_y + (ramka_h - textHeight) / 2 + textHeight - PdfUtils::mmToPt(2);
    painter.drawText(blok_x + PdfUtils::mmToPt(4), centerY, "Zamówienie");
    QString nrZam = PdfUtils::getOrderValue(orderData, {"Nr zamówienia", "order_number"});
    painter.drawText(blok_x + PdfUtils::mmToPt(52), centerY, QString("Nr zamówienia: %1").arg(nrZam));
    painter.setPen(Qt::black);
    // Poprawione: wyświetlanie numeru klienta pod napisem Zamówienie, pobierane wyłącznie z clientData
    QString nrKlienta = PdfUtils::getOrderValue(clientData, {"Nr klienta", "client_number", "client_id"});
    if (nrKlienta.isEmpty()) {
        qWarning() << "Brak numeru klienta w clientData!";
    }
    painter.setFont(QFont("DejaVu Sans", 10)); // powiększona czcionka o 1 pkt
    int klientY = centerY + PdfUtils::mmToPt(9); // przesunięcie w dół o 2 mm względem poprzedniego (było +7 mm)
    painter.drawText(blok_x + PdfUtils::mmToPt(4), klientY, QString("Nr klienta: %1").arg(nrKlienta));
    painter.setFont(QFont("DejaVu Sans", 11, QFont::Bold));

    // --- Daty ---
    int przesuniecie_dol = PdfUtils::mmToPt(10); // było 20, przesuwamy o 10 mm wyżej
    int daty_y = PdfUtils::mmToPt(33) + przesuniecie_dol;
    painter.setFont(QFont("DejaVu Sans", 10, QFont::Bold));
    QColor jasnySzary(220, 220, 220); // jasny szary
    painter.setBrush(jasnySzary);
    painter.setPen(Qt::black);
    painter.drawRect(leftMargin, daty_y, PdfUtils::mmToPt(64), PdfUtils::mmToPt(8));
    // Przesuwamy tło i całość "Data wysyłki" w prawo o 15 mm względem poprzedniego położenia
    int wysylka_x = leftMargin + PdfUtils::mmToPt(68) + PdfUtils::mmToPt(35); // 20+15 mm względem oryginału
    painter.drawRect(wysylka_x, daty_y, PdfUtils::mmToPt(64), PdfUtils::mmToPt(8));
    painter.setPen(Qt::white);
    // Wyśrodkowanie pionowe tekstów na prostokątach
    int rectHeight = PdfUtils::mmToPt(8);
    QFontMetrics fm(painter.font());
    int textOffsetY = (rectHeight + fm.ascent() - fm.descent()) / 2;
    // Zmieniamy kolor czcionki na czarny dla napisów "Data zamówienia" i "Data wysyłki"
    painter.setPen(Qt::black);
    painter.drawText(leftMargin + PdfUtils::mmToPt(2), daty_y + textOffsetY, "Data zamówienia:");
    painter.drawText(wysylka_x + PdfUtils::mmToPt(2), daty_y + textOffsetY, "Data wysyłki:");
    painter.setFont(QFont("DejaVu Sans", 10));
    painter.setPen(Qt::black); // Ustaw czarny kolor dla wartości dat
    QString dataZam = PdfUtils::getOrderValue(orderData, {"Data zamówienia", "order_date"});
    QString dataDost = PdfUtils::getOrderValue(orderData, {"Data dostawy", "delivery_date"});
    painter.drawText(leftMargin + PdfUtils::mmToPt(2), daty_y + rectHeight + PdfUtils::mmToPt(7), dataZam); // poniżej prostokąta
    painter.drawText(wysylka_x + PdfUtils::mmToPt(2), daty_y + rectHeight + PdfUtils::mmToPt(7), dataDost);

    // --- Dane klienta (lewo) i adres dostawy (prawo) ---
    int klient_x = leftMargin + PdfUtils::mmToPt(5) - PdfUtils::mmToPt(7);
    int adres_blok_x = leftMargin + PdfUtils::mmToPt(90 - 20 + 15) + PdfUtils::mmToPt(5);
    int y_klient = daty_y + PdfUtils::mmToPt(18) + PdfUtils::mmToPt(10);
    painter.setFont(QFont("DejaVu Sans", 9, QFont::Bold));
    painter.setPen(Qt::black);
    painter.drawText(klient_x + PdfUtils::mmToPt(4), y_klient, "Dane klienta");
    painter.drawText(adres_blok_x + PdfUtils::mmToPt(4), y_klient, "Adres dostawy");
    painter.setFont(QFont("DejaVu Sans", 8));
    int y_info = y_klient + PdfUtils::mmToPt(7);

    // Dane klienta z sekcji zamówienia (Dane zamawiającego) - dokładnie jak w formularzu
    QMap<QString, QVariant> zamawiajacy = orderDataFilled.value("Dane zamawiającego").toMap();
    struct FieldLabelKey { QString label; const char* key; };
    std::vector<FieldLabelKey> clientFields = {
        {"Nazwa firmy", "name"},
        {"Skrócona nazwa", "short_name"},
        {"Osoba kontaktowa", "contact_person"},
        {"Telefon", "phone"},
        {"E-mail", "email"},
        {"Ulica i nr", "street"},
        {"Kod pocztowy", "postal_code"},
        {"Miasto", "city"},
        {"NIP", "nip"}
    };
    int idx = 0;
    for (const auto& field : clientFields) {
        QString val = zamawiajacy.value(field.key).toString();
        if (!val.isEmpty()) {
            painter.setFont(QFont("DejaVu Sans", 8, QFont::Bold));
            painter.drawText(klient_x + PdfUtils::mmToPt(3), y_info + idx * PdfUtils::mmToPt(6), field.label + ":");
            painter.setFont(QFont("DejaVu Sans", 8));
            // Przywrócenie oryginalnego przesunięcia bloku wartości danych klienta (50 mm)
            painter.drawText(klient_x + PdfUtils::mmToPt(3) + PdfUtils::mmToPt(35), y_info + idx * PdfUtils::mmToPt(6), val);
            idx++;
        }
    }

    // Adres dostawy z wybranego adresu dostawy - dokładnie jak w formularzu
    QMap<QString, QVariant> adres = orderDataFilled.value("Adres dostawy").toMap();
    std::vector<FieldLabelKey> addrFields = {
        {"Nazwa firmy", "short_name"}, // skrócona nazwa firmy zamiast pełnej
        {"Ulica i nr", "street"},
        {"Kod pocztowy", "postal_code"},
        {"Miasto", "city"},
        {"Osoba kontaktowa", "contact_person"},
        {"Telefon", "phone"}
    };
    int idxAdres = 0;
    int etykieta_x_addr = adres_blok_x + PdfUtils::mmToPt(3);
    int wartosc_x_addr = etykieta_x_addr + PdfUtils::mmToPt(35); // przesunięcie o 15 mm w lewo względem poprzedniego (było 50 mm)
    for (const auto& field : addrFields) {
        QString val = adres.value(field.key).toString();
        if (!val.isEmpty()) {
            painter.setFont(QFont("DejaVu Sans", 8, QFont::Bold));
            painter.drawText(etykieta_x_addr, y_info + idxAdres * PdfUtils::mmToPt(6), field.label + ":");
            painter.setFont(QFont("DejaVu Sans", 8));
            painter.drawText(wartosc_x_addr, y_info + idxAdres * PdfUtils::mmToPt(6), val);
            idxAdres++;
        }
    }
    int y_tabela = std::max(y_info + idx * PdfUtils::mmToPt(6), y_info + idxAdres * PdfUtils::mmToPt(6)) + PdfUtils::mmToPt(40);

    // --- Przygotowanie zmiennych tabeli produkcji ---
    std::vector<QString> headers = {"Lp.", "Wymiar", "Materiał", "Na rolce", "Rdzeń", "Ilość", "Miara", "zam. rolki", "Cena"};
    std::vector<std::vector<QString>> rows;
    int lp = 1;
    for (const auto& item : orderItems) {
        QString width = item.value("width").toString();
        QString height = item.value("height").toString();
        QString wymiar = (!width.isEmpty() && !height.isEmpty()) ? width+"x"+height : (width+height);
        QString material = item.value("material").toString();
        QString roll_length = item.value("roll_length").toString();
        QString core = item.value("core").toString();
        QString ordered_quantity = item.value("ordered_quantity").toString();
        QString miara = item.value("quantity_type").toString();
        QString zam_rolki = item.value("zam_rolki").toString();
        QString cena = item.value("price").toString();
        QString cena_typ = item.value("price_type").toString();
        QString cena_sufix = PdfUtils::formatCena(cena, cena_typ);
        rows.push_back({QString::number(lp++), wymiar, material, roll_length, core, ordered_quantity, miara, zam_rolki, cena_sufix});
    }
    // Szerokości kolumn (w punktach, możesz dostosować)
    std::vector<int> col_widths = {30, 60, 80, 60, 50, 50, 40, 60, 60};
    int table_x = leftMargin;
    int table_y = y_tabela;
    int row_height = 20; // pt

    // --- Nagłówek sekcji produkcyjnej ---
    painter.setFont(QFont("DejaVu Sans", 9, QFont::Bold));
    painter.setPen(Qt::black);
    painter.drawText(table_x, y_tabela - 24, 300, 20, Qt::AlignLeft | Qt::AlignVCenter, "Dane produkcji");

    // --- Tabela produkcji (analogicznie jak w produkcji) ---
    // Nagłówek tabeli
    painter.setFont(QFont("DejaVu Sans", 9, QFont::Bold));
    painter.setPen(Qt::black);
    int col_x = table_x;
    for (int i = 0; i < headers.size(); ++i) {
        painter.drawRect(col_x, table_y, col_widths[i], row_height);
        painter.drawText(col_x, table_y, col_widths[i], row_height, Qt::AlignCenter, headers[i]);
        col_x += col_widths[i];
    }
    // Wiersze tabeli
    painter.setFont(QFont("DejaVu Sans", 8));
    int row_y = table_y + row_height;
    for (int i = 0; i < rows.size(); ++i) {
        col_x = table_x;
        for (int j = 0; j < headers.size(); ++j) {
            painter.setBrush(Qt::white); // tło białe dla każdej komórki
            painter.setPen(Qt::black);
            painter.drawRect(col_x, row_y, col_widths[j], row_height);
            painter.drawText(col_x, row_y, col_widths[j], row_height, Qt::AlignCenter, rows[i][j]);
            col_x += col_widths[j];
        }
        row_y += row_height;
    }
    // --- Uwagi ---
    painter.setFont(QFont("DejaVu Sans", 8, QFont::Bold));
    painter.drawText(table_x, row_y + 10, 100, row_height, Qt::AlignLeft | Qt::AlignVCenter, "Uwagi:");
    painter.setFont(QFont("DejaVu Sans", 8));
    painter.drawText(table_x + 50, row_y + 10, 400, row_height, Qt::AlignLeft | Qt::AlignVCenter, orderData.value("notes").toString());

    // --- Prośba o potwierdzenie ---
    int y_confirm = row_y + row_height + 30;
    painter.setFont(QFont("DejaVu Sans", 8, QFont::Bold));
    painter.drawText(table_x, y_confirm, 600, row_height, Qt::AlignLeft | Qt::AlignVCenter,
        "Prosimy o zwrotne potwierdzenie zamówienia na adres bok@termedialabels.pl lub do swojego opiekuna handlowego.");

    // --- Wystawił ---
    int y_user = y_confirm + row_height + 10;
    painter.setFont(QFont("DejaVu Sans", 8, QFont::Bold));
    painter.drawText(table_x, y_user, 200, row_height, Qt::AlignLeft | Qt::AlignVCenter, QString("Wystawił: %1").arg(currentUser.getDisplayName()));

    // --- QR kod i tekst pod nim na dole strony, wyśrodkowane względem strony ---
    int pageWidth = writer.width();
    int pageHeight = writer.height();
    int qrSize = 90; // zmniejszony rozmiar QR
    int bottomMargin = PdfUtils::mmToPt(30); // większy margines od dołu
    QString qrText = QString("Nr zamówienia: %1\nNr klienta: %2\nData wysyłki: %3")
        .arg(nrZam)
        .arg(nrKlienta)
        .arg(dataDost);
    QImage qr = generateQrCode(qrText, qrSize);
    int qr_x = (pageWidth - qr.width()) / 2;
    int qr_y = pageHeight - bottomMargin - qr.height() - 2 * row_height - 8; // więcej miejsca na tekst
    painter.drawImage(qr_x, qr_y, qr);
    // Tekst pod QR
    painter.setFont(QFont("DejaVu Sans", 9));
    QString terminText = "Podany termin wysyłki jest orientacyjny i może ulec zmianie.";
    painter.drawText(qr_x, qr_y + qr.height() + 8, qr.width(), 2 * row_height, Qt::AlignCenter | Qt::AlignVCenter, terminText);

    painter.end();
    QDesktopServices::openUrl(QUrl::fromLocalFile(outputPath));
    return QFileInfo::exists(outputPath);
}

// --- GENEROWANIE PDF ZLECENIA PRODUKCYJNEGO ---
bool generateProductionTicketPDF(const QMap<QString, QVariant>& orderData,
                                const QMap<QString, QVariant>& clientData,
                                const QList<QMap<QString, QVariant>>& orderItems,
                                const QString& outputPath)
{
    PdfUtils::loadFonts();
    QPdfWriter writer(outputPath);
    writer.setPageSize(QPageSize(QPageSize::A5));
    writer.setResolution(300);
    writer.setPageMargins(QMarginsF(0, 0, 0, 0));
    QPainter painter;
    if (!painter.begin(&writer)) return false;

    // Stałe w mm
    const double margin_left = 10, margin_top = 10;
    const double page_width = 148, page_height = 210;
    const double ticket_height = 90, ticket_spacing = 10;
    const double cut_line_y = page_height / 2;

    // Kolory
    QColor black(0,0,0), border(180,180,200), grey(245,245,245), white(255,255,255);
    QColor blue(40,80,160), blue_bg(180,200,245), cut_gray(180,180,180), cut_text(120,120,120);

    // --- Funkcja rysująca sekcję produkcyjną ---
    auto drawTicket = [&](double y_offset_mm) {
        double x = margin_left, y = margin_top + y_offset_mm;
        double box_h = 10;
        // --- Górny czarny pasek i bloki dat ---
        painter.setPen(Qt::NoPen);
        painter.setBrush(black);
        painter.setFont(QFont("DejaVu Sans", 11, QFont::Bold)); // 11pt
        QRectF nrRect(PdfUtils::mmToPt(x), PdfUtils::mmToPt(y), PdfUtils::mmToPt(60), PdfUtils::mmToPt(box_h));
        painter.drawRect(nrRect);
        painter.setPen(Qt::white);
        painter.drawText(nrRect.adjusted(PdfUtils::mmToPt(2),0,0,0), Qt::AlignVCenter|Qt::AlignLeft,
            QString("Nr zamówienia: %1").arg(orderData.value("order_number").toString()));
        // Data zamówienia
        QString date_order_label = "Data zamówienia:";
        painter.setFont(QFont("DejaVu Sans", 9, QFont::Bold)); // 9pt
        double date_order_label_w_mm = 28; // stała szerokość w mm
        QRectF dateRect(PdfUtils::mmToPt(x+62), PdfUtils::mmToPt(y), PdfUtils::mmToPt(date_order_label_w_mm), PdfUtils::mmToPt(box_h));
        painter.setBrush(black);
        painter.setPen(Qt::NoPen);
        painter.drawRect(dateRect);
        painter.setPen(Qt::white);
        painter.drawText(dateRect.adjusted(PdfUtils::mmToPt(2),0,0,0), Qt::AlignVCenter|Qt::AlignLeft, date_order_label);
        // Data wysyłki
        QString wysylka_label = "Data wysyłki:";
        double wysylka_label_w_mm = 24; // stała szerokość w mm
        QRectF wysylkaRect(PdfUtils::mmToPt(x+62+date_order_label_w_mm+10), PdfUtils::mmToPt(y), PdfUtils::mmToPt(wysylka_label_w_mm), PdfUtils::mmToPt(box_h));
        painter.setBrush(black);
        painter.setPen(Qt::NoPen);
        painter.drawRect(wysylkaRect);
        painter.setPen(Qt::white);
        painter.drawText(wysylkaRect.adjusted(PdfUtils::mmToPt(2),0,0,0), Qt::AlignVCenter|Qt::AlignLeft, wysylka_label);
        // Daty wartości
        painter.setFont(QFont("DejaVu Sans", 9));
        painter.setPen(Qt::black);
        painter.drawText(dateRect.left(), dateRect.bottom()+PdfUtils::mmToPt(2), dateRect.width(), PdfUtils::mmToPt(6), Qt::AlignLeft|Qt::AlignVCenter,
            orderData.value("order_date").toString());
        painter.drawText(wysylkaRect.left(), wysylkaRect.bottom()+PdfUtils::mmToPt(2), wysylkaRect.width(), PdfUtils::mmToPt(6), Qt::AlignLeft|Qt::AlignVCenter,
            orderData.value("delivery_date").toString());
        y += box_h + 7;
        // --- Dane klienta i adres dostawy ---
        painter.setFont(QFont("DejaVu Sans", 9, QFont::Bold));
        painter.setPen(Qt::black);
        painter.drawText(PdfUtils::mmToPt(x), PdfUtils::mmToPt(y), PdfUtils::mmToPt(40), PdfUtils::mmToPt(5), Qt::AlignLeft|Qt::AlignVCenter, "Dane klienta");
        painter.drawText(PdfUtils::mmToPt(x+70), PdfUtils::mmToPt(y), PdfUtils::mmToPt(40), PdfUtils::mmToPt(5), Qt::AlignLeft|Qt::AlignVCenter, "Adres dostawy");
        y += 5;
        // Dane klienta
        QFont f8b("DejaVu Sans", 8, QFont::Bold), f8("DejaVu Sans", 8);
        QString nazwa_skrocona = clientData.value("short_name").toString();
        std::vector<std::pair<QString, QString>> klient_info = {
            {"Firma:", !nazwa_skrocona.isEmpty() ? nazwa_skrocona : clientData.value("name").toString()},
            {"Nr klienta:", clientData.value("client_number").toString()},
            {"Ulica i nr:", clientData.value("street").toString()},
            {"Kod poczt:", clientData.value("postal_code").toString()},
            {"Miasto:", clientData.value("city").toString()}
        };
        for (int i=0, idx=0; i<klient_info.size(); ++i) {
            if (!klient_info[i].second.isEmpty()) {
                painter.setFont(f8b);
                painter.drawText(PdfUtils::mmToPt(x), PdfUtils::mmToPt(y+idx*6), PdfUtils::mmToPt(22), PdfUtils::mmToPt(6), Qt::AlignLeft|Qt::AlignVCenter, klient_info[i].first);
                painter.setFont(f8);
                painter.drawText(PdfUtils::mmToPt(x+22), PdfUtils::mmToPt(y+idx*6), PdfUtils::mmToPt(38), PdfUtils::mmToPt(6), Qt::AlignLeft|Qt::AlignVCenter, klient_info[i].second);
                idx++;
            }
        }
        // Adres dostawy
        std::vector<std::pair<QString, QString>> addr_info = {
            {"Firma:", orderData.value("delivery_company").toString().isEmpty() ? clientData.value("name").toString() : orderData.value("delivery_company").toString()},
            {"Ulica i nr:", orderData.value("delivery_street").toString().isEmpty() ? clientData.value("street").toString() : orderData.value("delivery_street").toString()},
            {"Kod poczt:", orderData.value("delivery_postal_code").toString().isEmpty() ? clientData.value("postal_code").toString() : orderData.value("delivery_postal_code").toString()},
            {"Miasto:", orderData.value("delivery_city").toString().isEmpty() ? clientData.value("city").toString() : orderData.value("delivery_city").toString()}
        };
        int offset = 0;
        for (int i=0, idx=0; i<addr_info.size(); ++i) {
            if (!addr_info[i].second.isEmpty()) {
                painter.setFont(f8b);
                painter.drawText(PdfUtils::mmToPt(x+70), PdfUtils::mmToPt(y+idx*6), PdfUtils::mmToPt(22), PdfUtils::mmToPt(6), Qt::AlignLeft|Qt::AlignVCenter, addr_info[i].first);
                painter.setFont(f8);
                painter.drawText(PdfUtils::mmToPt(x+92), PdfUtils::mmToPt(y+idx*6), PdfUtils::mmToPt(38), PdfUtils::mmToPt(6), Qt::AlignLeft|Qt::AlignVCenter, addr_info[i].second);
                idx++; offset=idx;
            }
        }
        // Osoba kontaktowa i telefon
        QString contact_person = orderData.value("delivery_contact_person").toString().isEmpty() ? clientData.value("contact_person").toString() : orderData.value("delivery_contact_person").toString();
        QString contact_phone = orderData.value("delivery_phone").toString().isEmpty() ? clientData.value("phone").toString() : orderData.value("delivery_phone").toString();
        if (!contact_person.isEmpty()) {
            painter.setFont(f8b);
            painter.drawText(PdfUtils::mmToPt(x+70), PdfUtils::mmToPt(y+offset*6), PdfUtils::mmToPt(22), PdfUtils::mmToPt(6), Qt::AlignLeft|Qt::AlignVCenter, "Osoba kont:");
            painter.setFont(f8);
            painter.drawText(PdfUtils::mmToPt(x+92), PdfUtils::mmToPt(y+offset*6), PdfUtils::mmToPt(38), PdfUtils::mmToPt(6), Qt::AlignLeft|Qt::AlignVCenter, contact_person);
            offset++;
        }
        if (!contact_phone.isEmpty()) {
            painter.setFont(f8b);
            painter.drawText(PdfUtils::mmToPt(x+70), PdfUtils::mmToPt(y+offset*6), PdfUtils::mmToPt(22), PdfUtils::mmToPt(6), Qt::AlignLeft|Qt::AlignVCenter, "tel:");
            painter.setFont(f8);
            painter.drawText(PdfUtils::mmToPt(x+92), PdfUtils::mmToPt(y+offset*6), PdfUtils::mmToPt(38), PdfUtils::mmToPt(6), Qt::AlignLeft|Qt::AlignVCenter, contact_phone);
            offset++;
        }
        // --- Tabela produkcji ---
        double y_table = y + 32 + ((contact_person.isEmpty() && contact_phone.isEmpty()) ? 0 : (6*(offset-4)));
        // Nagłówek tabeli
        std::vector<QString> headers = {"Lp.", "Wymiar", "Materiał", "Na rolce", "Rdzeń", "Ilość", "Miara", "zam. rolki", "Cena"};
        // Przygotuj dane do tabeli
        std::vector<std::vector<QString>> rows;
        int lp = 1;
        for (const auto& item : orderItems) {
            QString width = item.value("width").toString();
            QString height = item.value("height").toString();
            QString wymiar = (!width.isEmpty() && !height.isEmpty()) ? width+"x"+height : (width+height);
            QString material = item.value("material").toString();
            QString roll_length = item.value("roll_length").toString();
            QString core = item.value("core").toString();
            QString ordered_quantity = item.value("ordered_quantity").toString();
            QString miara = item.value("quantity_type").toString();
            QString zam_rolki = item.value("price").toString();
            QString cena = item.value("price").toString();
            QString cena_typ = item.value("price_type").toString();
            QString cena_sufix = item.value("price").toString();
            rows.push_back({QString::number(lp++), wymiar, material, roll_length, core, ordered_quantity, miara, zam_rolki, cena_sufix});
        }
        // Szerokości kolumn (dynamiczne, w mm, poprawione)
        std::vector<double> col_widths_mm = {10, 20, 24, 18, 16, 16, 14, 18, 18};
        double total_width_mm = page_width - 2*margin_left;
        double sum_width_mm = 0; for (auto w : col_widths_mm) sum_width_mm += w;
        double scale = total_width_mm / sum_width_mm;
        for (auto& w : col_widths_mm) w *= scale;
        // Nagłówek sekcji
        painter.setFont(QFont("DejaVu Sans", 9, QFont::Bold));
        painter.setPen(blue);
        painter.setBrush(blue_bg);
        QRectF prodRect(PdfUtils::mmToPt(x), PdfUtils::mmToPt(y_table), PdfUtils::mmToPt(total_width_mm), PdfUtils::mmToPt(8));
        painter.drawRect(prodRect);
        painter.drawText(prodRect, Qt::AlignLeft|Qt::AlignVCenter, "  Dane produkcji");
        // Nagłówki kolumn
        painter.setFont(QFont("DejaVu Sans", 8, QFont::Bold));
        painter.setPen(Qt::black);
        double col_x = x;
        for (int i=0; i<headers.size(); ++i) {
            QRectF cellRect(PdfUtils::mmToPt(col_x), PdfUtils::mmToPt(y_table+8), PdfUtils::mmToPt(col_widths_mm[i]), PdfUtils::mmToPt(7));
            painter.setBrush(blue_bg);
            painter.setPen(border);
            painter.drawRect(cellRect);
            painter.setPen(Qt::black);
            painter.drawText(cellRect, Qt::AlignCenter, headers[i]);
            col_x += col_widths_mm[i];
        }
        // Wiersze tabeli
        painter.setFont(QFont("DejaVu Sans", 8));
        double row_y = y_table+15;
        for (int i=0; i<rows.size(); ++i) {
            col_x = x;
            QColor bg = (i%2==1) ? grey : white;
            for (int j=0; j<headers.size(); ++j) {
                QRectF cellRect(PdfUtils::mmToPt(col_x), PdfUtils::mmToPt(row_y), PdfUtils::mmToPt(col_widths_mm[j]), PdfUtils::mmToPt(7));
                painter.setBrush(bg);
                painter.setPen(border);
                painter.drawRect(cellRect);
                painter.setPen(Qt::black);
                painter.drawText(cellRect.adjusted(PdfUtils::mmToPt(1),0,-PdfUtils::mmToPt(1),0), Qt::AlignCenter, rows[i][j]);
                col_x += col_widths_mm[j];
            }
            row_y += 7;
        }
        // --- Uwagi ---
        painter.setFont(QFont("DejaVu Sans", 8, QFont::Bold));
        painter.setPen(Qt::black);
        painter.drawText(PdfUtils::mmToPt(x), PdfUtils::mmToPt(row_y+3), PdfUtils::mmToPt(20), PdfUtils::mmToPt(6), Qt::AlignLeft|Qt::AlignVCenter, "Uwagi:");
        painter.setFont(QFont("DejaVu Sans", 8));
        painter.drawText(PdfUtils::mmToPt(x+20), PdfUtils::mmToPt(row_y+3), PdfUtils::mmToPt(110), PdfUtils::mmToPt(6), Qt::AlignLeft|Qt::AlignVCenter, orderData.value("notes").toString());
    };

    // Sekcja 1 (góra)
    drawTicket(0);
    // Sekcja 2 (dół)
    drawTicket(ticket_height + ticket_spacing);
    // Linia cięcia
    painter.setPen(QPen(cut_gray, 1.5, Qt::DashLine));
    painter.drawLine(PdfUtils::mmToPt(margin_left), PdfUtils::mmToPt(cut_line_y), PdfUtils::mmToPt(page_width-margin_left), PdfUtils::mmToPt(cut_line_y));
    painter.setFont(QFont("DejaVu Sans", 9));
    painter.setPen(cut_text);
    painter.drawText(PdfUtils::mmToPt(page_width-margin_left-40), PdfUtils::mmToPt(cut_line_y-4), PdfUtils::mmToPt(40), PdfUtils::mmToPt(6), Qt::AlignRight|Qt::AlignVCenter, "--- cięcie ---");

    painter.end();
    QDesktopServices::openUrl(QUrl::fromLocalFile(outputPath));
    return QFileInfo::exists(outputPath);
}
