#include "track.h"
#include "rnjesus.h"
#include "coverart/covers.h"

#include "fileref.h"
#include "tag.h"
#include "tpropertymap.h"

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QHash>

#include <limits>

namespace {
  QString firstProperty(const TagLib::PropertyMap &props, const char *key) {
    const TagLib::StringList v = props.value(key);
    if (v.isEmpty()) {
      return QString();
    }
    return QString(v.front().toCString(true));
  }
}

Track::Track() {
  _uid = 0;
  _begin = 0;
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

  readMetadata();
  setCue(false);

  _format = detectFormat();
}

Track::Track(const QString &fp,
             quint64 bgn,
             const QString &artst,
             const QString &albm,
             const QString &ttle,
             quint16 tracknum,
             quint16 yr,
             quint64 dur,
             quint8 chans,
             quint16 bitrt,
             quint32 samplert) {
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
  _begin = 0;
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

QString Track::formattedTime(quint64 tm) {
  tm /= 1000;
  quint32 seconds = tm % 60;
  quint32 minutes = (tm / 60) % 60;
  quint32 hours = (tm / 60 / 60);

  if (hours == 0) {
    return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
  } else if (hours >= 24) {
    quint32 days = hours / 24;
    return QString("%1d %2").arg(days).arg(formattedTime((tm - days * 86400) * 1000));
  }
  return QString("%1:%2:%3").arg(hours, 2, 10).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
}

bool Track::isValid() const {
  return uid() != 0 && (isMpd() || QFile::exists(path()) || isStream());
}

Track::AudioProperties Track::audioPropertiesOf(const QString &filepath) {
  AudioProperties result;
  const QByteArray encoded = filepath.toUtf8();
  TagLib::FileRef f(encoded.constData());
  if (f.isNull()) {
    return result;
  }
  if (const auto *props = f.audioProperties()) {
    result.duration = static_cast<quint64>(props->lengthInMilliseconds());
    result.channels = static_cast<quint8>(props->channels());
    result.bitrate = static_cast<quint16>(props->bitrate());
    result.sample_rate = static_cast<quint32>(props->sampleRate());
  }
  return result;
}

bool Track::readMetadata() {
  if (isMpd() || !isValid()) {
    return false;
  }
  const QByteArray encoded = path().toUtf8();
  TagLib::FileRef f(encoded.constData());
  if (f.isNull()) {
    return false;
  }

  if (const auto *props = f.audioProperties()) {
    _duration = static_cast<quint64>(props->lengthInMilliseconds());
    _channels = static_cast<quint8>(props->channels());
    _bitrate = static_cast<quint16>(props->bitrate());
    _sample_rate = static_cast<quint32>(props->sampleRate());
  }

  if (TagLib::Tag *tag = f.tag()) {
    const TagLib::PropertyMap tag_props = tag->properties();
    _album_artist = firstProperty(tag_props, "ALBUMARTIST");
    _artist = QString(tag->artist().toCString(true));
    if (_artist.isEmpty()) {
      _artist = _album_artist;
    }
    _album = QString(tag->album().toCString(true));
    _genre = QString(tag->genre().toCString(true));
    _title = QString(tag->title().toCString(true));
    _year = static_cast<quint16>(tag->year());
    _track_number = static_cast<quint16>(tag->track());
    _disc_number = parseDiscNumber(firstProperty(tag_props, "DISCNUMBER"));
  }

  return true;
}

// DISCNUMBER is commonly "1/2" - take the numerator
quint16 Track::parseDiscNumber(const QString &raw) {
  if (raw.isEmpty()) {
    return 0;
  }
  const QString num = raw.section('/', 0, 0).trimmed();
  bool ok = false;
  const uint value = num.toUInt(&ok);
  if (!ok || value > std::numeric_limits<quint16>::max()) {
    return 0;
  }
  return static_cast<quint16>(value);
}

bool Track::reload() {
  return readMetadata();
}

void Track::generateUidByHashing(const QString &prefix) {
  QByteArray utf8 = (prefix + filepath).toUtf8();
  _uid = qHashBits(utf8.constData(), utf8.size());
}

void Track::setPlaylistName(const QString &pln) {
  _playlist_name = pln;
}

void Track::setMpd(const QUrl &mpd_server_url) {
  _mpd_server_url = mpd_server_url;
}

void Track::setDuration(quint64 dur) {
  _duration = dur;
}

void Track::setAlbumArtist(const QString &aa) {
  _album_artist = aa;
}

void Track::setGenre(const QString &g) {
  _genre = g;
}

void Track::setDiscNumber(quint16 dn) {
  _disc_number = dn;
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

QString Track::album_artist() const {
  return _album_artist;
}

QString Track::genre() const {
  return _genre;
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

bool Track::isMpd() const {
  return !mpd_server_url().isEmpty();
}

QUrl Track::mpd_server_url() const {
  return _mpd_server_url;
}

quint64 Track::uid() const {
  return _uid;
}

QString Track::dir() const {
  if (!_dir_cached) {
    _dir_cache = QFileInfo(path()).absoluteDir().canonicalPath();
    _dir_cached = true;
  }
  return _dir_cache;
}

QString Track::formattedTitle() const {
  if (year() == 0) {
    return QString("%1 - %2 - %3").arg(artist()).arg(album()).arg(title());
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
  return CoverArt::Covers::instance().get(filepath, artist(), album());
}

void Track::setAudioFormat(quint32 sample_rate, quint8 channels, quint16 bitrate) {
  _sample_rate = sample_rate;
  _channels = channels;
  _bitrate = bitrate;
}

quint32 Track::sample_rate() const {
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
  if (!_filename_cached) {
    _filename_cache = QFileInfo(path()).fileName();
    _filename_cached = true;
  }
  return _filename_cache;
}

quint16 Track::track_number() const {
  return _track_number;
}

quint16 Track::disc_number() const {
  return _disc_number;
}

quint32 Track::begin() const {
  return _begin;
}

QString Track::playlist_name() const {
  return _playlist_name;
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
