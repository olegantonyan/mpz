#ifndef MACDOCKICON_H
#define MACDOCKICON_H

#include "config/global.h"

#include <QIcon>
#include <QObject>
#include <QTimer>
#include <QVector>

class MacDockIcon : public QObject {
  Q_OBJECT
public:
  explicit MacDockIcon(Config::Global &global_c, QObject *parent = nullptr);

public slots:
  void start();
  void pause();
  void stop();

private:
  bool enabled() const;
  void buildFrames();
  void tick();

  Config::Global &global_conf;
  QVector<QIcon> frames;
  QTimer timer;
  int frame = 0;
};

#endif // MACDOCKICON_H
