#include "track.h"

#include "fileref.h"
#include "tag.h"
#include "tpropertymap.h"

#include <QDateTime>
#include <QDebug>

Track::Track(const QString &fp) {
  filepath = fp;

  TagLib::FileRef f(path().toStdString().c_str());
  if(!f.isNull()) {
    _duration = static_cast<quint32>(f.audioProperties()->length());
    _channels = static_cast<quint32>(f.audioProperties()->channels());
    _bitrate = static_cast<quint32>(f.audioProperties()->bitrate());
    _sample_rate = static_cast<quint32>(f.audioProperties()->sampleRate());
    if (f.tag()) {
      TagLib::Tag *tag = f.tag();
      _artist = QString(tag->artist().toCString(true));
      _album = QString(tag->album().toCString(true));
      _title = QString(tag->title().toCString(true));
      _year = static_cast<quint16>(tag->year());
    }
  }

  //qDebug() << formattedAudioInfo();
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
  return _title;
}

quint16 Track::year() const {
  return _year;
}

quint32 Track::duration() const {
  return _duration;
}

QString Track::formattedDuration() const {
  quint32 seconds = duration() % 60;
  quint32 minutes = (duration() / 60) % 60;
  quint32 hours = (duration() / 60 / 60);

  if (hours == 0) {
    return QString("%1:%2").arg(minutes, 2, 10).arg(seconds, 2, 10, QChar('0'));
  }
  return QString("%1:%2:%3").arg(hours, 2, 10).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
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
  return QString("%1kbps | %2Hz | %3").arg(bitrate()).arg(sample_rate()).arg(c);
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
