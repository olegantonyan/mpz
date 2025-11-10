#include "modusoperandi.h"

#include <QDebug>

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
