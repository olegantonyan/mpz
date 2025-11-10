#ifndef MODUSOPERANDI_H
#define MODUSOPERANDI_H

#include "config/local.h"

#include <QObject>

class ModusOperandi : public QObject {
  Q_OBJECT

public:
  enum ActiveMode {
    MODUS_LOCALFS,
    MODUS_MPD
  };
  Q_ENUM(ActiveMode)

  static ModusOperandi& init(Config::Local &local_cfg, QObject *parent = nullptr);
  static ModusOperandi& instance();

  ActiveMode get() const;

public slots:
  void set(ActiveMode new_mode);

signals:
  void changed(ActiveMode new_mode);

private:
  explicit ModusOperandi(Config::Local &local_cfg, QObject *parent = nullptr);
  ~ModusOperandi() = default;

  static ModusOperandi *_instance;

  Config::Local local_config;
  ActiveMode active;

  Q_DISABLE_COPY(ModusOperandi)
};

#endif // MODUSOPERANDI_H
