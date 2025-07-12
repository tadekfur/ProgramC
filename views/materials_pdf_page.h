// Wstępna integracja: zamiast PdfViewer w MaterialsMode, wyświetl nowy formularz
#include "materials_order_form.h"
#include <QVBoxLayout>
#include <QWidget>

class MaterialsPdfPage : public QWidget {
    Q_OBJECT
public:
    explicit MaterialsPdfPage(QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        form = new MaterialsOrderForm(this);
        layout->addWidget(form);
        setLayout(layout);
    }
    MaterialsOrderForm *form;
};
