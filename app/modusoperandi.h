#ifndef MODUSOPERANDI_H
#define MODUSOPERANDI_H

#include "config/local.h"
#include "mpdconnection.h"

#include <QObject>

class ModusOperandi : public QObject {
  Q_OBJECT

public:
  explicit ModusOperandi(Config::Local &local_cfg, QObject *parent = nullptr);

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

signals:
  void changed(ActiveMode new_mode);
  void mpdChanged(const QString &path);

private:
  Config::Local local_config;
  ActiveMode active;

  void waitForConnected() const;
};

#endif // MODUSOPERANDI_H
