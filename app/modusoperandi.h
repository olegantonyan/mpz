#ifndef MODUSOPERANDI_H
#define MODUSOPERANDI_H

#include "config/local.h"
#include "singleinstanceguard.h"
#ifdef ENABLE_MPD_SUPPORT
  #include "mpd_client/client.h"
#endif
#ifdef ENABLE_MPZD_SUPPORT
  #include "mpzd_client/client.h"
#endif
#include "slidingbanner.h"

#include <QObject>

class ModusOperandi : public QObject, public SingleInstanceGuard<ModusOperandi> {
  Q_OBJECT

public:
  explicit ModusOperandi(Config::Local &local_cfg, SlidingBanner *banner, QObject *parent = nullptr);

  enum ActiveMode {
    MODUS_LOCALFS,
    MODUS_MPD,
    MODUS_MPZD
  };
  Q_ENUM(ActiveMode)

  ActiveMode get() const;

#ifdef ENABLE_MPD_SUPPORT
  MpdClient::Client mpd_client;
#endif
#ifdef ENABLE_MPZD_SUPPORT
  MpzdClient::Client mpzd_client;
#endif

  static ActiveMode modeOf(const QString &library_path);
  bool isRemote() const;

public slots:
  void set(ActiveMode new_mode);
  void onLibraryPathChange(const QString &path);
  QString onLibraryPathChange(int idx);

signals:
  void changed(ActiveMode new_mode);
  void mpdReady(const QUrl &url);
  void mpdLost(const QUrl &url);
  void mpzdReady(const QUrl &url);
  void mpzdLost(const QUrl &url);

private:
  Config::Local &local_config;
  ActiveMode active;
};

#endif // MODUSOPERANDI_H
