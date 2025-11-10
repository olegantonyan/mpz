#ifndef MODUSOPERANDI_H
#define MODUSOPERANDI_H

#include "config/local.h"

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

public slots:
  void set(ActiveMode new_mode);

signals:
  void changed(ActiveMode new_mode);

private:
  Config::Local local_config;
  ActiveMode active;
};

#endif // MODUSOPERANDI_H
