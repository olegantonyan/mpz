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
}

void ModusOperandi::set(ActiveMode new_mode) {
  bool change = false;
  if (new_mode != active) {
    change = true;
  }
  active = new_mode;
  if (change) {
    qDebug() << "ModusOperandi switched to" << active;
    if (active == MODUS_LOCALFS) {
      emit changed(active);
    } else {
      (void)QtConcurrent::run(QThreadPool::globalInstance(), [=]() {
        waitForConnected();
        emit changed(active);
      });
    }
  }
}

void ModusOperandi::onLibraryPathChange(const QString &path) {
  ActiveMode new_mode = path.startsWith("mpd://") ? MODUS_MPD : MODUS_LOCALFS;

  if (new_mode != active) {
    set(new_mode);
  } else if (new_mode == MODUS_MPD && active == MODUS_MPD) {
    qDebug() << "ModusOperandi MPD changed to" << path;
    (void)QtConcurrent::run(QThreadPool::globalInstance(), [=]() {
      waitForConnected();
      emit mpdChanged(path);
    });
  }
}

void ModusOperandi::waitForConnected() const {
  QEventLoop loop;
  connect(&mpd_connection, &MpdConnection::connected, &loop, &QEventLoop::quit);
}

ModusOperandi::ActiveMode ModusOperandi::get() const {
  return active;
}
