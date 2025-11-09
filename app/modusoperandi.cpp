#include "modusoperandi.h"

#include <QDebug>

ModusOperandi* ModusOperandi::_instance = nullptr;

ModusOperandi::ModusOperandi(Config::Local &local_cfg, QObject *parent) : QObject{parent}, local_config(local_cfg) {
  if (local_config.libraryPaths().empty()) {
    active = MODUS_LOCALFS;
  } else {
    int current_index = local_config.currentLibraryPath();
    auto current_path = local_config.libraryPaths().at(current_index);
    if (current_path.startsWith("mpd://")) {
      active = MODUS_MPD;
    } else {
      active = MODUS_LOCALFS;
    }
  }
  qDebug() << "ModusOperandi initilized in" << active;
}

ModusOperandi &ModusOperandi::init(Config::Local &local_cfg, QObject *parent) {
  if (_instance) {
    return instance();
  }
  _instance = new ModusOperandi(local_cfg, parent);
  return instance();
}

ModusOperandi &ModusOperandi::instance() {
  Q_ASSERT(_instance && "ModusOperandi not initialized!");
  return *_instance;
}

void ModusOperandi::set(ActiveMode new_mode) {
  bool change = false;
  if (new_mode != active) {
    change = true;
  }
  active = new_mode;
  if (change) {
    qDebug() << "ModusOperandi switched to" << active;
    emit changed(active);
  }
}

ModusOperandi::ActiveMode ModusOperandi::get() const {
  return active;
}
