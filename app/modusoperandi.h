#ifndef MODUSOPERANDI_H
#define MODUSOPERANDI_H

#include "config/local.h"
#ifdef ENABLE_MPD_SUPPORT
  #include "mpdconnection.h"
#endif
#include "slidingbanner.h"

#include <QObject>

class ModusOperandi : public QObject {
  Q_OBJECT

public:
  explicit ModusOperandi(Config::Local &local_cfg, SlidingBanner *banner, QObject *parent = nullptr);

  enum ActiveMode {
    MODUS_LOCALFS,
    MODUS_MPD
  };
  Q_ENUM(ActiveMode)

  ActiveMode get() const;

#ifdef ENABLE_MPD_SUPPORT
  MpdConnection mpd_connection;
#endif

public slots:
  void set(ActiveMode new_mode);
  void onLibraryPathChange(const QString &path);
  QString onLibraryPathChange(int idx);

signals:
  void changed(ActiveMode new_mode);
  void mpdReady(const QUrl &url);
  void mpdLost(const QUrl &url);

private:
  Config::Local local_config;
  ActiveMode active;
};

#endif // MODUSOPERANDI_H
