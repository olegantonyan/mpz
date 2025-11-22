#include "mediaplayer.h"


namespace Playback {
  namespace Mpd {
  MediaPlayer::MediaPlayer(quint32 stream_buffer_size, QByteArray outdevid, MpdConnection &conn, QObject *parent) : Playback::MediaPlayer{stream_buffer_size, outdevid, parent}, connection(conn) {
    }

    MediaPlayer::State Playback::Mpd::MediaPlayer::state() const {
      return MediaPlayer::State::StoppedState;
    }

    int Playback::Mpd::MediaPlayer::volume() const {
      return 0;
    }

    qint64 Playback::Mpd::MediaPlayer::position() const {
      return 0;
    }

    void Playback::Mpd::MediaPlayer::pause() {
    }

    void Playback::Mpd::MediaPlayer::play() {
    }

    void Playback::Mpd::MediaPlayer::stop() {
    }

    void Playback::Mpd::MediaPlayer::setPosition(qint64 position) {
    }

    void Playback::Mpd::MediaPlayer::setVolume(int volume) {
    }

    void Playback::Mpd::MediaPlayer::setTrack(const Track &track) {
    }

    void Playback::Mpd::MediaPlayer::clearTrack() {
    }

    void Playback::Mpd::MediaPlayer::setOutputDevice(QByteArray deviceid) {
    }
  }
}

