#ifndef LOCAL_H
#define LOCAL_H

#include "storage.h"

#include <QByteArray>

namespace Config {
  class Local {
  public:
    Local();

    bool saveWindowGeometry(const QByteArray &v);
    bool saveWindowState(const QByteArray &v);

  //private:
    Config::Storage storage;
  };
}

#endif // LOCAL_H
