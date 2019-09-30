#include "local.h"

#include <QDebug>

namespace Config {
  Local::Local() : storage("local.yml") {
  }

  bool Local::sync() {
    return storage.save();
  }

  bool Local::saveWindowGeometry(const QByteArray &v) {
    return storage.set("window_geometry", v);
  }

  QByteArray Local::windowGeomentry() const {
    return storage.getByteArray("window_geometry");
  }

  bool Local::saveWindowState(const QByteArray &v) {
    return storage.set("window_state", v);
  }

  QByteArray Local::windowState() const {
    return storage.getByteArray("window_state");
  }
}
