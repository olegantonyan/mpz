#ifndef PLAYBACKVIEW_H
#define PLAYBACKVIEW_H

#include "controls.h"
#include "track.h"

#include <QObject>

namespace Playback {
  class View : public QObject {
    Q_OBJECT
  public:
    explicit View(const Playback::Controls &c, QObject *parent = nullptr);

  signals:
    void started(const Track &track);
    void stopped();
    void paused(const Track &track);

  public slots:
    void play(const Track &track);
    void stop();

  private:
    Playback::Controls controls;
  };

}
#endif // PLAYBACKVIEW_H
