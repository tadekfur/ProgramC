#include "stylemanager.h"
#include <QApplication>
#include <QStyleFactory>

void StyleManager::applyStyle(const QString &styleName) {
    qApp->setStyle(QStyleFactory::create(styleName));
}
