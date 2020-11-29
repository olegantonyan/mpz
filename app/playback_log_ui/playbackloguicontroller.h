#ifndef PLAYBACKLOGCONTROLLER_H
#define PLAYBACKLOGCONTROLLER_H

#include "playback_log_ui/playbackloguimodel.h"
#include "track.h"
#include "config/local.h"
#include "config/global.h"

#include <QObject>
#include <QDebug>
#include <QString>

namespace PlaybackLogUi {
  class Controller : public QObject {
    Q_OBJECT
  public:
    explicit Controller(Config::Local &local_c, Config::Global &global_c, QObject *parent = nullptr);

  signals:
    void jumpToTrack(quint64 track_uid);

  public slots:
    void append(const Track& t);
    void on_monotonicPlaybackTimeIncrement(int by);
    void showWindow();

  private:
    PlaybackLogUi::Model *model;
  };
}


#endif // PLAYBACKLOGCONTROLLER_H
