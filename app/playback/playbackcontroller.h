#ifndef PLAYBACKVIEW_H
#define PLAYBACKVIEW_H

#include "controls.h"
#include "track.h"
#include "playback/mediaplayer.h"
#include "modusoperandi.h"
#ifdef ENABLE_MPD_SUPPORT
  #include "playback/mpd/mediaplayer.h"
#endif

#include <QObject>
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

    explicit Controller(const Playback::Controls &c, quint32 stream_buffer_size, QByteArray outdevid, ModusOperandi &modus, QObject *parent = nullptr);

    Playback::Controls controls() const;
    int volume();
    bool isStopped();
    enum State state();
    int position();
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
    void trackChangedQuery(const QString &track_path, const QString &playlist_name_hint);

  public slots:
    void play(const Track &track);
    void stop();
    void setVolume(int value);
    void seek(int seconds);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    void setOutputDevice(QByteArray deviceid);
#endif
    void setCurrentTrack(const Track &track);
    void trackChangedQueryComplete(const Track &track);

  private:
    void on_seek(int position);
    QString time_text(quint64 pos) const;
    void setup_monotonic_timer();
    MediaPlayer &player();

    Playback::Controls _controls;
    MediaPlayer _player;
#ifdef ENABLE_MPD_SUPPORT
    Mpd::MediaPlayer _mpdplayer;
#endif
    Track _current_track;
    QTimer monotonic_timer;
    ModusOperandi &modus_operndi;

  private slots:
    void on_controlsPause();
    void on_controlsNext();
    void on_controlsPrev();
    void on_controlsPlay();
    void on_positionChanged(quint64 pos);
    void on_stateChanged(MediaPlayer::State state);
    void switchTo(ModusOperandi::ActiveMode new_mode);

  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
  };
}
#endif // PLAYBACKVIEW_H
