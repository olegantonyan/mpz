#include "track.h"
#include "rnjesus.h"
#include "coverart/covers.h"

#include "taglib_compat.h"

#include "fileref.h"
#include "tag.h"
#include "tpropertymap.h"

#include <aiffproperties.h>
#include <apeproperties.h>
#include <asfproperties.h>
#include <flacproperties.h>
#include <mp4properties.h>
#include <trueaudioproperties.h>
#include <wavpackproperties.h>
#include <wavproperties.h>
#if MPZ_TAGLIB_SINCE(2, 0)
  #include <dsdiffproperties.h>
  #include <dsfproperties.h>
#endif
#if MPZ_TAGLIB_SINCE(2, 1)
  #include <shortenproperties.h>
#endif
#if MPZ_TAGLIB_SINCE(2, 2)
  #include <matroskaproperties.h>
#endif

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QHash>

namespace {
  quint16 bitsPerSampleOf(TagLib::AudioProperties *p) {
    if (auto *x = dynamic_cast<TagLib::FLAC::Properties *>(p)) {
      return quint16(x->bitsPerSample());
    }
    if (auto *x = dynamic_cast<TagLib::RIFF::WAV::Properties *>(p)) {
      return quint16(x->bitsPerSample());
    }
    if (auto *x = dynamic_cast<TagLib::RIFF::AIFF::Properties *>(p)) {
      return quint16(x->bitsPerSample());
    }
    if (auto *x = dynamic_cast<TagLib::APE::Properties *>(p)) {
      return quint16(x->bitsPerSample());
    }
    if (auto *x = dynamic_cast<TagLib::WavPack::Properties *>(p)) {
      return quint16(x->bitsPerSample());
    }
    if (auto *x = dynamic_cast<TagLib::TrueAudio::Properties *>(p)) {
      return quint16(x->bitsPerSample());
    }
    if (auto *x = dynamic_cast<TagLib::MP4::Properties *>(p)) {
      return quint16(x->bitsPerSample());
    }
    if (auto *x = dynamic_cast<TagLib::ASF::Properties *>(p)) {
      return quint16(x->bitsPerSample());
    }
#if MPZ_TAGLIB_SINCE(2, 0)
    if (auto *x = dynamic_cast<TagLib::DSF::Properties *>(p)) {
      return quint16(x->bitsPerSample());
    }
    if (auto *x = dynamic_cast<TagLib::DSDIFF::Properties *>(p)) {
      return quint16(x->bitsPerSample());
    }
#endif
#if MPZ_TAGLIB_SINCE(2, 1)
    if (auto *x = dynamic_cast<TagLib::Shorten::Properties *>(p)) {
      return quint16(x->bitsPerSample());
    }
#endif
#if MPZ_TAGLIB_SINCE(2, 2)
    if (auto *x = dynamic_cast<TagLib::Matroska::Properties *>(p)) {
      return quint16(x->bitsPerSample());
    }
#endif
    return 0;
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
  initPathParts();
}

Track::Track(const QString &fp, quint32 bgn) {
  _uid = generateUid();
  _begin = bgn;

  filepath = fp;
  initPathParts();

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
  initPathParts();
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

Track::Track(const QUrl &stream_url, const QString &filepath_reference, const QString &title) {
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
  initPathParts();
  _title = title;
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
    result.bits_per_sample = bitsPerSampleOf(f.audioProperties());
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
    const TagLib::PropertyMap props = tag->properties();

    const TagLib::StringList album_artist = props.value("ALBUMARTIST");
    _album_artist = album_artist.isEmpty() ? QString() : QString(album_artist.front().toCString(true));

    const TagLib::StringList disc = props.value("DISCNUMBER");
    _disc_number = disc.isEmpty() ? QString() : QString(disc.front().toCString(true));

    _artist = QString(tag->artist().toCString(true));
    if (_artist.isEmpty()) {
      _artist = _album_artist;
    }
    _album = QString(tag->album().toCString(true));
    _title = QString(tag->title().toCString(true));
    _year = static_cast<quint16>(tag->year());
    _track_number = static_cast<quint16>(tag->track());
  }

  return true;
}

void Track::setReplayGain(const ReplayGain::Gain &g) {
  _replay_gain = g;
}

const ReplayGain::Gain &Track::replayGain() const {
  return _replay_gain;
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

void Track::setCue(bool is_cue) {
  _cue = is_cue;
}

void Track::setAlbumArtist(const QString &aa) {
  _album_artist = aa;
}

void Track::setDiscNumber(const QString &disc) {
  _disc_number = disc;
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

QString Track::album_artist() const {
  return _album_artist;
}

QString Track::album() const {
  return _album;
}

QString Track::displayUrl() const {
  return _stream_url.toDisplayString(QUrl::RemoveUserInfo | QUrl::RemoveQuery | QUrl::RemoveFragment);
}

QString Track::title() const {
  // A live ICY title wins over _title (which may be a station name).
  if (isStream()) {
    if (!streamMeta().title().isEmpty()) {
      return streamMeta().title();
    }
    if (!_title.isEmpty()) {
      return _title;
    }
    return displayUrl();
  }
  return _title.isEmpty() ? filename() : _title;
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
  return url().toDisplayString();
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
  return _dir;
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

QString Track::stationName() const {
  return _title.isEmpty() ? displayUrl() : _title;
}

QString Track::streamNowPlaying() const {
  if (!isStream()) {
    return QString();
  }
  const QString a = _stream_meta.artist();
  const QString t = _stream_meta.title();
  if (!a.isEmpty() && !t.isEmpty()) {
    return a + " - " + t;
  }
  return a.isEmpty() ? t : a;
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
  return _filename;
}

void Track::initPathParts() {
  const QFileInfo info(filepath);
  _dir = info.absoluteDir().canonicalPath();
  _filename = info.fileName();
}

quint16 Track::track_number() const {
  return _track_number;
}

QString Track::disc_number() const {
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
