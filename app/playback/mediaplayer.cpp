#include "mediaplayer.h"

#include <QBuffer>
#include <QFile>
#include <QThread>
#include <QtConcurrent>

namespace Playback {
  MediaPlayer::MediaPlayer(QObject *parent) : QObject(parent) {
    /*connect(&player, &QMediaPlayer::positionChanged, this, &MediaPlayer::positionChanged);
    connect(&player, &QMediaPlayer::stateChanged, [=](QMediaPlayer::State state) {
      switch (state) {
        case QMediaPlayer::StoppedState:
          emit stateChanged(MediaPlayer::StoppedState);
          break;
        case QMediaPlayer::PlayingState:
          emit stateChanged(MediaPlayer::PlayingState);
          break;
        case QMediaPlayer::PausedState:
          emit stateChanged(MediaPlayer::PausedState);
          break;
      }
    });
    connect(&player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), [=](QMediaPlayer::Error err) {
      emit error(QString("QMediaPlayer error %1").arg(err));
    });
    connect(&stream, &Stream::fillChanged, this, &MediaPlayer::streamBufferfillChanged);
    connect(&stream, &Stream::metadataChanged, this, &MediaPlayer::streamMetaChanged);
    connect(&stream, &Stream::error, [&](const QString& message) {
      qDebug() << "stream error" << message;
      player.stop();
    });*/
    /*connect(this, &MediaPlayer::stateChanged, [](State st) {
      qDebug() << "state change" << st;
    });*/

    connect(&mplayer, &QProcess::errorOccurred, [=](QProcess::ProcessError err) {
      qDebug() << "error" << err;
    });
    connect(&mplayer, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int code) {
      mplayer_state = StoppedState;
      qDebug() << "mplayer finished" << code;
    });

    /*connect(&mplayer, &QProcess::started, [=]() {
      mplayer_state = PlayingState;
      qDebug() << "mplayer started";
      emit stateChanged(mplayer_state);
    });
    connect(&mplayer, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int code) {
      mplayer_state = StoppedState;
      qDebug() << "mplayer finished" << code;
      if (!skip_stop_signal) {
        emit stateChanged(mplayer_state);
      }
      skip_stop_signal = false;
    });
    mplayer_state = StoppedState;
    skip_stop_signal = false;*/


    QStringList args;
    args << "-noconfig";
    args << "all";
    args << "-cache";
    args << "1024";
    args << "-cache-min";
    args << "10";
    args << "-vo";
    args << "null";
    args << "-slave";
    args << "-idle";
    args << "-really-quiet";
    args << "-msglevel";
    args << "global=4";
    args << "-input";
    args << "nodefault-bindings";
    mplayer.start("/tmp/mplayerbin/bin/mplayer", args);
    qDebug() << args.join(" ");
    mplayer.waitForStarted();
  }

  MediaPlayer::~MediaPlayer() {
    //stop();
  }

  MediaPlayer::State MediaPlayer::state() const {
    /*switch (player.state()) {
      case QMediaPlayer::StoppedState:
        return MediaPlayer::StoppedState;
      case QMediaPlayer::PlayingState:
        return MediaPlayer::PlayingState;
      case QMediaPlayer::PausedState:
        return MediaPlayer::PausedState;
    }*/
    return mplayer_state;
  }

  int MediaPlayer::volume() const {
    return player.volume();
  }

  qint64 MediaPlayer::position() const {
    return player.position();
  }

  void MediaPlayer::pause() {
    // TODO: stream pause? prevent buffer overflow
    //player.pause();
    mplayer.write("pause\n");
    mplayer.waitForBytesWritten();
    if (mplayer_state == PausedState) {
      mplayer_state = PlayingState;
      emit stateChanged(MediaPlayer::PlayingState);
    } else if (mplayer_state == PlayingState) {
      mplayer_state = PausedState;
      emit stateChanged(mplayer_state);
    }

    /*mplayer.write("get_time_pos\n");
    qDebug() << mplayer.readAllStandardOutput();*/
  }

  void MediaPlayer::play() {
    /*if (!stream.isRunning() && stream.isValidUrl()) {
      if (!stream.start()) {
        qWarning() << "error starting stream form" << stream.url();
      }
    }*/
    //player.play();

    if (mplayer_state == PausedState) {
      pause();
      return;
    }

    //qDebug() << args.join(" ");

    qDebug() << mplayer.state();

    if (mplayer.state() == QProcess::Running) {
      mplayer.write("pause\n");
      //mplayer.waitForBytesWritten();
      auto s = QString("loadfile \"%1\"\n").arg(media.remove("file://"));
      qDebug() << s;
      mplayer.write(s.toLatin1());
      //mplayer.waitForBytesWritten();
    } else {
      qWarning() << "mplayer dead";
    }
    mplayer_state = PlayingState;
    emit stateChanged(mplayer_state);

  }

  void MediaPlayer::stop() {
    qDebug() << "stop";
    player.stop();
    stream.stop();


    mplayer.write("stop\n");
    mplayer_state = StoppedState;
    emit stateChanged(mplayer_state);
  }

  void MediaPlayer::setPosition(qint64 pos) {
    player.setPosition(pos);
  }

  void MediaPlayer::setVolume(int vol) {
    player.setVolume(vol);
  }

  void MediaPlayer::setMedia(const QUrl &url) {
    stream.stop();
    media = url.toString();
    if (url.scheme() == "file") {
      player.setMedia(url);
    } else {
      stream.setUrl(url);
      player.setMedia(url, &stream);
    }
  }

  void MediaPlayer::removeMedia() {
    player.setMedia(nullptr);
  }
}
