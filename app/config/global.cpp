#include "global.h"

namespace Config {
  Global::Global() : storage("global.yml") {
  }

  bool Global::sync() {
    return storage.save();
  }
}
