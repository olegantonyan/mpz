#ifndef PLAYBACKVIEW_H
#define PLAYBACKVIEW_H

#include "controls.h"
#include "track.h"

#include <QObject>
#include <QMediaPlayer>
#include <memory>
#include <QEvent>
#include <QMouseEvent>

namespace Playback {
  class View : public QObject {
    Q_OBJECT
  public:
    explicit View(const Playback::Controls &c, QObject *parent = nullptr);

  signals:
    void started(const Track &track);
    void stopped();
    void paused(const Track &track);
    void prevRequested();
    void nextRequested();
    void startRequested();

  public slots:
    void play(const Track &track);
    void stop();

  private:
    Playback::Controls controls;
    QMediaPlayer player;
    Track _current_track;

    void on_seek(int position);

    QString time_text(int pos) const;

    bool next_after_stop;

  private slots:
    void on_positionChanged(quint64 pos);
    void on_stateChanged(QMediaPlayer::State state);
    void on_error(QMediaPlayer::Error error);

  protected:
    bool eventFilter(QObject *obj, QEvent *event);
  };
}
#endif // PLAYBACKVIEW_H
