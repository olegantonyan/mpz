#ifndef LOCAL_H
#define LOCAL_H

#include "storage.h"

#include <QByteArray>

namespace Config {
  class Local {
  public:
    Local();

    bool sync();

    bool saveWindowGeometry(const QByteArray &v);
    QByteArray windowGeomentry() const;

    bool saveWindowState(const QByteArray &v);
    QByteArray windowState() const;

  private:
    Config::Storage storage;
  };
}

#endif // LOCAL_H
