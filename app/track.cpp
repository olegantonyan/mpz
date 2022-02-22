#include "track.h"
#include "rnjesus.h"
#include "coverart/covers.h"

#ifdef USE_SYSTEM_TAGLIB
  #include "taglib/fileref.h"
  #include "taglib/tag.h"
  #include "taglib/tpropertymap.h"
#else
  #include "fileref.h"
  #include "tag.h"
  #include "tpropertymap.h"
#endif

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QDir>

Track::Track() {
  Track("", 0);
  _uid = 0;
  _duration = 0;
  _channels = 0;
  _bitrate = 0;
  _sample_rate = 0;
  _year = 0;
  _track_number = 0;
  setCue(false);
}

Track::Track(const QString &fp, quint32 bgn) {
  _uid = generateUid();
  _begin = bgn;

  filepath = fp;

  fillAudioProperties();
  fillTags();
  setCue(false);

  _format = detectFormat();
}

Track::Track(const QString &fp,
             quint32 bgn,
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
  _begin = bgn;

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

  setCue(false);

  _format = detectFormat();
}

Track::Track(const QUrl &stream_url, const QString &filepath_reference) {
  _duration = 0;
  _channels = 0;
  _bitrate = 0;
  _sample_rate = 0;
  _year = 0;
  _track_number = 0;

  setCue(false);

  _uid = generateUid();
  _stream_url = stream_url;
  filepath = filepath_reference;
}

QString Track::formattedTime(quint32 tm) {
  quint32 seconds = tm % 60;
  quint32 minutes = (tm / 60) % 60;
  quint32 hours = (tm / 60 / 60);

  if (hours == 0) {
    return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
  } else if (hours >= 24) {
    quint32 days = hours / 24;
    return QString("%1d %2").arg(days).arg(formattedTime(tm - days * 86400));
  }
  return QString("%1:%2:%3").arg(hours, 2, 10).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
}

bool Track::isValid() const {
  return uid() != 0 && (QFile::exists(path()) || isStream());
}

bool Track::fillAudioProperties() {
  TagLib::FileRef f(path().toUtf8().constData());
  if(!f.isNull()) {
    if (f.audioProperties()) {
      _duration = static_cast<quint32>(f.audioProperties()->length());
      _channels = static_cast<quint8>(f.audioProperties()->channels());
      _bitrate = static_cast<quint16>(f.audioProperties()->bitrate());
      _sample_rate = static_cast<quint16>(f.audioProperties()->sampleRate());
      return true;
    }
  }
  return false;
}

bool Track::fillTags() {
  TagLib::FileRef f(path().toUtf8().constData());
  if(!f.isNull()) {
    if (f.tag()) {
      TagLib::Tag *tag = f.tag();
      _artist = QString(tag->artist().toCString(true));
      _album = QString(tag->album().toCString(true));
      _title = QString(tag->title().toCString(true));
      _year = static_cast<quint16>(tag->year());
      _track_number = static_cast<quint16>(tag->track());
      return true;
    }
  }
  return false;
}

bool Track::reload() {
  return fillAudioProperties() && fillTags();
}

void Track::setDuration(quint32 dur) {
  _duration = dur;
}

void Track::setCue(bool is_cue) {
  _cue = is_cue;
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
    return _stream_meta.artist();
  }
  return _artist;
}

QString Track::album() const {
  return _album;
}

QString Track::title() const {
  if (_title.isEmpty()) {
    if (isStream()) {
      if (streamMeta().title().isEmpty()) {
        QUrl displayable_url;
        displayable_url.setScheme(_stream_url.scheme());
        displayable_url.setHost(_stream_url.host());
        displayable_url.setPort(_stream_url.port());
        displayable_url.setPath(_stream_url.path());
        return displayable_url.toString();
      } else {
        return streamMeta().title();
      }
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
    c.append(" Mono");
  } else if (channels() == 2) {
    c.append(" Stereo");
  }
  if (bitrate() > 0) {
    c.append(QString(" %1kbps").arg(bitrate()));
  }
  if (sample_rate() > 0) {
    c.append(QString(" %1Hz").arg(sample_rate()));
  }
  return c;
}

QString Track::shortText() const {
  if (!title().isEmpty() && !artist().isEmpty()) {
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
  if (year() == 0) {
    return QString("%1 - %2 - %4").arg(artist()).arg(album()).arg(title());
  }
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

QString Track::artCover() const {
  return CoverArt::Covers::instance().get(filepath);
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
  if (isStream()) {
    return streamMeta().format();
  }
  return _format;
}

QString Track::filename() const {
  return QFileInfo(path()).fileName();
}

quint16 Track::track_number() const {
  return _track_number;
}

quint32 Track::begin() const {
  return _begin;
}

bool Track::isCue() const {
  return _cue;
}

quint64 Track::generateUid() const {
  return RNJesus::generate();
}

QString Track::detectFormat() const {
  return QFileInfo(path()).suffix().toUpper();
}
