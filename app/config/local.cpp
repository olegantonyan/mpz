#include "local.h"

#include <QDebug>

namespace Config {
  Local::Local() : storage("local.yml") {
  }

  bool Local::saveWindowGeometry(const QByteArray &v) {
    qDebug() << v.data();

    return true;
  }

  bool Local::saveWindowState(const QByteArray &v) {
    qDebug() << v.data();

    return true;
  }
}
