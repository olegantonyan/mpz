#include "modusoperandi.h"

#include <QDebug>
#include <QtConcurrent>

ModusOperandi::ModusOperandi(Config::Local &local_cfg, SlidingBanner *banner, QObject *parent) : QObject{parent}, SingleInstanceGuard("ModusOperandi"), local_config(local_cfg) {
  active = MODUS_LOCALFS;
  const int idx = local_config.currentLibraryPath();
  if (idx >= 0 && idx < local_config.libraryPaths().size()) {
    active = modeOf(local_config.libraryPaths().at(idx));
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
#endif
#ifdef ENABLE_MPZD_SUPPORT
  connect(&mpzd_client, &MpzdClient::Client::connected, this, &ModusOperandi::mpzdReady);
  connect(&mpzd_client, &MpzdClient::Client::error, this, &ModusOperandi::mpzdLost);
  connect(&mpzd_client, &MpzdClient::Client::disconnected, this, &ModusOperandi::mpzdLost);
  connect(&mpzd_client, &MpzdClient::Client::error, this, [=](const QUrl &url, const QString &message) {
    Q_UNUSED(url);
    if (active == MODUS_MPZD) {
      banner->showMessage(tr("mpzd error") + "\n" + message, SlidingBanner::BannerType::Error);
    }
  });
  connect(&mpzd_client, &MpzdClient::Client::connected, this, [=] {
    banner->showMessage(tr("mpzd connected"), SlidingBanner::BannerType::Success, 3456);
  });
#endif
  connect(this, &ModusOperandi::changed, this, [=](auto mode) {
    if (mode == ModusOperandi::MODUS_LOCALFS) {
      banner->collapse();
    }
  });
}

ModusOperandi::ActiveMode ModusOperandi::modeOf(const QString &library_path) {
  if (library_path.startsWith("mpd://")) {
    return MODUS_MPD;
  }
  if (library_path.startsWith("mpzd://")) {
    return MODUS_MPZD;
  }
  return MODUS_LOCALFS;
}

bool ModusOperandi::isRemote() const {
  return active != MODUS_LOCALFS;
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
  ActiveMode new_mode = modeOf(path);
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
#ifdef ENABLE_MPZD_SUPPORT
  if (new_mode == MODUS_MPZD) {
    mpzd_client.openConnection(QUrl(path));
  } else {
    mpzd_client.closeConnection();
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
