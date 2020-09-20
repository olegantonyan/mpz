#ifndef PLAYBACKLOG_H
#define PLAYBACKLOG_H

#include "playback_log_ui/playbackloguimodel.h"
#include "track.h"

#include <QObject>
#include <QDebug>
#include <QString>

namespace PlaybackLogUi {
  class Controller : public QObject {
    Q_OBJECT
  public:
    explicit Controller(QObject *parent = nullptr);

  signals:
    void jumpToTrack(quint64 track_uid);

  public slots:
    void append(const Track& t);
    void showWindow();

  private:
    PlaybackLogUi::Model *model;
  };
}


#endif // PLAYBACKLOG_H
