#include "logger.h"
#include <QDebug>

void Logger::log(const QString &msg) {
    qDebug() << msg;
}
