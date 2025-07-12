#include "status_legend_widget.h"
#include "../utils/app_constants.h"

StatusLegendWidget::StatusLegendWidget(QWidget *parent) 
    : QWidget(parent) {
    setupUI();
}

void StatusLegendWidget::setupUI() {
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(2);
    layout->setContentsMargins(8, 5, 8, 8);
    
    auto *title = new QLabel("DNI DO WYSYŁKI", this);
    title->setStyleSheet("font-size:10px; font-weight:bold; color:#333;");
    title->setWordWrap(false);
    layout->addWidget(title, 0, Qt::AlignLeft);
    
    // Definicje kolorów i tekstów z wykorzystaniem stałych
    const struct { QString color; QString text; } legends[] = {
        {AppConstants::COLOR_OVERDUE, "po terminie"},
        {AppConstants::COLOR_TODAY, "dzień wysyłki"},
        {AppConstants::COLOR_ONE_DAY, "1 dzień"},
        {AppConstants::COLOR_TWO_DAYS, "2 dni"},
        {AppConstants::COLOR_THREE_DAYS, "3 dni"},
        {AppConstants::COLOR_FOUR_DAYS, "4 dni"}
    };
    
    for (const auto &legend : legends) {
        addLegendRow(QColor(legend.color), legend.text);
    }
    
    layout->addStretch();
}

void StatusLegendWidget::addLegendRow(const QColor &color, const QString &text) {
    auto *row = new QWidget(this);
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(6);
    
    auto *colorBox = new QLabel(row);
    colorBox->setFixedSize(12, 10);
    colorBox->setStyleSheet(QString("background:%1; border:1px solid #999;").arg(color.name()));
    
    auto *label = new QLabel(text, row);
    label->setStyleSheet("font-size:9px; color:#333;");
    label->setWordWrap(false);
    
    rowLayout->addWidget(colorBox);
    rowLayout->addWidget(label, 1);
    
    layout()->addWidget(row);
}
