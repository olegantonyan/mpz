#include "modusoperandi.h"

#include <QDebug>
#include <QtConcurrent>

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
#ifdef ENABLE_MPD_SUPPORT
  connect(&mpd_connection, &MpdConnection::connected, this, &ModusOperandi::mpdReady);
  connect(&mpd_connection, &MpdConnection::disconnected, this, &ModusOperandi::mpdLost);
  connect(&mpd_connection, &MpdConnection::lost, this, &ModusOperandi::mpdLost);
  connect(&mpd_connection, &MpdConnection::failed, this, &ModusOperandi::mpdLost);
#endif
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

void ModusOperandi::onLibraryPathChange(const QString &path) {
  ActiveMode new_mode = path.startsWith("mpd://") ? MODUS_MPD : MODUS_LOCALFS;
  if (new_mode != active) {
    set(new_mode);
  }
#ifdef ENABLE_MPD_SUPPORT
  if (new_mode == MODUS_MPD) {
    mpd_connection.establish(QUrl(path));
  } else {
    mpd_connection.destroy();
  }
#endif
}

QString ModusOperandi::onLibraryPathChange(int idx) {
  if (0 <= idx && idx < local_config.libraryPaths().size()) {
    auto path = local_config.libraryPaths()[idx];
    if (!path.isEmpty()) {
      onLibraryPathChange(path);
      local_config.saveCurrentLibraryPath(idx);
    }
    return path;
  }
  return QString();
}

ModusOperandi::ActiveMode ModusOperandi::get() const {
  return active;
}
