#ifndef PLAYBACKVIEW_H
#define PLAYBACKVIEW_H

#include "controls.h"
#include "track.h"
#include "playback/mediaplayer.h"

#include <QObject>
#include <memory>
#include <QEvent>
#include <QMouseEvent>
#include <QTimer>

namespace Playback {
  class Controller : public QObject {
    Q_OBJECT

  public:
    enum State {
      Stopped,
      Playing,
      Paused
    };

    explicit Controller(const Playback::Controls &c, quint32 stream_buffer_size, QObject *parent = nullptr);

    Playback::Controls controls() const;
    int volume() const;
    bool isStopped() const;
    enum State state() const;
    int position() const;
    const Track& currentTrack() const;

  signals:
    void started(const Track &track);
    void stopped();
    void paused(const Track &track);
    void progress(const Track &track, int current_seconds);
    void prevRequested();
    void nextRequested();
    void startRequested();
    void volumeChanged(int value);
    void seeked(int value);
    void streamFill(const Track &track, quint32 bytes);
    void trackChanged(const Track &track);
    void monotonicPlaybackTimerIncrement(int by);

  public slots:
    void play(const Track &track);
    void stop();
    void setVolume(int value);
    void seek(int seconds);

  private:
    void on_seek(int position);
    QString time_text(int pos) const;
    void setup_monotonic_timer();

    Playback::Controls _controls;
    MediaPlayer _player;
    Track _current_track;
    bool next_after_stop;
    QTimer monotonic_timer;

  private slots:
    void on_positionChanged(quint64 pos);
    void on_stateChanged(MediaPlayer::State state);
    void on_error(const QString &message);

  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
  };
}
#endif // PLAYBACKVIEW_H
