#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QColor>

class StatusLegendWidget : public QWidget {
    Q_OBJECT

public:
    explicit StatusLegendWidget(QWidget *parent = nullptr);

private:
    void setupUI();
    void addLegendRow(const QColor &color, const QString &text);
};
