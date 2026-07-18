#ifndef GAPLESS_MEDIAPLAYER_H
#define GAPLESS_MEDIAPLAYER_H

#include "playback/gapless/engine.h"
#include "playback/mediaplayer.h"
#include "track.h"

#include <QByteArray>
#include <QObject>

namespace Playback::Gapless {
  class GaplessMediaPlayer : public Playback::MediaPlayer {
    Q_OBJECT
  public:
    explicit GaplessMediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, QObject *parent = nullptr);

    MediaPlayer::State state() override;
    qint64 position() override;

  public slots:
    void pause() override;
    void play() override;
    void stop() override;
    void setPosition(qint64 position) override;
    void setVolume(int volume) override;
    void setTrack(const Track &track) override;
    void clearTrack() override;
    void prepareNextTrack(const Track &track) override;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    void setOutputDevice(QByteArray deviceid) override;
#endif

  private:
    Engine engine;
    enum class Backend { None, Qmp, Engine } backend = Backend::None;
  };
}

#endif // GAPLESS_MEDIAPLAYER_H
