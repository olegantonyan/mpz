#ifndef GLOBAL_H
#define GLOBAL_H

#include "storage.h"

namespace Config {
  class Global {
  public:
    Global();

    bool sync();

  private:
    Config::Storage storage;
  };
}

#endif // GLOBAL_H
