#include "modusoperandi.h"

#include <QDebug>
#include <QtConcurrent>

ModusOperandi::ModusOperandi(Config::Local &local_cfg, SlidingBanner *banner, QObject *parent) : QObject{parent}, local_config(local_cfg) {
  active = MODUS_LOCALFS;
  const int idx = local_config.currentLibraryPath();
  if (idx >= 0 && idx < local_config.libraryPaths().size()
      && local_config.libraryPaths().at(idx).startsWith("mpd://")) {
    active = MODUS_MPD;
  }
  qDebug() << "ModusOperandi initilized in" << active;
#ifdef ENABLE_MPD_SUPPORT
  connect(&mpd_client, &MpdClient::Client::connected, this, &ModusOperandi::mpdReady);
  connect(&mpd_client, &MpdClient::Client::error, this, &ModusOperandi::mpdLost);
  connect(&mpd_client, &MpdClient::Client::disconnected, this, &ModusOperandi::mpdLost);

  connect(&mpd_client, &MpdClient::Client::error, this, [=](const QUrl &url, const QString &message) {
    Q_UNUSED(url);
    if (active == MODUS_MPD) {
      banner->showMessage(tr("mpd error") + "\n" + message, SlidingBanner::BannerType::Error);
    }
  });
  connect(&mpd_client, &MpdClient::Client::connected, this, [=] {
    banner->showMessage(tr("mpd connected"), SlidingBanner::BannerType::Success, 3456);
  });
  connect(this, &ModusOperandi::changed, this, [=](auto mode) {
    if (mode != ModusOperandi::ActiveMode::MODUS_MPD) {
      banner->collapse();
    }
  });
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
    mpd_client.openConnection(QUrl(path));
  } else {
    mpd_client.closeConnection();
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
