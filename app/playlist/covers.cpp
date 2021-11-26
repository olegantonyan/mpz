#include "covers.h"

#include <QDebug>

namespace Playlist {
  Covers &Covers::instance() {
    static Covers *self = nullptr;
    if (self == nullptr) {
      self = new Covers();
    }
    return *self;
  }

  Covers::Covers(QObject *parent) : QObject(parent) {
  }

  QString Covers::get(const QString &filepath) {
    return QString();
  }
}
