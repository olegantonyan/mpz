#include "streammetadata.h"

#include <QDebug>
#include <QRegularExpression>

StreamMetaData::StreamMetaData() {
}

StreamMetaData::StreamMetaData(const QString &title) : title_explicit(title) {
}

bool StreamMetaData::isEmpty() const {
  return _data.isEmpty() && _status_now_playing.isEmpty();
}

void StreamMetaData::insert(const QString &key, const QString &value) {
  _data.insert(key, value);
}

void StreamMetaData::setStatusNowPlaying(const QString &raw) {
  _status_now_playing = raw;
}

void StreamMetaData::clear() {
  _data.clear();
  _status_now_playing.clear();
}

quint16 StreamMetaData::bitrate() const {
  bool ok = false;
  auto i = _data.value("icy-br", "0").toUInt(&ok);
  if (!ok) {
    return 0;
  }
  return static_cast<quint16>(i);
}

quint32 StreamMetaData::samplerate() const {
  bool ok = false;
  auto i = _data.value("icy-sr", "0").toUInt(&ok);
  if (!ok) {
    return 0;
  }
  return i;
}

QString StreamMetaData::nowPlaying() const {
  static const QRegularExpression r("StreamTitle=('|\")(.*?)('|\");");
  auto match = r.match(_data.value("stream", ""));
  if (match.hasMatch() && !match.captured(2).isEmpty()) {
    return match.captured(2);
  }
  return _status_now_playing;
}

QString StreamMetaData::artist() const {
  if (!title_explicit.isEmpty()) {
    return title_explicit;
  }
  auto np = nowPlaying();
  return np.isEmpty() ? "" : np.split(" - ").first();
}

QString StreamMetaData::title() const {
  if (!title_explicit.isEmpty()) {
    return title_explicit;
  }
  auto np = nowPlaying();
  return np.isEmpty() ? "" : np.split(" - ").last();
}

QString StreamMetaData::format() const {
  return _data.value("content-type", "");
}
