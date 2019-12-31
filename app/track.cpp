#include "track.h"

#include "fileref.h"
#include "tag.h"
#include "tpropertymap.h"

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QDir>

Track::Track() {
  Track("");
  _uid = 0;
}

Track::Track(const QString &fp) {
  _uid = QRandomGenerator::global()->generate64();

  filepath = fp;

  TagLib::FileRef f(path().toStdString().c_str());
  if(!f.isNull()) {
    if (f.audioProperties()) {
      _duration = static_cast<quint32>(f.audioProperties()->length());
      _channels = static_cast<quint8>(f.audioProperties()->channels());
      _bitrate = static_cast<quint16>(f.audioProperties()->bitrate());
      _sample_rate = static_cast<quint16>(f.audioProperties()->sampleRate());
    }
    if (f.tag()) {
      TagLib::Tag *tag = f.tag();
      _artist = QString(tag->artist().toCString(true));
      _album = QString(tag->album().toCString(true));
      _title = QString(tag->title().toCString(true));
      _year = static_cast<quint16>(tag->year());
      _track_number = static_cast<quint16>(tag->track());
    }
  }

  _format = QFileInfo(path()).suffix().toUpper();
}

QString Track::formattedTime(quint32 tm) {
  quint32 seconds = tm % 60;
  quint32 minutes = (tm / 60) % 60;
  quint32 hours = (tm / 60 / 60);

  if (hours == 0) {
    return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
  }
  return QString("%1:%2:%3").arg(hours, 2, 10).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
}

bool Track::isValid() const {
  return uid() != 0 && QFile::exists(path());
}

QString Track::path() const {
  return filepath;
}

QString Track::artist() const {
  return _artist;
}

QString Track::album() const {
  return _album;
}

QString Track::title() const {
  if (_title.length() == 0) {
    return filename();
  } else {
    return _title;
  }
}

quint16 Track::year() const {
  return _year;
}

quint32 Track::duration() const {
  return _duration;
}

QString Track::formattedDuration() const {
  return Track::formattedTime(duration());
}

QString Track::formattedAudioInfo() const {
  QString c;
  if (channels() == 1) {
    c = "Mono";
  } else if (channels() == 2) {
    c = "Stereo";
  } else {
    c = QString("%1 channels").arg(channels());
  }
  return QString("%1 | %2kbps | %3Hz | %4").arg(format()).arg(bitrate()).arg(sample_rate()).arg(c);
}

quint64 Track::uid() const {
  return _uid;
}

QString Track::dir() const {
  return QFileInfo(path()).absoluteDir().canonicalPath();
}

quint16 Track::sample_rate() const {
  return _sample_rate;
}

quint8 Track::channels() const {
  return _channels;
}

quint16 Track::bitrate() const {
  return _bitrate;
}

QString Track::format() const {
  return _format;
}

QString Track::filename() const {
  return QFileInfo(path()).fileName();
}

quint16 Track::track_number() const {
  return _track_number;
}
