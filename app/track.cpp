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
  _duration = 0;
  _channels = 0;
  _bitrate = 0;
  _sample_rate = 0;
  _year = 0;
  _track_number = 0;
}

Track::Track(const QString &fp) {
  _uid = generateUid();

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

  _format = detectFormat();
}

Track::Track(const QString &fp,
             const QString &artst,
             const QString &albm,
             const QString &ttle,
             quint16 tracknum,
             quint16 yr,
             quint32 dur,
             quint8 chans,
             quint16 bitrt,
             quint16 samplert) {
  _uid = generateUid();

  filepath = fp;
  _duration = dur;
  _year = yr;
  _title = ttle;
  _artist = artst;
  _album = albm;
  _sample_rate = samplert;
  _bitrate = bitrt;
  _channels = chans;
  _track_number = tracknum;

  _format = detectFormat();
}

Track::Track(const QUrl &stream_url) {
  _duration = 0;
  _channels = 0;
  _bitrate = 0;
  _sample_rate = 0;
  _year = 0;
  _track_number = 0;

  _uid = generateUid();
  _stream_url = stream_url;
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
  return uid() != 0 && (QFile::exists(path()) || isStream());
}

QString Track::path() const {
  return filepath;
}

QUrl Track::url() const {
  if (!isStream()) {
    return QUrl::fromLocalFile(path());
  }
  return _stream_url;
}

QString Track::artist() const {
  if (isStream()) {
    return _stream_meta.stream();
  }
  return _artist;
}

QString Track::album() const {
  return _album;
}

QString Track::title() const {
  if (_title.length() == 0) {
    if (isStream()) {
      return _stream_url.toString();
    } else {
      return filename();
    }
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
  QString c = format();
  if (channels() == 1) {
    c =+ " Mono";
  } else if (channels() == 2) {
    c =+ " Stereo";
  }
  if (bitrate() > 0) {
    c += QString(" %1kbps").arg(bitrate());
  }
  if (sample_rate() > 0) {
    c += QString(" %1Hz").arg(sample_rate());
  }
  return c;
}

QString Track::shortText() const {
  if (isStream()) {
    return streamMeta().stream();
  } else if (!title().isEmpty() && !artist().isEmpty()) {
    return artist() + " - " + title();
  }  else if (!title().isEmpty()) {
    return title();
  } else if (!filename().isEmpty()) {
    return filename();
  }
  return url().toString();
}

quint64 Track::uid() const {
  return _uid;
}

QString Track::dir() const {
  return QFileInfo(path()).absoluteDir().canonicalPath();
}

QString Track::formattedTitle() const {
  return QString("%1 - %2 (%3) - %4").arg(artist()).arg(album()).arg(year()).arg(title());
}

bool Track::isStream() const {
  return !_stream_url.isEmpty();
}

void Track::setStreamMeta(const StreamMetaData &meta) {
  _stream_meta = meta;
}

void Track::clearStreamMeta() {
  _stream_meta.clear();
}

const StreamMetaData &Track::streamMeta() const {
  return _stream_meta;
}

quint16 Track::sample_rate() const {
  if (isStream()) {
    return streamMeta().samplerate();
  }
  return _sample_rate;
}

quint8 Track::channels() const {
  return _channels;
}

quint16 Track::bitrate() const {
  if (isStream()) {
    return streamMeta().bitrate();
  }
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

quint64 Track::generateUid() const {
  return QRandomGenerator::global()->generate64();
}

QString Track::detectFormat() const {
  return QFileInfo(path()).suffix().toUpper();
}
